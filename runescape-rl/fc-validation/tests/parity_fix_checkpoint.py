#!/usr/bin/env python3
"""CONTRACT-004 checkpoint, evaluator, manifest, and replay gates."""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time


RUNESCAPE_DIR = Path(__file__).resolve().parents[2]
WORKSPACE_DIR = RUNESCAPE_DIR.parent
PREFLIGHT = RUNESCAPE_DIR / "tools" / "validation" / "contract_preflight.py"
MANIFEST = RUNESCAPE_DIR / "tools" / "validation" / "run_manifest.py"
TRAIN_SH = RUNESCAPE_DIR / "train.sh"
EVALUATOR = RUNESCAPE_DIR / "fc-viewer" / "eval_viewer.py"
CANONICAL_CONFIG = RUNESCAPE_DIR / "config" / "fight_caves.ini"
PUFFER_DEFAULT_CONFIG = WORKSPACE_DIR / "pufferlib_4" / "config" / "default.ini"

OBSERVATION_VERSION = (
    "fight_caves_puffer_policy_obs_v8_prayer_timing_mask8_no_supplies"
)
ACTION_VERSION = "fight_caves_multidiscrete_3_head_no_supplies_v3_prayer8_stationary_attack_tick"
REWARD_VERSION = (
    "fight_caves_v4_progress_npc_heal_penalty_m0005_"
    "prayer_snapshot_flick_drain"
)
PRAYER_TIMING_VERSION = (
    "fight_caves_prayer_timing_v1_tick_start_snapshot_flick_drain_jad_lock"
)
ACTIVE_LOADOUT = "FC_LOADOUT_SOTA_TBOW"

EXPECTED_CONTRACT: dict[str, object] = {
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
    "reward_version": REWARD_VERSION,
    "prayer_timing_version": PRAYER_TIMING_VERSION,
    "state_hash_version": 4,
    "active_loadout": ACTIVE_LOADOUT,
}

# The fake evaluator policy preserves the real 319/{17,9,8}/512x3 topology
# shapes while avoiding CUDA, viewer graphics, and a real training process.
EXPECTED_PARAMETER_FLOATS = (
    512 * 319       # encoder
    + (17 + 9 + 8 + 1) * 512  # fused action/value decoder
    + 3 * 3 * 512 * 512  # three MinGRU 3H-by-H weight matrices
)
EXPECTED_PARAMETER_BYTES = EXPECTED_PARAMETER_FLOATS * 4


def contract_identity(contract: dict[str, object]) -> str:
    encoded = json.dumps(contract, sort_keys=True, separators=(",", ":")).encode()
    return hashlib.sha256(encoded).hexdigest()


def sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(
    command: list[str],
    *,
    env: dict[str, str] | None = None,
    timeout: float = 15.0,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        cwd=RUNESCAPE_DIR,
        env=env,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        timeout=timeout,
    )


def fail(case: str, failures: list[str]) -> int:
    if failures:
        print(f"FAIL CONTRACT-004 {case}:")
        for failure in failures:
            print(f"  {failure}")
        return 1
    print(f"PASS CONTRACT-004 {case}")
    return 0


def fixture_env(fixture_so: Path) -> dict[str, str]:
    env = os.environ.copy()
    env["FC_TEST_CONTRACT_JSON"] = json.dumps(
        EXPECTED_CONTRACT, sort_keys=True, separators=(",", ":")
    )
    env["FC_COMPILED_BACKEND_PATH"] = str(fixture_so)
    env["FC_ACTIVE_LOADOUT"] = ACTIVE_LOADOUT
    return env


def preflight_payload(config_path: Path, backend_path: Path) -> dict[str, object]:
    config_hash = sha256_file(config_path)
    return {
        "preflight_schema_version": 1,
        "contract": dict(EXPECTED_CONTRACT),
        "contract_identity": contract_identity(EXPECTED_CONTRACT),
        "backend": {
            "path": str(backend_path.resolve()),
            "sha256": sha256_file(backend_path),
        },
        "config": {
            "source_path": str(config_path.resolve()),
            "synced_path": str(config_path.resolve()),
            "source_sha256": config_hash,
            "synced_sha256": config_hash,
            "byte_identical": True,
        },
    }


def checkpoint_marker(contract: dict[str, object]) -> dict[str, object]:
    return {
        "checkpoint_contract_schema_version": 2,
        "artifact_type": "fight_caves_checkpoint_directory",
        "contract_identity": contract_identity(contract),
        "contract": contract,
    }


def write_fake_viewer(path: Path) -> None:
    path.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
    path.chmod(0o755)
    future = time.time() + 60.0
    os.utime(path, (future, future))


def write_fake_policy_modules(root: Path) -> None:
    package = root / "pufferlib"
    package.mkdir(parents=True)
    (package / "__init__.py").write_text("", encoding="utf-8")
    (root / "torch.py").write_text(
        """
import numpy as np

class Tensor:
    def __init__(self, value):
        self.value = np.asarray(value, dtype=np.float32)
    @property
    def shape(self):
        return self.value.shape
    def numel(self):
        return int(self.value.size)

def zeros_like(value):
    return Tensor(np.zeros(value.shape, dtype=np.float32))

def from_numpy(value):
    return Tensor(np.asarray(value, dtype=np.float32))
""".lstrip(),
        encoding="utf-8",
    )
    (package / "models.py").write_text(
        """
import os
import numpy as np
import torch

class DefaultEncoder:
    def __init__(self, input_size, hidden_size):
        self.input_size = input_size
        self.hidden_size = hidden_size

class DefaultDecoder:
    def __init__(self, action_dims, hidden_size):
        self.action_dims = action_dims
        self.hidden_size = hidden_size

class MinGRU:
    def __init__(self, **kwargs):
        self.kwargs = kwargs

class Policy:
    def __init__(self, encoder, decoder, network):
        if os.environ.get('FC_TEST_MODEL_CONSTRUCTION_FAILURE') == '1':
            raise RuntimeError('injected model construction failure')
        hidden = encoder.hidden_size
        self._state = {
            'encoder.encoder.weight': torch.Tensor(
                np.zeros((hidden, encoder.input_size), dtype=np.float32)),
            'decoder.decoder.weight': torch.Tensor(
                np.zeros((sum(decoder.action_dims), hidden), dtype=np.float32)),
            'decoder.value_function.weight': torch.Tensor(
                np.zeros((1, hidden), dtype=np.float32)),
            'network.layers.0.weight': torch.Tensor(
                np.zeros((3 * hidden, hidden), dtype=np.float32)),
            'network.layers.1.weight': torch.Tensor(
                np.zeros((3 * hidden, hidden), dtype=np.float32)),
            'network.layers.2.weight': torch.Tensor(
                np.zeros((3 * hidden, hidden), dtype=np.float32)),
        }
    def cpu(self):
        return self
    def state_dict(self):
        return dict(self._state)
    def load_state_dict(self, state):
        self._state = dict(state)
    def eval(self):
        return self
    def initial_state(self, batch_size, device):
        return None
""".lstrip(),
        encoding="utf-8",
    )
    (package / "pufferl.py").write_text(
        """
def load_config(_env_name):
    return {
        'policy': {'hidden_size': 512, 'num_layers': 3, 'expansion_factor': 1},
        'torch': {
            'network': 'MinGRU',
            'encoder': 'DefaultEncoder',
            'decoder': 'DefaultDecoder',
        },
    }
""".lstrip(),
        encoding="utf-8",
    )


def evaluator_env(fixture_so: Path, tmpdir: Path) -> dict[str, str]:
    fake_modules = tmpdir / "fake-modules"
    write_fake_policy_modules(fake_modules)
    viewer = tmpdir / "fake-viewer"
    write_fake_viewer(viewer)
    env = fixture_env(fixture_so)
    env["PUFFERLIB_DIR"] = str(fake_modules)
    env["PYTHONPATH"] = str(fake_modules)
    env["FC_VIEWER_PATH"] = str(viewer)
    env["FC_CHECKPOINT_ROOT"] = str(tmpdir)
    (tmpdir / "contract.json").write_text(
        json.dumps(checkpoint_marker(dict(EXPECTED_CONTRACT)), indent=2) + "\n",
        encoding="utf-8",
    )
    return env


def test_evaluator_explicit_random(fixture_so: Path) -> int:
    failures: list[str] = []
    with tempfile.TemporaryDirectory(prefix="fc-eval-random-") as tmp:
        tmpdir = Path(tmp)
        viewer = tmpdir / "fake-viewer"
        write_fake_viewer(viewer)
        env = fixture_env(fixture_so)
        env["FC_VIEWER_PATH"] = str(viewer)
        proc = run(
            [sys.executable, str(EVALUATOR), "--random", "--episodes", "1"],
            env=env,
        )
        output = f"{proc.stdout}\n{proc.stderr}".lower()
        if proc.returncode != 0:
            failures.append(
                f"explicit --random exited {proc.returncode}: {output.strip()}"
            )
        if "falling back" in output:
            failures.append("explicit --random was reported as an implicit fallback")
    return fail("explicit evaluator random mode", failures)


def test_evaluator_hard_failures(fixture_so: Path) -> int:
    failures: list[str] = []
    with tempfile.TemporaryDirectory(prefix="fc-eval-hard-fail-") as tmp:
        tmpdir = Path(tmp)
        env = evaluator_env(fixture_so, tmpdir)
        expected = tmpdir / "exact.bin"
        expected.write_bytes(b"\0" * EXPECTED_PARAMETER_BYTES)
        malformed = tmpdir / "malformed.bin"
        malformed.write_bytes(b"bad")
        short = tmpdir / "short.bin"
        short.write_bytes(b"\0\0\0\0")
        long_path = tmpdir / "long.bin"
        long_path.write_bytes(b"\0" * (EXPECTED_PARAMETER_BYTES + 4))
        old_contract = tmpdir / "old-contract.bin"
        old_contract.write_bytes(expected.read_bytes())
        stale = dict(EXPECTED_CONTRACT)
        stale["puffer_action_dims"] = [17, 9, 8 - 3]
        stale["puffer_mask_size"] = 34 - 3
        (old_contract.parent / f"{old_contract.name}.contract.json").write_text(
            json.dumps(checkpoint_marker(stale), indent=2) + "\n",
            encoding="utf-8",
        )

        cases = [
            ("missing", tmpdir / "missing.bin", env, "missing"),
            ("malformed", malformed, env, str(malformed.stat().st_size)),
            ("short", short, env, str(short.stat().st_size)),
            ("long/unused", long_path, env, str(long_path.stat().st_size)),
            ("old contract", old_contract, env, "contract"),
        ]
        model_env = dict(env)
        model_env["FC_TEST_MODEL_CONSTRUCTION_FAILURE"] = "1"
        cases.append(("model construction", expected, model_env, "construction"))

        common_tokens = [
            "expected",
            "actual",
            "285",
            "319",
            "17",
            "9",
            "8",
            str(EXPECTED_PARAMETER_BYTES),
            OBSERVATION_VERSION,
            ACTION_VERSION,
            REWARD_VERSION,
            PRAYER_TIMING_VERSION,
        ]
        for label, checkpoint, case_env, actual_token in cases:
            proc = run(
                [
                    sys.executable,
                    str(EVALUATOR),
                    "--ckpt",
                    str(checkpoint),
                    "--episodes",
                    "1",
                ],
                env=case_env,
            )
            output = f"{proc.stdout}\n{proc.stderr}"
            if proc.returncode == 0:
                failures.append(f"{label} checkpoint failure fell through to replay/random")
                continue
            missing_tokens = [token for token in [*common_tokens, actual_token] if token not in output]
            if missing_tokens:
                failures.append(
                    f"{label} diagnostic omitted {missing_tokens!r}: {output.strip()}"
                )
            if "falling back" in output.lower():
                failures.append(f"{label} failure still advertises random fallback")

    return fail("evaluator hard failures", failures)


def resolve_command(
    request: str,
    root: Path,
    preflight_path: Path,
    output_path: Path,
    checkpoint_path: Path | None = None,
) -> list[str]:
    command = [
        sys.executable,
        str(PREFLIGHT),
        "resolve-checkpoint",
        "--request",
        request,
        "--checkpoint-root",
        str(root),
        "--preflight-path",
        str(preflight_path),
        "--output-path",
        str(output_path),
    ]
    if checkpoint_path is not None:
        command.extend(["--checkpoint-path", str(checkpoint_path)])
    return command


def test_latest_checkpoint_filter(fixture_so: Path) -> int:
    failures: list[str] = []
    with tempfile.TemporaryDirectory(prefix="fc-checkpoint-latest-") as tmp:
        tmpdir = Path(tmp)
        config = tmpdir / "fight_caves.ini"
        config.write_bytes(CANONICAL_CONFIG.read_bytes())
        preflight_path = tmpdir / "preflight.json"
        preflight_path.write_text(
            json.dumps(preflight_payload(config, fixture_so), indent=2) + "\n",
            encoding="utf-8",
        )
        root = tmpdir / "checkpoints"
        active_root = root / "contracts" / contract_identity(EXPECTED_CONTRACT)
        active_run = active_root / "fight_caves" / "active-run"
        active_run.mkdir(parents=True)
        active_marker = active_root / "contract.json"
        active_marker.write_text(
            json.dumps(checkpoint_marker(dict(EXPECTED_CONTRACT)), indent=2) + "\n",
            encoding="utf-8",
        )
        compatible = active_run / "0001.bin"
        compatible.write_bytes(b"active-checkpoint")

        stale_contract = dict(EXPECTED_CONTRACT)
        stale_contract["puffer_action_dims"] = [17, 9, 8 - 3]
        stale_contract["puffer_mask_size"] = 34 - 3
        stale_root = root / "contracts" / contract_identity(stale_contract)
        stale_run = stale_root / "fight_caves" / "old-run"
        stale_run.mkdir(parents=True)
        (stale_root / "contract.json").write_text(
            json.dumps(checkpoint_marker(stale_contract), indent=2) + "\n",
            encoding="utf-8",
        )
        stale_newer = stale_run / "9999.bin"
        stale_newer.write_bytes(b"old-contract-checkpoint")

        schema1_root = root / "schema1-compatible"
        schema1_run = schema1_root / "fight_caves" / "pre-parity-run"
        schema1_run.mkdir(parents=True)
        schema1_marker = checkpoint_marker(dict(EXPECTED_CONTRACT))
        schema1_marker["checkpoint_contract_schema_version"] = 1
        (schema1_root / "contract.json").write_text(
            json.dumps(schema1_marker, indent=2) + "\n",
            encoding="utf-8",
        )
        schema1_newest = schema1_run / "schema1-newest.bin"
        schema1_newest.write_bytes(b"schema1-compatible-checkpoint")

        unmarked = root / "unmarked" / "fight_caves" / "unknown-run"
        unmarked.mkdir(parents=True)
        unmarked_newest = unmarked / "99999.bin"
        unmarked_newest.write_bytes(b"unmarked-checkpoint")
        now = time.time()
        os.utime(compatible, (now - 30, now - 30))
        os.utime(stale_newer, (now - 20, now - 20))
        os.utime(unmarked_newest, (now - 10, now - 10))
        os.utime(schema1_newest, (now, now))

        output = tmpdir / "latest-resolution.json"
        proc = run(
            resolve_command("latest", root, preflight_path, output),
            env=fixture_env(fixture_so),
        )
        if proc.returncode != 0:
            failures.append(
                f"contract-filtered latest exited {proc.returncode}: "
                f"{(proc.stderr or proc.stdout).strip()}"
            )
        elif not output.is_file():
            failures.append("contract-filtered latest did not write a resolution artifact")
        else:
            payload = json.loads(output.read_text(encoding="utf-8"))
            expected_fields = {
                "checkpoint_resolution_schema_version": 1,
                "request_mode": "latest",
                "resolved_path": str(compatible.resolve()),
                "file_size": compatible.stat().st_size,
                "checkpoint_sha256": sha256_file(compatible),
                "sidecar_path": str(active_marker.resolve()),
                "sidecar_contract_identity": contract_identity(EXPECTED_CONTRACT),
            }
            for field, expected_value in expected_fields.items():
                if payload.get(field) != expected_value:
                    failures.append(
                        f"latest resolution {field}={payload.get(field)!r}, "
                        f"expected {expected_value!r}"
                    )

        explicit_output = tmpdir / "explicit-resolution.json"
        proc = run(
            resolve_command(
                "explicit", root, preflight_path, explicit_output, compatible
            ),
            env=fixture_env(fixture_so),
        )
        if proc.returncode != 0:
            failures.append(
                f"compatible explicit checkpoint was rejected: "
                f"{(proc.stderr or proc.stdout).strip()}"
            )
        elif json.loads(explicit_output.read_text(encoding="utf-8")).get(
            "resolved_path"
        ) != str(compatible.resolve()):
            failures.append("explicit resolution did not retain the selected checkpoint")

        schema1_output = tmpdir / "schema1-resolution.json"
        proc = run(
            resolve_command(
                "explicit", root, preflight_path, schema1_output, schema1_newest
            ),
            env=fixture_env(fixture_so),
        )
        diagnostic = f"{proc.stdout}\n{proc.stderr}"
        if proc.returncode == 0:
            failures.append("explicit schema-1 checkpoint sidecar was accepted")
        elif "expected=2" not in diagnostic or "actual=1" not in diagnostic:
            failures.append(
                "schema-1 checkpoint rejection omitted expected=2/actual=1: "
                f"{diagnostic.strip()}"
            )

        sized_output = tmpdir / "sized-resolution.json"
        sized_command = resolve_command(
            "explicit", root, preflight_path, sized_output, compatible
        )
        sized_command.extend(
            [
                "--config-path",
                str(CANONICAL_CONFIG),
                "--default-config-path",
                str(PUFFER_DEFAULT_CONFIG),
            ]
        )
        proc = run(sized_command, env=fixture_env(fixture_so))
        diagnostic = f"{proc.stdout}\n{proc.stderr}"
        if proc.returncode == 0:
            failures.append("training checkpoint resolution accepted the wrong raw byte size")
        else:
            required = [
                str(EXPECTED_PARAMETER_BYTES),
                str(compatible.stat().st_size),
                "319",
                "17",
                "9",
                "8",
                OBSERVATION_VERSION,
                ACTION_VERSION,
                REWARD_VERSION,
                PRAYER_TIMING_VERSION,
            ]
            missing = [token for token in required if token not in diagnostic]
            if missing:
                failures.append(
                    f"training size rejection omitted {missing!r}: {diagnostic.strip()}"
                )

        stale_output = tmpdir / "stale-resolution.json"
        proc = run(
            resolve_command(
                "explicit", root, preflight_path, stale_output, stale_newer
            ),
            env=fixture_env(fixture_so),
        )
        diagnostic = f"{proc.stdout}\n{proc.stderr}"
        if proc.returncode == 0:
            failures.append("explicit old-contract checkpoint was accepted")
        elif "expected" not in diagnostic or "actual" not in diagnostic:
            failures.append("old-contract rejection omitted expected/actual identity")

        active_marker.unlink()
        missing_output = tmpdir / "missing-sidecar-resolution.json"
        proc = run(
            resolve_command("latest", root, preflight_path, missing_output),
            env=fixture_env(fixture_so),
        )
        if proc.returncode == 0:
            failures.append("latest accepted checkpoints with no matching schema-2 sidecar")

    return fail("contract-filtered latest", failures)


def write_backend_stamp(path: Path, fixture_so: Path) -> None:
    path.write_text(
        "FC_ACTIVE_LOADOUT=FC_LOADOUT_SOTA_TBOW\n"
        f"FC_OBSERVATION_VERSION={OBSERVATION_VERSION}\n"
        f"FC_ACTION_VERSION={ACTION_VERSION}\n"
        f"FC_REWARD_VERSION={REWARD_VERSION}\n"
        f"FC_PRAYER_TIMING_VERSION={PRAYER_TIMING_VERSION}\n"
        "BUILD_MODE=cuda\n"
        "SOURCE_SHA256=fixture-source-sha256\n"
        "NVCC_ARCH=sm_120\n"
        "NVCC_VERSION=fixture-nvcc\n"
        "CC=gcc\n"
        "CC_VERSION=fixture-gcc\n"
        "CXX=g++\n"
        "CXX_VERSION=fixture-g++\n"
        f"BACKEND_SHA256={sha256_file(fixture_so)}\n",
        encoding="utf-8",
    )


def manifest_command(
    fixture_so: Path,
    tmpdir: Path,
    source: Path,
    synced: Path,
    preflight_path: Path,
    stamp: Path,
    resolution_path: Path,
    request_mode: str,
    output_path: Path,
) -> list[str]:
    return [
        sys.executable,
        str(MANIFEST),
        "--repo-root",
        str(WORKSPACE_DIR),
        "--runescape-dir",
        str(RUNESCAPE_DIR),
        "--puffer-dir",
        str(tmpdir / "puffer"),
        "--config-path",
        str(source),
        "--synced-config-path",
        str(synced),
        "--backend-stamp",
        str(stamp),
        "--backend-path",
        str(fixture_so),
        "--contract-path",
        str(preflight_path),
        "--checkpoint-request-mode",
        request_mode,
        "--checkpoint-resolution-path",
        str(resolution_path),
        "--active-loadout",
        ACTIVE_LOADOUT,
        "--python-bin",
        sys.executable,
        "--mode",
        "train",
        "--output-path",
        str(output_path),
        "--",
        sys.executable,
        "-m",
        "pufferlib.pufferl",
        "train",
        "fight_caves",
    ]


def test_manifest_warm_modes(fixture_so: Path) -> int:
    failures: list[str] = []
    with tempfile.TemporaryDirectory(prefix="fc-manifest-warm-") as tmp:
        tmpdir = Path(tmp)
        source = tmpdir / "source.ini"
        synced = tmpdir / "synced.ini"
        source.write_bytes(CANONICAL_CONFIG.read_bytes())
        synced.write_bytes(source.read_bytes())
        preflight_path = tmpdir / "preflight.json"
        payload = preflight_payload(source, fixture_so)
        payload["config"] = {
            "source_path": str(source.resolve()),
            "synced_path": str(synced.resolve()),
            "source_sha256": sha256_file(source),
            "synced_sha256": sha256_file(synced),
            "byte_identical": True,
        }
        preflight_path.write_text(json.dumps(payload) + "\n", encoding="utf-8")
        puffer_config = tmpdir / "puffer" / "config"
        puffer_config.mkdir(parents=True)
        (puffer_config / "default.ini").write_bytes(PUFFER_DEFAULT_CONFIG.read_bytes())

        checkpoint_root = tmpdir / "checkpoints" / "contracts" / contract_identity(
            EXPECTED_CONTRACT
        )
        checkpoint = checkpoint_root / "fight_caves" / "run" / "0001.bin"
        checkpoint.parent.mkdir(parents=True)
        checkpoint.write_bytes(b"warm-checkpoint")
        marker = checkpoint_root / "contract.json"
        marker.write_text(
            json.dumps(checkpoint_marker(dict(EXPECTED_CONTRACT))) + "\n",
            encoding="utf-8",
        )
        stamp = tmpdir / "backend.env"
        write_backend_stamp(stamp, fixture_so)

        for request_mode in ("explicit", "latest"):
            resolution = tmpdir / f"{request_mode}-resolution.json"
            resolution.write_text(
                json.dumps(
                    {
                        "checkpoint_resolution_schema_version": 1,
                        "request_mode": request_mode,
                        "contract_identity": contract_identity(EXPECTED_CONTRACT),
                        "resolved_path": str(checkpoint.resolve()),
                        "file_size": checkpoint.stat().st_size,
                        "checkpoint_sha256": sha256_file(checkpoint),
                        "sidecar_path": str(marker.resolve()),
                        "sidecar_contract_identity": contract_identity(
                            EXPECTED_CONTRACT
                        ),
                    }
                )
                + "\n",
                encoding="utf-8",
            )
            output = tmpdir / f"{request_mode}-manifest.json"
            proc = run(
                manifest_command(
                    fixture_so,
                    tmpdir,
                    source,
                    synced,
                    preflight_path,
                    stamp,
                    resolution,
                    request_mode,
                    output,
                ),
                env=fixture_env(fixture_so),
            )
            if proc.returncode != 0:
                failures.append(
                    f"{request_mode} schema-2 manifest exited {proc.returncode}: "
                    f"{(proc.stderr or proc.stdout).strip()}"
                )
                continue
            manifest = json.loads(output.read_text(encoding="utf-8"))
            checkpoint_record = manifest.get("checkpoint", {})
            expected_record = {
                "request_mode": request_mode,
                "cold_start": False,
                "resolved_path": str(checkpoint.resolve()),
                "file_size": checkpoint.stat().st_size,
                "sidecar_path": str(marker.resolve()),
                "sidecar_contract_identity": contract_identity(EXPECTED_CONTRACT),
            }
            for field, expected_value in expected_record.items():
                if checkpoint_record.get(field) != expected_value:
                    failures.append(
                        f"{request_mode} manifest checkpoint.{field}="
                        f"{checkpoint_record.get(field)!r}, expected {expected_value!r}"
                    )

        forged = tmpdir / "forged-resolution.json"
        forged_payload = {
            "checkpoint_resolution_schema_version": 1,
            "request_mode": "explicit",
            "contract_identity": contract_identity(EXPECTED_CONTRACT),
            "resolved_path": str(checkpoint.resolve()),
            "file_size": checkpoint.stat().st_size + 1,
            "checkpoint_sha256": sha256_file(checkpoint),
            "sidecar_path": str(marker.resolve()),
            "sidecar_contract_identity": contract_identity(EXPECTED_CONTRACT),
        }
        forged.write_text(json.dumps(forged_payload) + "\n", encoding="utf-8")
        proc = run(
            manifest_command(
                fixture_so,
                tmpdir,
                source,
                synced,
                preflight_path,
                stamp,
                forged,
                "explicit",
                tmpdir / "forged-manifest.json",
            ),
            env=fixture_env(fixture_so),
        )
        if proc.returncode == 0:
            failures.append("manifest accepted forged checkpoint file-size provenance")

    return fail("warm/latest manifest identity", failures)


def test_checkpoint_consumer_order(_fixture_so: Path) -> int:
    failures: list[str] = []
    train = TRAIN_SH.read_text(encoding="utf-8")
    evaluator = EVALUATOR.read_text(encoding="utf-8")

    train_tokens = {
        "compiled preflight": '"$CONTRACT_PREFLIGHT" check',
        "checkpoint resolution": "resolve-checkpoint",
        "command construction": 'CMD=("$PYTHON_BIN"',
        "manifest checkpoint provenance": "--checkpoint-resolution-path",
        "training execution": '"${CMD[@]}"',
    }
    positions = {name: train.find(token) for name, token in train_tokens.items()}
    for name, position in positions.items():
        if position < 0:
            failures.append(f"train.sh omits {name}: {train_tokens[name]}")
    if not failures and not (
        positions["compiled preflight"]
        < positions["command construction"]
        < positions["checkpoint resolution"]
        < positions["manifest checkpoint provenance"]
        < positions["training execution"]
    ):
        failures.append(
            "train.sh must preflight, construct the base command, resolve/validate "
            "the checkpoint, write provenance, and only then execute training"
        )
    if 'CMD+=(--load-model-path "$LOAD_MODEL_PATH")' in train:
        failures.append("train.sh still passes an unresolved checkpoint/latest request")
    if "checkpoint_dir" not in train and "checkpoint-dir" not in train:
        failures.append("train.sh does not scope Puffer saves to a contract directory")

    evaluator_resolution_tokens = ("resolve-checkpoint", "resolve_checkpoint(")
    resolution_positions = [
        evaluator.find(token)
        for token in evaluator_resolution_tokens
        if evaluator.find(token) >= 0
    ]
    if not resolution_positions:
        failures.append("evaluator does not use contract-filtered checkpoint resolution")
    if "find_latest_checkpoint" in evaluator:
        failures.append("evaluator retains unrestricted recursive latest discovery")
    load_position = evaluator.find("pufferlib.models.Policy(")
    resolution_position = min(resolution_positions, default=-1)
    if load_position >= 0 and resolution_position > load_position:
        failures.append("evaluator resolves the checkpoint after model construction")

    return fail("checkpoint consumer ordering", failures)


def main(argv: list[str]) -> int:
    if len(argv) != 3:
        print(
            f"usage: {argv[0]} "
            "<evaluator_explicit_random|evaluator_hard_failures|"
            "latest_checkpoint_filter|manifest_warm_modes|"
            "checkpoint_consumer_order> <fixture-so>",
            file=sys.stderr,
        )
        return 2
    fixture_so = Path(argv[2]).resolve()
    if not fixture_so.is_file():
        print(f"compiled contract fixture is missing: {fixture_so}", file=sys.stderr)
        return 2
    cases = {
        "evaluator_explicit_random": test_evaluator_explicit_random,
        "evaluator_hard_failures": test_evaluator_hard_failures,
        "latest_checkpoint_filter": test_latest_checkpoint_filter,
        "manifest_warm_modes": test_manifest_warm_modes,
        "checkpoint_consumer_order": test_checkpoint_consumer_order,
    }
    case = cases.get(argv[1])
    if case is None:
        print(f"unknown checkpoint-contract case: {argv[1]}", file=sys.stderr)
        return 2
    return case(fixture_so)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
