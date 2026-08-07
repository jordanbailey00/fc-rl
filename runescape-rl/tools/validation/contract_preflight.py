#!/usr/bin/env python3
"""Load and validate the Fight Caves contract from a compiled backend."""

from __future__ import annotations

import argparse
import configparser
import ctypes
import hashlib
import json
import os
from pathlib import Path
import sys
from typing import Any


class ContractError(RuntimeError):
    """A fail-closed training-contract validation error."""


OBSERVATION_VERSION = (
    "fight_caves_puffer_policy_obs_v8_prayer_timing_mask8_no_supplies"
)
ACTION_VERSION = "fight_caves_multidiscrete_3_head_no_supplies_v2_prayer8"
PRAYER_TIMING_VERSION = (
    "fight_caves_prayer_timing_v1_tick_start_snapshot_flick_drain_jad_lock"
)
SUPPORTED_REWARD_VERSIONS = frozenset(
    {
        "fight_caves_v4_progress_npc_heal_penalty_m0005_"
        "prayer_snapshot_flick_drain",
        "fight_caves_v38_fc_revamp_step2_raw_work_progress_"
        "prayer_conserve_no_attack_prayer_snapshot_flick_drain",
    }
)

# This is the one validation-side exact oracle required by CONTRACT-004. The
# authoritative compiled values come from fc_contracts.h through the backend
# symbol; shell launchers, manifests, and evaluators do not repeat arithmetic.
EXPECTED_COMPILED_FIELDS: dict[str, Any] = {
    "contract_dump_schema_version": 1,
    "policy_obs_size": 285,
    "puffer_obs_size": 319,
    "puffer_action_dims": [17, 9, 8],
    "puffer_mask_size": 34,
    "core_obs_size": 474,
    "core_action_dims": [17, 9, 8, 3, 2, 65, 65],
    "core_action_mask": 169,
    "reward_feature_count": 20,
    "observation_version": OBSERVATION_VERSION,
    "action_version": ACTION_VERSION,
    "prayer_timing_version": PRAYER_TIMING_VERSION,
    "state_hash_version": 1,
}
REQUIRED_CONTRACT_FIELDS = frozenset(
    {*EXPECTED_COMPILED_FIELDS, "reward_version", "active_loadout"}
)


def sha256_file(path: str | Path) -> str:
    resolved = Path(path)
    if not resolved.is_file():
        raise ContractError(f"required file is unavailable: {resolved}")
    digest = hashlib.sha256()
    with resolved.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def contract_identity(contract: dict[str, Any]) -> str:
    encoded = json.dumps(contract, sort_keys=True, separators=(",", ":")).encode()
    return hashlib.sha256(encoded).hexdigest()


def _mismatch(field: str, expected: Any, actual: Any) -> ContractError:
    return ContractError(
        f"compiled contract mismatch for {field}: expected={expected!r}, actual={actual!r}"
    )


def load_compiled_contract(backend_so: str | Path) -> dict[str, Any]:
    """Read the project-owned JSON symbol from the selected binary."""
    backend = Path(backend_so).resolve()
    if not backend.is_file():
        raise ContractError(f"compiled backend is unavailable: {backend}")
    try:
        library = ctypes.CDLL(str(backend))
    except OSError as exc:
        raise ContractError(f"cannot load compiled backend {backend}: {exc}") from exc
    try:
        symbol = library.fc_training_contract_json
    except AttributeError as exc:
        raise ContractError(
            f"compiled backend {backend} omits fc_training_contract_json"
        ) from exc
    symbol.argtypes = []
    symbol.restype = ctypes.c_char_p
    raw = symbol()
    if raw is None:
        raise ContractError("fc_training_contract_json returned null")
    try:
        decoded = raw.decode("utf-8")
    except UnicodeDecodeError as exc:
        raise ContractError("compiled contract is not UTF-8") from exc
    try:
        contract = json.loads(decoded)
    except json.JSONDecodeError as exc:
        raise ContractError(f"compiled contract is not valid JSON: {exc}") from exc
    if not isinstance(contract, dict):
        raise ContractError(
            f"compiled contract must be a JSON object: actual={type(contract).__name__}"
        )
    return contract


def validate_compiled_contract(
    contract: dict[str, Any],
    *,
    expected_active_loadout: str | None = None,
) -> None:
    for field in REQUIRED_CONTRACT_FIELDS:
        if field not in contract:
            expected = EXPECTED_COMPILED_FIELDS.get(field, "required")
            raise _mismatch(field, expected, "<missing>")
    for field, expected in EXPECTED_COMPILED_FIELDS.items():
        actual = contract.get(field)
        if actual != expected:
            raise _mismatch(field, expected, actual)

    reward_version = contract.get("reward_version")
    if reward_version not in SUPPORTED_REWARD_VERSIONS:
        raise _mismatch(
            "reward_version", sorted(SUPPORTED_REWARD_VERSIONS), reward_version
        )
    active_loadout = contract.get("active_loadout")
    if not isinstance(active_loadout, str) or not active_loadout:
        raise _mismatch("active_loadout", "nonempty compiled identifier", active_loadout)
    if expected_active_loadout is not None and active_loadout != expected_active_loadout:
        raise _mismatch("active_loadout", expected_active_loadout, active_loadout)


def read_config_contract(config_path: str | Path) -> dict[str, Any]:
    path = Path(config_path).resolve()
    parser = configparser.ConfigParser()
    try:
        loaded = parser.read(path, encoding="utf-8")
    except configparser.Error as exc:
        raise ContractError(f"cannot parse config {path}: {exc}") from exc
    if not loaded or not parser.has_section("run"):
        raise ContractError(f"config is missing a readable [run] section: {path}")

    values: dict[str, Any] = {}
    try:
        values["manifest_schema_version"] = parser.getint(
            "run", "manifest_schema_version"
        )
    except (ValueError, configparser.Error) as exc:
        raise ContractError(
            f"config [run].manifest_schema_version is invalid in {path}: {exc}"
        ) from exc
    for field in ("observation_version", "action_version", "reward_version"):
        if not parser.has_option("run", field):
            raise ContractError(f"config [run].{field} is unavailable in {path}")
        values[field] = parser.get("run", field).strip().strip("'\"")

    if values["manifest_schema_version"] != 2:
        raise ContractError(
            "config contract mismatch for manifest_schema_version: "
            f"expected=2, actual={values['manifest_schema_version']!r}"
        )
    if values["observation_version"] != OBSERVATION_VERSION:
        raise ContractError(
            "config contract mismatch for observation_version: "
            f"expected={OBSERVATION_VERSION!r}, "
            f"actual={values['observation_version']!r}"
        )
    if values["action_version"] != ACTION_VERSION:
        raise ContractError(
            "config contract mismatch for action_version: "
            f"expected={ACTION_VERSION!r}, actual={values['action_version']!r}"
        )
    if values["reward_version"] not in SUPPORTED_REWARD_VERSIONS:
        raise ContractError(
            "config contract mismatch for reward_version: "
            f"expected={sorted(SUPPORTED_REWARD_VERSIONS)!r}, "
            f"actual={values['reward_version']!r}"
        )
    return values


def verify_selected_config(
    contract: dict[str, Any],
    config_path: str | Path,
    synced_config_path: str | Path,
) -> dict[str, Any]:
    source = Path(config_path).resolve()
    synced = Path(synced_config_path).resolve()
    source_hash = sha256_file(source)
    synced_hash = sha256_file(synced)
    if source.read_bytes() != synced.read_bytes():
        raise ContractError(
            "source/copied config byte mismatch: expected byte-identical files, "
            f"source_sha256={source_hash}, synced_sha256={synced_hash}"
        )

    values = read_config_contract(source)
    for field in ("observation_version", "action_version", "reward_version"):
        expected = contract.get(field)
        actual = values[field]
        if actual != expected:
            raise ContractError(
                f"selected config mismatch for {field}: expected={expected!r}, "
                f"actual={actual!r}"
            )
    return {
        "source_path": str(source),
        "synced_path": str(synced),
        "source_sha256": source_hash,
        "synced_sha256": synced_hash,
        "byte_identical": True,
    }


def build_verified_preflight(
    backend_so: str | Path,
    config_path: str | Path,
    synced_config_path: str | Path,
    active_loadout: str,
) -> dict[str, Any]:
    backend = Path(backend_so).resolve()
    contract = load_compiled_contract(backend)
    validate_compiled_contract(contract, expected_active_loadout=active_loadout)
    config = verify_selected_config(contract, config_path, synced_config_path)
    return {
        "preflight_schema_version": 1,
        "contract": contract,
        "contract_identity": contract_identity(contract),
        "backend": {
            "path": str(backend),
            "sha256": sha256_file(backend),
        },
        "config": config,
    }


def load_verified_preflight(
    preflight_path: str | Path,
    *,
    backend_so: str | Path,
    config_path: str | Path,
    synced_config_path: str | Path,
    active_loadout: str,
) -> dict[str, Any]:
    path = Path(preflight_path).resolve()
    if not path.is_file():
        raise ContractError(f"verified preflight artifact is unavailable: {path}")
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ContractError(f"cannot read verified preflight artifact {path}: {exc}") from exc
    if not isinstance(payload, dict):
        raise ContractError("verified preflight artifact must be a JSON object")
    if payload.get("preflight_schema_version") != 1:
        raise ContractError(
            "verified preflight schema mismatch: "
            f"expected=1, actual={payload.get('preflight_schema_version')!r}"
        )
    contract = payload.get("contract")
    if not isinstance(contract, dict):
        raise ContractError("verified preflight contract is unavailable")
    validate_compiled_contract(contract, expected_active_loadout=active_loadout)

    current_contract = load_compiled_contract(backend_so)
    validate_compiled_contract(
        current_contract, expected_active_loadout=active_loadout
    )
    if current_contract != contract:
        raise ContractError(
            "verified/current backend contract mismatch: "
            f"expected={contract!r}, actual={current_contract!r}"
        )
    current_config = verify_selected_config(
        contract, config_path, synced_config_path
    )
    recorded_config = payload.get("config")
    if not isinstance(recorded_config, dict):
        raise ContractError("verified preflight config identity is unavailable")
    for field in (
        "source_path",
        "synced_path",
        "source_sha256",
        "synced_sha256",
        "byte_identical",
    ):
        if recorded_config.get(field) != current_config[field]:
            raise ContractError(
                f"verified preflight config mismatch for {field}: "
                f"expected={recorded_config.get(field)!r}, "
                f"actual={current_config[field]!r}"
            )
    return payload


def write_json_atomic(path: str | Path, payload: dict[str, Any]) -> None:
    output = Path(path).resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_name(f".{output.name}.{os.getpid()}.tmp")
    with temporary.open("w", encoding="utf-8") as handle:
        json.dump(payload, handle, indent=2, sort_keys=True)
        handle.write("\n")
    os.replace(temporary, output)


def _require_sha256(value: Any, field: str) -> str:
    if (
        not isinstance(value, str)
        or len(value) != 64
        or any(character not in "0123456789abcdef" for character in value)
    ):
        raise ContractError(
            f"invalid {field}: expected=64 lowercase hexadecimal characters, "
            f"actual={value!r}"
        )
    return value


def load_preflight_artifact(preflight_path: str | Path) -> dict[str, Any]:
    """Load a freshly verified artifact for downstream artifact validation."""
    path = Path(preflight_path).resolve()
    if not path.is_file():
        raise ContractError(f"verified preflight artifact is unavailable: {path}")
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ContractError(f"cannot read verified preflight artifact {path}: {exc}") from exc
    if not isinstance(payload, dict):
        raise ContractError("verified preflight artifact must be a JSON object")
    if payload.get("preflight_schema_version") != 1:
        raise ContractError(
            "verified preflight schema mismatch: "
            f"expected=1, actual={payload.get('preflight_schema_version')!r}"
        )
    contract = payload.get("contract")
    if not isinstance(contract, dict):
        raise ContractError("verified preflight contract is unavailable")
    validate_compiled_contract(contract)
    expected_identity = contract_identity(contract)
    actual_identity = payload.get("contract_identity")
    if actual_identity != expected_identity:
        raise ContractError(
            "verified preflight contract identity mismatch: "
            f"expected={expected_identity!r}, actual={actual_identity!r}"
        )
    backend = payload.get("backend")
    config = payload.get("config")
    if not isinstance(backend, dict) or not isinstance(config, dict):
        raise ContractError("verified preflight backend/config identity is unavailable")
    _require_sha256(backend.get("sha256"), "preflight backend sha256")
    _require_sha256(config.get("source_sha256"), "preflight config sha256")
    return payload


def checkpoint_contract_marker(preflight: dict[str, Any]) -> dict[str, Any]:
    return {
        "checkpoint_contract_schema_version": 2,
        "artifact_type": "fight_caves_checkpoint_directory",
        "contract_identity": preflight["contract_identity"],
        "contract": preflight["contract"],
    }


def _read_json_object(path: Path, description: str) -> dict[str, Any]:
    if not path.is_file():
        raise ContractError(f"{description} is unavailable: {path}")
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ContractError(f"cannot read {description} {path}: {exc}") from exc
    if not isinstance(payload, dict):
        raise ContractError(f"{description} must be a JSON object: {path}")
    return payload


def validate_checkpoint_marker(
    marker_path: str | Path,
    preflight: dict[str, Any],
) -> dict[str, Any]:
    path = Path(marker_path).resolve()
    marker = _read_json_object(path, "checkpoint contract sidecar")
    if marker.get("checkpoint_contract_schema_version") != 2:
        raise ContractError(
            "checkpoint sidecar schema mismatch: "
            f"expected=2, actual={marker.get('checkpoint_contract_schema_version')!r}"
        )
    if marker.get("artifact_type") != "fight_caves_checkpoint_directory":
        raise ContractError(
            "checkpoint sidecar artifact type mismatch: "
            "expected='fight_caves_checkpoint_directory', "
            f"actual={marker.get('artifact_type')!r}"
        )
    marker_contract = marker.get("contract")
    if not isinstance(marker_contract, dict):
        raise ContractError("checkpoint sidecar contract is unavailable")
    expected_contract = preflight["contract"]
    for field in sorted(set(expected_contract) | set(marker_contract)):
        expected = expected_contract.get(field, "<missing>")
        actual = marker_contract.get(field, "<missing>")
        if actual != expected:
            raise ContractError(
                f"checkpoint contract mismatch for {field}: "
                f"expected={expected!r}, actual={actual!r}"
            )
    expected_identity = preflight["contract_identity"]
    calculated_identity = contract_identity(marker_contract)
    actual_identity = marker.get("contract_identity")
    if actual_identity != calculated_identity:
        raise ContractError(
            "checkpoint sidecar self-identity mismatch: "
            f"expected={calculated_identity!r}, actual={actual_identity!r}"
        )
    if actual_identity != expected_identity:
        raise ContractError(
            "checkpoint contract identity mismatch: "
            f"expected={expected_identity!r}, actual={actual_identity!r}"
        )
    return marker


def prepare_checkpoint_directory(
    checkpoint_root: str | Path,
    preflight: dict[str, Any],
) -> tuple[Path, Path]:
    root = Path(checkpoint_root).resolve()
    contract_dir = root / "contracts" / preflight["contract_identity"]
    marker_path = contract_dir / "contract.json"
    contract_dir.mkdir(parents=True, exist_ok=True)
    if marker_path.exists():
        validate_checkpoint_marker(marker_path, preflight)
    else:
        write_json_atomic(marker_path, checkpoint_contract_marker(preflight))
    return contract_dir, marker_path


def _checkpoint_marker_for_path(checkpoint: Path, checkpoint_root: Path) -> Path:
    adjacent = checkpoint.with_name(f"{checkpoint.name}.contract.json")
    if adjacent.is_file():
        return adjacent

    try:
        checkpoint.relative_to(checkpoint_root)
    except ValueError as exc:
        raise ContractError(
            "explicit checkpoint is outside the contract checkpoint root and "
            f"has no adjacent sidecar: actual={checkpoint}"
        ) from exc

    current = checkpoint.parent
    while True:
        marker = current / "contract.json"
        if marker.is_file():
            return marker
        if current == checkpoint_root or current.parent == current:
            break
        current = current.parent
    raise ContractError(
        f"checkpoint contract sidecar is unavailable: actual={checkpoint}"
    )


def _checkpoint_resolution(
    checkpoint: Path,
    marker_path: Path,
    preflight: dict[str, Any],
    request_mode: str,
) -> dict[str, Any]:
    if not checkpoint.is_file():
        raise ContractError(f"checkpoint file is unavailable: actual={checkpoint}")
    validate_checkpoint_marker(marker_path, preflight)
    return {
        "checkpoint_resolution_schema_version": 1,
        "request_mode": request_mode,
        "contract_identity": preflight["contract_identity"],
        "resolved_path": str(checkpoint.resolve()),
        "file_size": checkpoint.stat().st_size,
        "checkpoint_sha256": sha256_file(checkpoint),
        "sidecar_path": str(marker_path.resolve()),
        "sidecar_contract_identity": preflight["contract_identity"],
    }


def expected_checkpoint_parameter_bytes(
    contract: dict[str, Any],
    config_path: str | Path,
    default_config_path: str | Path,
) -> int:
    parser = configparser.ConfigParser()
    default_path = Path(default_config_path).resolve()
    selected_path = Path(config_path).resolve()
    loaded = parser.read([default_path, selected_path], encoding="utf-8")
    if str(default_path) not in loaded or str(selected_path) not in loaded:
        raise ContractError(
            "checkpoint policy config is unavailable: "
            f"expected={[str(default_path), str(selected_path)]!r}, actual={loaded!r}"
        )
    try:
        hidden_size = parser.getint("policy", "hidden_size")
        num_layers = parser.getint("policy", "num_layers")
        network_name = parser.get("torch", "network")
    except (configparser.Error, ValueError) as exc:
        raise ContractError(f"cannot read checkpoint policy topology: {exc}") from exc
    if network_name != "MinGRU":
        raise ContractError(
            "unsupported raw-checkpoint network: "
            f"expected='MinGRU', actual={network_name!r}"
        )
    if hidden_size <= 0 or num_layers <= 0:
        raise ContractError(
            "invalid checkpoint policy topology: "
            f"expected=positive hidden_size/num_layers, "
            f"actual={hidden_size}/{num_layers}"
        )
    input_size = contract["puffer_obs_size"]
    action_dims = contract["puffer_action_dims"]
    parameter_floats = (
        input_size * hidden_size
        + (sum(action_dims) + 1) * hidden_size
        + num_layers * 3 * hidden_size * hidden_size
    )
    return parameter_floats * 4


def validate_checkpoint_parameter_bytes(
    resolution: dict[str, Any],
    expected_bytes: int,
    contract: dict[str, Any],
) -> None:
    actual_bytes = resolution["file_size"]
    if actual_bytes == expected_bytes:
        return
    raise ContractError(
        "checkpoint raw weight size mismatch: "
        f"expected_policy_obs={contract['policy_obs_size']}, "
        f"actual_policy_obs={contract['policy_obs_size']}, "
        f"expected_puffer_obs={contract['puffer_obs_size']}, "
        f"actual_puffer_obs={contract['puffer_obs_size']}, "
        f"expected_action_dims={contract['puffer_action_dims']!r}, "
        f"actual_action_dims={contract['puffer_action_dims']!r}, "
        f"expected_parameter_bytes={expected_bytes}, "
        f"actual_parameter_bytes={actual_bytes}, "
        f"observation_version={contract['observation_version']!r}, "
        f"action_version={contract['action_version']!r}, "
        f"reward_version={contract['reward_version']!r}, "
        f"prayer_timing_version={contract['prayer_timing_version']!r}, "
        f"state_hash_version={contract['state_hash_version']!r}"
    )


def resolve_checkpoint(
    request: str,
    checkpoint_root: str | Path,
    preflight: dict[str, Any],
    checkpoint_path: str | Path | None = None,
) -> dict[str, Any]:
    root = Path(checkpoint_root).resolve()
    if request == "explicit":
        if checkpoint_path is None:
            raise ContractError("explicit checkpoint resolution requires --checkpoint-path")
        checkpoint = Path(checkpoint_path).resolve()
        marker = _checkpoint_marker_for_path(checkpoint, root)
        return _checkpoint_resolution(checkpoint, marker, preflight, request)
    if request != "latest":
        raise ContractError(
            f"unsupported checkpoint request: expected='explicit' or 'latest', actual={request!r}"
        )
    if not root.is_dir():
        raise ContractError(f"checkpoint root is unavailable: actual={root}")

    compatible: list[tuple[int, str, Path, Path]] = []
    for marker in sorted(root.rglob("contract.json")):
        try:
            validate_checkpoint_marker(marker, preflight)
        except ContractError:
            continue
        for candidate in marker.parent.rglob("*.bin"):
            if not candidate.is_file():
                continue
            stat = candidate.stat()
            compatible.append((stat.st_mtime_ns, str(candidate.resolve()), candidate, marker))
    if not compatible:
        raise ContractError(
            "no compatible checkpoint found: "
            f"expected_contract_identity={preflight['contract_identity']!r}, "
            "actual='<none>'"
        )
    _, _, checkpoint, marker = max(compatible)
    return _checkpoint_resolution(checkpoint, marker, preflight, request)


def load_checkpoint_resolution(
    resolution_path: str | Path,
    preflight: dict[str, Any],
    *,
    expected_request_mode: str | None = None,
) -> dict[str, Any]:
    path = Path(resolution_path).resolve()
    resolution = _read_json_object(path, "checkpoint resolution artifact")
    if resolution.get("checkpoint_resolution_schema_version") != 1:
        raise ContractError(
            "checkpoint resolution schema mismatch: "
            f"expected=1, actual={resolution.get('checkpoint_resolution_schema_version')!r}"
        )
    request_mode = resolution.get("request_mode")
    if request_mode not in {"explicit", "latest"}:
        raise ContractError(
            "checkpoint resolution request mode mismatch: "
            f"expected='explicit' or 'latest', actual={request_mode!r}"
        )
    if expected_request_mode is not None and request_mode != expected_request_mode:
        raise ContractError(
            "checkpoint resolution request mode mismatch: "
            f"expected={expected_request_mode!r}, actual={request_mode!r}"
        )
    expected_identity = preflight["contract_identity"]
    for field in ("contract_identity", "sidecar_contract_identity"):
        if resolution.get(field) != expected_identity:
            raise ContractError(
                f"checkpoint resolution {field} mismatch: "
                f"expected={expected_identity!r}, actual={resolution.get(field)!r}"
            )
    checkpoint_value = resolution.get("resolved_path")
    sidecar_value = resolution.get("sidecar_path")
    if not isinstance(checkpoint_value, str) or not isinstance(sidecar_value, str):
        raise ContractError("checkpoint resolution omits resolved_path or sidecar_path")
    checkpoint = Path(checkpoint_value).resolve()
    marker = Path(sidecar_value).resolve()
    validated = _checkpoint_resolution(checkpoint, marker, preflight, request_mode)
    for field in (
        "resolved_path",
        "file_size",
        "checkpoint_sha256",
        "sidecar_path",
        "sidecar_contract_identity",
        "contract_identity",
    ):
        if resolution.get(field) != validated[field]:
            raise ContractError(
                f"checkpoint resolution {field} mismatch: "
                f"expected={validated[field]!r}, actual={resolution.get(field)!r}"
            )
    return resolution


def validate_trace_artifact(
    trace_path: str | Path,
    preflight: dict[str, Any],
) -> dict[str, Any]:
    path = Path(trace_path).resolve()
    trace = _read_json_object(path, "action trace artifact")
    if trace.get("trace_schema_version") != 1:
        raise ContractError(
            "trace schema mismatch: "
            f"expected=1, actual={trace.get('trace_schema_version')!r}"
        )
    if trace.get("artifact_type") != "fight_caves_action_trace":
        raise ContractError(
            "trace artifact type mismatch: expected='fight_caves_action_trace', "
            f"actual={trace.get('artifact_type')!r}"
        )
    expected_contract = preflight["contract"]
    trace_contract = trace.get("contract")
    if not isinstance(trace_contract, dict):
        raise ContractError("trace contract is unavailable")
    for field in sorted(set(expected_contract) | set(trace_contract)):
        expected = expected_contract.get(field, "<missing>")
        actual = trace_contract.get(field, "<missing>")
        if actual != expected:
            raise ContractError(
                f"trace contract mismatch for {field}: "
                f"expected={expected!r}, actual={actual!r}"
            )
    expected_identity = preflight["contract_identity"]
    if trace.get("contract_identity") != expected_identity:
        raise ContractError(
            "trace contract identity mismatch: "
            f"expected={expected_identity!r}, actual={trace.get('contract_identity')!r}"
        )
    expected_loadout = expected_contract["active_loadout"]
    if trace.get("active_loadout") != expected_loadout:
        raise ContractError(
            "trace active loadout mismatch: "
            f"expected={expected_loadout!r}, actual={trace.get('active_loadout')!r}"
        )
    expected_backend = preflight["backend"]["sha256"]
    expected_config = preflight["config"]["source_sha256"]
    for field, expected in (
        ("backend_sha256", expected_backend),
        ("config_sha256", expected_config),
    ):
        if trace.get(field) != expected:
            raise ContractError(
                f"trace {field} mismatch: expected={expected!r}, actual={trace.get(field)!r}"
            )
    seed = trace.get("seed")
    if not isinstance(seed, int) or isinstance(seed, bool) or not 0 <= seed <= 0xFFFFFFFF:
        raise ContractError(f"trace seed is invalid: expected=uint32, actual={seed!r}")
    ticks = trace.get("ticks")
    if not isinstance(ticks, list):
        raise ContractError(f"trace ticks are invalid: expected=list, actual={type(ticks).__name__}")
    action_dims = expected_contract["core_action_dims"]
    for index, tick in enumerate(ticks):
        if not isinstance(tick, dict):
            raise ContractError(f"trace tick {index} must be a JSON object")
        if tick.get("tick") != index:
            raise ContractError(
                f"trace tick index mismatch: expected={index}, actual={tick.get('tick')!r}"
            )
        actions = tick.get("actions")
        if not isinstance(actions, list) or len(actions) != len(action_dims):
            raise ContractError(
                f"trace tick {index} action stride mismatch: "
                f"expected={len(action_dims)}, actual={len(actions) if isinstance(actions, list) else actions!r}"
            )
        for head, (action, dimension) in enumerate(zip(actions, action_dims)):
            if (
                not isinstance(action, int)
                or isinstance(action, bool)
                or not 0 <= action < dimension
            ):
                raise ContractError(
                    f"trace tick {index} action head {head} is invalid: "
                    f"expected=[0,{dimension}), actual={action!r}"
                )
        state_hash = tick.get("state_hash")
        if (
            not isinstance(state_hash, int)
            or isinstance(state_hash, bool)
            or not 0 <= state_hash <= 0xFFFFFFFF
        ):
            raise ContractError(
                f"trace tick {index} state_hash is invalid: "
                f"expected=uint32, actual={state_hash!r}"
            )
    return trace


def backend_source_hash(runescape_dir: str | Path, puffer_dir: str | Path) -> str:
    runescape = Path(runescape_dir).resolve()
    puffer = Path(puffer_dir).resolve()
    candidates: list[Path] = []
    for subtree in (runescape / "fc-core", runescape / "fc-training"):
        if not subtree.is_dir():
            raise ContractError(f"backend source directory is unavailable: {subtree}")
        for path in subtree.rglob("*"):
            if not path.is_file() or "build" in path.parts:
                continue
            if path.suffix in {".c", ".h", ".sh"} or path.name == "CMakeLists.txt":
                candidates.append(path)
    for relative in ("src/vecenv.h", "src/pufferlib.cu", "src/bindings.cu"):
        path = puffer / relative
        if not path.is_file():
            raise ContractError(f"backend dependency source is unavailable: {path}")
        candidates.append(path)

    digest = hashlib.sha256()
    for path in sorted(set(candidates), key=lambda item: str(item)):
        if path.is_relative_to(runescape):
            identity = f"runescape-rl/{path.relative_to(runescape)}"
        else:
            identity = f"pufferlib_4/{path.relative_to(puffer)}"
        digest.update(identity.encode("utf-8"))
        digest.update(b"\0")
        digest.update(path.read_bytes())
        digest.update(b"\0")
    return digest.hexdigest()


def _command_dump(args: argparse.Namespace) -> int:
    contract = load_compiled_contract(args.backend_so)
    print(json.dumps(contract, sort_keys=True, separators=(",", ":")))
    return 0


def _command_config_values(args: argparse.Namespace) -> int:
    values = read_config_contract(args.config_path)
    print(values["observation_version"])
    print(values["action_version"])
    print(values["reward_version"])
    print(PRAYER_TIMING_VERSION)
    return 0


def _command_source_hash(args: argparse.Namespace) -> int:
    print(backend_source_hash(args.runescape_dir, args.puffer_dir))
    return 0


def _command_check(args: argparse.Namespace) -> int:
    payload = build_verified_preflight(
        args.backend_so,
        args.config_path,
        args.synced_config_path,
        args.active_loadout,
    )
    write_json_atomic(args.output_path, payload)
    print(Path(args.output_path).resolve())
    return 0


def _command_prepare_checkpoint_dir(args: argparse.Namespace) -> int:
    preflight = load_preflight_artifact(args.preflight_path)
    contract_dir, marker_path = prepare_checkpoint_directory(
        args.checkpoint_root, preflight
    )
    print(contract_dir)
    print(marker_path)
    return 0


def _command_resolve_checkpoint(args: argparse.Namespace) -> int:
    preflight = load_preflight_artifact(args.preflight_path)
    resolution = resolve_checkpoint(
        args.request,
        args.checkpoint_root,
        preflight,
        checkpoint_path=args.checkpoint_path,
    )
    if args.config_path or args.default_config_path:
        if not args.config_path or not args.default_config_path:
            raise ContractError(
                "checkpoint byte validation requires both --config-path and "
                "--default-config-path"
            )
        expected_bytes = expected_checkpoint_parameter_bytes(
            preflight["contract"], args.config_path, args.default_config_path
        )
        validate_checkpoint_parameter_bytes(
            resolution, expected_bytes, preflight["contract"]
        )
        resolution["expected_parameter_bytes"] = expected_bytes
    write_json_atomic(args.output_path, resolution)
    print(Path(args.output_path).resolve())
    print(resolution["resolved_path"])
    return 0


def _command_validate_trace(args: argparse.Namespace) -> int:
    preflight = load_preflight_artifact(args.preflight_path)
    validate_trace_artifact(args.trace_path, preflight)
    print(Path(args.trace_path).resolve())
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Inspect and validate the compiled Fight Caves contract"
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    dump = subparsers.add_parser("dump", help="print the compiled contract JSON")
    dump.add_argument("--backend-so", required=True)
    dump.set_defaults(handler=_command_dump)

    config_values = subparsers.add_parser(
        "config-values", help="print selected build-time contract identifiers"
    )
    config_values.add_argument("--config-path", required=True)
    config_values.set_defaults(handler=_command_config_values)

    source_hash = subparsers.add_parser(
        "source-hash", help="hash project and linked backend sources"
    )
    source_hash.add_argument("--runescape-dir", required=True)
    source_hash.add_argument("--puffer-dir", required=True)
    source_hash.set_defaults(handler=_command_source_hash)

    check = subparsers.add_parser("check", help="run fail-closed launch preflight")
    check.add_argument("--backend-so", required=True)
    check.add_argument("--config-path", required=True)
    check.add_argument("--synced-config-path", required=True)
    check.add_argument("--active-loadout", required=True)
    check.add_argument("--output-path", required=True)
    check.set_defaults(handler=_command_check)

    prepare_checkpoint = subparsers.add_parser(
        "prepare-checkpoint-dir",
        help="create or validate the active contract checkpoint directory",
    )
    prepare_checkpoint.add_argument("--checkpoint-root", required=True)
    prepare_checkpoint.add_argument("--preflight-path", required=True)
    prepare_checkpoint.set_defaults(handler=_command_prepare_checkpoint_dir)

    resolve = subparsers.add_parser(
        "resolve-checkpoint",
        help="resolve an explicit/latest checkpoint through schema-2 metadata",
    )
    resolve.add_argument("--request", required=True, choices=("explicit", "latest"))
    resolve.add_argument("--checkpoint-root", required=True)
    resolve.add_argument("--preflight-path", required=True)
    resolve.add_argument("--checkpoint-path")
    resolve.add_argument("--config-path")
    resolve.add_argument("--default-config-path")
    resolve.add_argument("--output-path", required=True)
    resolve.set_defaults(handler=_command_resolve_checkpoint)

    validate_trace = subparsers.add_parser(
        "validate-trace",
        help="validate action-trace provenance against the active contract",
    )
    validate_trace.add_argument("--trace-path", required=True)
    validate_trace.add_argument("--preflight-path", required=True)
    validate_trace.set_defaults(handler=_command_validate_trace)
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        return args.handler(args)
    except ContractError as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
