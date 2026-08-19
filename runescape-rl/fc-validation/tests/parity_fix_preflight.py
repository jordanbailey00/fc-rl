#!/usr/bin/env python3
"""CONTRACT-004 compiled-preflight and manifest-schema regression tests."""

from __future__ import annotations

import configparser
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile


RUNESCAPE_DIR = Path(__file__).resolve().parents[2]
WORKSPACE_DIR = RUNESCAPE_DIR.parent
PREFLIGHT = RUNESCAPE_DIR / "tools" / "validation" / "contract_preflight.py"
MANIFEST = RUNESCAPE_DIR / "tools" / "validation" / "run_manifest.py"
TRAIN_SH = RUNESCAPE_DIR / "train.sh"
EVALUATOR = RUNESCAPE_DIR / "fc-viewer" / "eval_viewer.py"
CANONICAL_CONFIG = RUNESCAPE_DIR / "config" / "fight_caves.ini"
PUFFER_MIRROR = WORKSPACE_DIR / "pufferlib_4" / "config" / "fight_caves.ini"
PUFFER_DEFAULT_CONFIG = WORKSPACE_DIR / "pufferlib_4" / "config" / "default.ini"
COMBINED_SWEEP_CONFIG = (
    RUNESCAPE_DIR
    / "config"
    / "experiments"
    / "fight_caves_v45_value_arch_batch_sweep_1p5b.ini"
)

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

# Narrow test-owned mirror of the exact release contract specified by
# parity_fix_config.md. The production helper must obtain these values from the
# compiled backend; this map is only an independent expected-value oracle.
EXPECTED_CONTRACT = {
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
    "state_hash_version": 2,
    "active_loadout": ACTIVE_LOADOUT,
}

EXPERIMENT_CONFIGS = sorted(
    (RUNESCAPE_DIR / "config" / "experiments").glob("*.ini")
)
SCHEMA_RE = re.compile(
    r"^(\s*manifest_schema_version\s*=\s*)\d+(\s*)$", re.MULTILINE
)


def sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command: list[str], *, env: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        cwd=RUNESCAPE_DIR,
        env=env,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )


def fail(case: str, failures: list[str]) -> int:
    if failures:
        print(f"FAIL CONTRACT-004 {case}:")
        for failure in failures:
            print(f"  {failure}")
        return 1
    print(f"PASS CONTRACT-004 {case}")
    return 0


def schema2_copy(source: Path, destination: Path) -> None:
    text = source.read_text(encoding="utf-8")
    replaced, count = SCHEMA_RE.subn(r"\g<1>2\g<2>", text)
    if count != 1:
        raise RuntimeError(f"expected one manifest_schema_version in {source}, found {count}")
    destination.write_text(replaced, encoding="utf-8")


def config_value(path: Path, key: str) -> str:
    parser = configparser.ConfigParser()
    if not parser.read(path, encoding="utf-8") or not parser.has_section("run"):
        raise RuntimeError(f"missing readable [run] section: {path}")
    return parser.get("run", key).strip("'\"")


def fixture_env(contract: dict[str, object]) -> dict[str, str]:
    env = os.environ.copy()
    env["FC_TEST_CONTRACT_JSON"] = json.dumps(
        contract, separators=(",", ":"), sort_keys=True
    )
    return env


def preflight_command(
    fixture_so: Path,
    source: Path,
    synced: Path,
    output: Path,
) -> list[str]:
    return [
        sys.executable,
        str(PREFLIGHT),
        "check",
        "--backend-so",
        str(fixture_so),
        "--config-path",
        str(source),
        "--synced-config-path",
        str(synced),
        "--active-loadout",
        ACTIVE_LOADOUT,
        "--output-path",
        str(output),
    ]


def test_compiled_preflight(fixture_so: Path) -> int:
    failures: list[str] = []
    if not PREFLIGHT.is_file():
        return fail("compiled preflight", [f"shared helper is missing: {PREFLIGHT}"])

    proc = run(
        [sys.executable, str(PREFLIGHT), "dump", "--backend-so", str(fixture_so)]
    )
    if proc.returncode != 0:
        failures.append(
            f"compiled dump exited {proc.returncode}: {(proc.stderr or proc.stdout).strip()}"
        )
    else:
        try:
            actual = json.loads(proc.stdout)
        except json.JSONDecodeError as exc:
            failures.append(f"compiled dump did not emit JSON: {exc}")
        else:
            if actual != EXPECTED_CONTRACT:
                failures.append(
                    f"compiled dump mismatch: actual={actual!r}, expected={EXPECTED_CONTRACT!r}"
                )

    with tempfile.TemporaryDirectory(prefix="fc-preflight-exact-") as tmp:
        tmpdir = Path(tmp)
        source = tmpdir / "source.ini"
        synced = tmpdir / "synced.ini"
        output = tmpdir / "verified.json"
        schema2_copy(CANONICAL_CONFIG, source)
        synced.write_bytes(source.read_bytes())
        proc = run(preflight_command(fixture_so, source, synced, output))
        if proc.returncode != 0:
            failures.append(
                f"exact contract preflight exited {proc.returncode}: "
                f"{(proc.stderr or proc.stdout).strip()}"
            )
        elif not output.is_file():
            failures.append("exact contract preflight did not write its verified output")
        else:
            payload = json.loads(output.read_text(encoding="utf-8"))
            if payload.get("contract") != EXPECTED_CONTRACT:
                failures.append("verified preflight output did not retain the compiled contract")
            config = payload.get("config", {})
            if config.get("source_sha256") != sha256_file(source):
                failures.append("verified preflight output has the wrong source config hash")
            if config.get("synced_sha256") != sha256_file(synced):
                failures.append("verified preflight output has the wrong synced config hash")
            if config.get("byte_identical") is not True:
                failures.append("verified preflight output did not assert byte identity")

    return fail("compiled preflight", failures)


def test_preflight_rejections(fixture_so: Path) -> int:
    if not PREFLIGHT.is_file():
        return fail("mismatch rejection", [f"shared helper is missing: {PREFLIGHT}"])

    failures: list[str] = []
    mutations: dict[str, object] = {
        "contract_dump_schema_version": 2,
        "policy_obs_size": 286,
        "puffer_obs_size": 320,
        "puffer_action_dims": [17, 9, 5],
        "puffer_mask_size": 31,
        "core_obs_size": 471,
        "core_action_dims": [17, 9, 5, 3, 2, 65, 65],
        "core_action_mask": 166,
        "reward_feature_count": 19,
        "observation_version": "stale_observation_version",
        "action_version": "stale_action_version",
        "reward_version": "stale_reward_version",
        "prayer_timing_version": "stale_prayer_timing_version",
        "state_hash_version": 0,
        "active_loadout": "FC_LOADOUT_BOWFA_CRYSTAL",
    }

    with tempfile.TemporaryDirectory(prefix="fc-preflight-negative-") as tmp:
        tmpdir = Path(tmp)
        source = tmpdir / "source.ini"
        synced = tmpdir / "synced.ini"
        schema2_copy(CANONICAL_CONFIG, source)
        synced.write_bytes(source.read_bytes())

        baseline = run(
            preflight_command(fixture_so, source, synced, tmpdir / "baseline.json"),
            env=fixture_env(EXPECTED_CONTRACT),
        )
        if baseline.returncode != 0:
            return fail(
                "mismatch rejection",
                [
                    "exact fixture must pass before negative cases: "
                    f"{(baseline.stderr or baseline.stdout).strip()}"
                ],
            )

        for field, bad_value in mutations.items():
            contract = dict(EXPECTED_CONTRACT)
            contract[field] = bad_value
            proc = run(
                preflight_command(
                    fixture_so, source, synced, tmpdir / f"bad-{field}.json"
                ),
                env=fixture_env(contract),
            )
            diagnostic = f"{proc.stdout}\n{proc.stderr}"
            if proc.returncode == 0:
                failures.append(f"compiled {field} mutation was accepted")
            elif field not in diagnostic or "expected" not in diagnostic or "actual" not in diagnostic:
                failures.append(
                    f"compiled {field} rejection lacked expected/actual diagnostics: "
                    f"{diagnostic.strip()}"
                )

        for field in ("observation_version", "action_version", "reward_version"):
            mutated = tmpdir / f"bad-config-{field}.ini"
            text = source.read_text(encoding="utf-8")
            text, count = re.subn(
                rf"^(\s*{field}\s*=\s*).*$",
                rf"\g<1>'stale_{field}'",
                text,
                flags=re.MULTILINE,
            )
            if count != 1:
                failures.append(f"test fixture could not mutate config {field}")
                continue
            mutated.write_text(text, encoding="utf-8")
            proc = run(
                preflight_command(
                    fixture_so, mutated, mutated, tmpdir / f"bad-config-{field}.json"
                ),
                env=fixture_env(EXPECTED_CONTRACT),
            )
            diagnostic = f"{proc.stdout}\n{proc.stderr}"
            if proc.returncode == 0:
                failures.append(f"selected config {field} mutation was accepted")
            elif field not in diagnostic or "expected" not in diagnostic or "actual" not in diagnostic:
                failures.append(
                    f"config {field} rejection lacked expected/actual diagnostics: "
                    f"{diagnostic.strip()}"
                )

        mismatched = tmpdir / "one-byte-different.ini"
        mismatched.write_bytes(source.read_bytes() + b"\n")
        proc = run(
            preflight_command(
                fixture_so, source, mismatched, tmpdir / "byte-mismatch.json"
            ),
            env=fixture_env(EXPECTED_CONTRACT),
        )
        diagnostic = f"{proc.stdout}\n{proc.stderr}"
        if proc.returncode == 0:
            failures.append("one-byte source/synced config mismatch was accepted")
        elif "byte" not in diagnostic.lower() or "source_sha256" not in diagnostic or "synced_sha256" not in diagnostic:
            failures.append(
                "byte mismatch rejection lacked both config hashes: "
                f"{diagnostic.strip()}"
            )

    return fail("mismatch rejection", failures)


def verified_payload(source: Path, synced: Path) -> dict[str, object]:
    return {
        "preflight_schema_version": 1,
        "contract": dict(EXPECTED_CONTRACT),
        "config": {
            "source_path": str(source.resolve()),
            "synced_path": str(synced.resolve()),
            "source_sha256": sha256_file(source),
            "synced_sha256": sha256_file(synced),
            "byte_identical": True,
        },
    }


def manifest_command(
    fixture_so: Path,
    tmpdir: Path,
    source: Path,
    synced: Path,
    contract_path: Path,
    stamp: Path,
    output: Path,
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
        str(contract_path),
        "--checkpoint-request-mode",
        "cold",
        "--active-loadout",
        ACTIVE_LOADOUT,
        "--python-bin",
        sys.executable,
        "--mode",
        "train",
        "--output-path",
        str(output),
        "--",
        sys.executable,
        "-m",
        "pufferlib.pufferl",
        "train",
        "fight_caves",
    ]


def test_manifest_schema2(fixture_so: Path) -> int:
    failures: list[str] = []
    with tempfile.TemporaryDirectory(prefix="fc-manifest-schema2-") as tmp:
        tmpdir = Path(tmp)
        source = tmpdir / "source.ini"
        synced = tmpdir / "synced.ini"
        contract_path = tmpdir / "verified-contract.json"
        stamp = tmpdir / "backend.env"
        output = tmpdir / "manifest.json"
        schema2_copy(CANONICAL_CONFIG, source)
        synced.write_bytes(source.read_bytes())
        contract_path.write_text(
            json.dumps(verified_payload(source, synced), indent=2) + "\n",
            encoding="utf-8",
        )
        stamp.write_text(
            "FC_ACTIVE_LOADOUT=FC_LOADOUT_SOTA_TBOW\n"
            f"FC_OBSERVATION_VERSION={OBSERVATION_VERSION}\n"
            f"FC_ACTION_VERSION={ACTION_VERSION}\n"
            f"FC_REWARD_VERSION={REWARD_VERSION}\n"
            f"FC_PRAYER_TIMING_VERSION={PRAYER_TIMING_VERSION}\n"
            "BUILD_MODE=cuda\n"
            "SOURCE_SHA256=fixture-source-sha256\n"
            "NVCC_ARCH=sm_89\n"
            "NVCC_VERSION=fixture-nvcc\n"
            "CC=gcc\n"
            "CC_VERSION=fixture-gcc\n"
            "CXX=g++\n"
            "CXX_VERSION=fixture-g++\n"
            f"BACKEND_SHA256={sha256_file(fixture_so)}\n",
            encoding="utf-8",
        )
        proc = run(
            manifest_command(
                fixture_so, tmpdir, source, synced, contract_path, stamp, output
            )
        )
        if proc.returncode != 0:
            failures.append(
                f"schema-2 manifest writer exited {proc.returncode}: "
                f"{(proc.stderr or proc.stdout).strip()}"
            )
            return fail("manifest schema 2", failures)

        manifest = json.loads(output.read_text(encoding="utf-8"))
        if manifest.get("schema_version") != 2:
            failures.append(f"schema_version={manifest.get('schema_version')!r}, expected 2")
        if manifest.get("contract") != EXPECTED_CONTRACT:
            failures.append("manifest contract does not exactly match verified compiled dump")

        config = manifest.get("config", {})
        if config.get("source_sha256") != sha256_file(source):
            failures.append("manifest source config hash is missing or wrong")
        if config.get("synced_sha256") != sha256_file(synced):
            failures.append("manifest synced config hash is missing or wrong")
        if config.get("byte_identical") is not True:
            failures.append("manifest does not record source/synced byte identity")

        backend = manifest.get("backend", {})
        required_backend = {
            "binary_sha256": sha256_file(fixture_so),
            "source_sha256": "fixture-source-sha256",
            "active_loadout": ACTIVE_LOADOUT,
        }
        for key, expected in required_backend.items():
            if backend.get(key) != expected:
                failures.append(
                    f"manifest backend.{key}={backend.get(key)!r}, expected {expected!r}"
                )
        if not backend.get("build_stamp_sha256"):
            failures.append("manifest backend build/config hash is unavailable")
        compiler = backend.get("compiler", {})
        for key in ("cc", "cc_version", "cxx", "cxx_version", "nvcc_version"):
            if not compiler.get(key):
                failures.append(f"manifest compiler identity omits {key}")
        if not manifest.get("git", {}).get("commit"):
            failures.append("manifest backend/source commit is unavailable")

        checkpoint = manifest.get("checkpoint", {})
        expected_checkpoint = {
            "request_mode": "cold",
            "cold_start": True,
            "resolved_path": None,
            "file_size": None,
            "sidecar_path": None,
            "sidecar_contract_identity": None,
        }
        for key, expected in expected_checkpoint.items():
            if checkpoint.get(key) != expected:
                failures.append(
                    f"manifest checkpoint.{key}={checkpoint.get(key)!r}, "
                    f"expected {expected!r}"
                )

        # Required values are fail-closed: schema 1, omitted contract fields,
        # or incomplete build/compiler provenance may never emit a manifest.
        schema1 = tmpdir / "schema1.ini"
        schema1.write_text(
            SCHEMA_RE.sub(r"\g<1>1\g<2>", source.read_text(encoding="utf-8")),
            encoding="utf-8",
        )
        schema1_contract = tmpdir / "schema1-contract.json"
        schema1_contract.write_text(
            json.dumps(verified_payload(schema1, schema1), indent=2) + "\n",
            encoding="utf-8",
        )
        proc = run(
            manifest_command(
                fixture_so,
                tmpdir,
                schema1,
                schema1,
                schema1_contract,
                stamp,
                tmpdir / "schema1-manifest.json",
            )
        )
        if proc.returncode == 0:
            failures.append("manifest writer accepted manifest_schema_version = 1")

        for field in EXPECTED_CONTRACT:
            payload = verified_payload(source, synced)
            del payload["contract"][field]  # type: ignore[index]
            incomplete = tmpdir / f"missing-{field}.json"
            incomplete.write_text(json.dumps(payload) + "\n", encoding="utf-8")
            proc = run(
                manifest_command(
                    fixture_so,
                    tmpdir,
                    source,
                    synced,
                    incomplete,
                    stamp,
                    tmpdir / f"missing-{field}-manifest.json",
                )
            )
            if proc.returncode == 0:
                failures.append(f"manifest writer accepted missing contract field {field}")

        for stamp_field in ("SOURCE_SHA256", "CC_VERSION"):
            incomplete_stamp = tmpdir / f"missing-{stamp_field}.env"
            incomplete_stamp.write_text(
                "\n".join(
                    line
                    for line in stamp.read_text(encoding="utf-8").splitlines()
                    if not line.startswith(f"{stamp_field}=")
                )
                + "\n",
                encoding="utf-8",
            )
            proc = run(
                manifest_command(
                    fixture_so,
                    tmpdir,
                    source,
                    synced,
                    contract_path,
                    incomplete_stamp,
                    tmpdir / f"missing-{stamp_field}-manifest.json",
                )
            )
            if proc.returncode == 0:
                failures.append(f"manifest writer accepted missing build field {stamp_field}")

    return fail("manifest schema 2", failures)


def test_consumer_order(_fixture_so: Path) -> int:
    failures: list[str] = []
    train = TRAIN_SH.read_text(encoding="utf-8")
    required_train_tokens = {
        "config sync": 'cp "$CONFIG_PATH" "$PUFFER_DIR/config/fight_caves.ini"',
        "compiled preflight": '"$CONTRACT_PREFLIGHT" check',
        "backend build": 'bash "$TRAINING_BUILD_SH"',
        "command construction": 'CMD=("$PYTHON_BIN"',
        "checkpoint use": 'if [ -n "${LOAD_MODEL_PATH:-}" ]',
        "manifest writer": "run_manifest.py",
        "verified contract manifest input": "--contract-path",
    }
    positions: dict[str, int] = {}
    for label, token in required_train_tokens.items():
        position = train.find(token)
        positions[label] = position
        if position < 0:
            failures.append(f"train.sh omits {label}: {token}")

    if not failures:
        if not (
            positions["config sync"]
            < positions["backend build"]
            < positions["compiled preflight"]
            < positions["command construction"]
            < positions["checkpoint use"]
            < positions["manifest writer"]
        ):
            failures.append(
                "train.sh must sync, build, preflight, construct the command, "
                "handle the checkpoint request, then write the manifest"
            )

    evaluator = EVALUATOR.read_text(encoding="utf-8")
    if "contract_preflight" not in evaluator:
        failures.append("evaluator does not consume the shared compiled-contract helper")
    if "parse_header_definitions" in evaluator or "load_contract_dims" in evaluator:
        failures.append("evaluator still derives the training contract by parsing headers")
    model_position = evaluator.find("pufferlib.models.Policy(")
    preflight_position = evaluator.find("contract_preflight")
    if model_position >= 0 and preflight_position >= 0 and preflight_position > model_position:
        failures.append("evaluator compiled preflight occurs after model construction")

    return fail("consumer ordering", failures)


def test_active_configs(fixture_so: Path) -> int:
    failures: list[str] = []
    configs = [CANONICAL_CONFIG, *EXPERIMENT_CONFIGS]
    if not PREFLIGHT.is_file():
        failures.append(f"shared helper is missing: {PREFLIGHT}")

    if PUFFER_MIRROR.read_bytes() != CANONICAL_CONFIG.read_bytes():
        failures.append("canonical and copied Puffer configs are not byte-identical")

    with tempfile.TemporaryDirectory(prefix="fc-active-configs-") as tmp:
        tmpdir = Path(tmp)
        for config in configs:
            try:
                schema = config_value(config, "manifest_schema_version")
                reward = config_value(config, "reward_version")
            except (RuntimeError, configparser.Error, KeyError) as exc:
                failures.append(str(exc))
                continue
            if schema != "2":
                failures.append(
                    f"{config.relative_to(WORKSPACE_DIR)}: "
                    f"manifest_schema_version={schema!r}, expected '2'"
                )
                continue
            if not PREFLIGHT.is_file():
                continue
            contract = dict(EXPECTED_CONTRACT)
            contract["reward_version"] = reward
            synced = tmpdir / config.name
            synced.write_bytes(config.read_bytes())
            proc = run(
                preflight_command(
                    fixture_so,
                    config,
                    synced,
                    tmpdir / f"{config.stem}-verified.json",
                ),
                env=fixture_env(contract),
            )
            if proc.returncode != 0:
                failures.append(
                    f"{config.relative_to(WORKSPACE_DIR)} failed exact preflight: "
                    f"{(proc.stderr or proc.stdout).strip()}"
                )

    return fail("active schema-2 configs", failures)


def test_sweep_launch_preflight(_fixture_so: Path) -> int:
    failures: list[str] = []
    mirror_before = PUFFER_MIRROR.read_bytes()
    command = [
        sys.executable,
        str(PREFLIGHT),
        "validate-sweep-config",
        "--config-path",
        str(COMBINED_SWEEP_CONFIG),
        "--default-config-path",
        str(PUFFER_DEFAULT_CONFIG),
    ]
    proc = run(command)
    if proc.returncode != 0:
        failures.append(
            "intended combined sweep failed launch preflight: "
            f"{(proc.stderr or proc.stdout).strip()}"
        )
    else:
        try:
            payload = json.loads(proc.stdout)
        except json.JSONDecodeError as exc:
            failures.append(f"sweep preflight did not emit JSON: {exc}")
        else:
            expected = {
                "method": "Protein",
                "metric": "jad_kill_rate",
                "goal": "maximize",
                "max_runs": 130,
                "total_timesteps": 1_500_000_000,
                "sweep_only": [
                    "train/vf_coef",
                    "train/vf_clip_coef",
                    "train/max_grad_norm",
                    "train/beta1",
                    "policy/hidden_size",
                    "policy/num_layers",
                    "vec/total_agents",
                ],
            }
            for key, value in expected.items():
                if payload.get(key) != value:
                    failures.append(
                        f"sweep preflight {key}={payload.get(key)!r}, expected {value!r}"
                    )

    canonical = run(
        [
            sys.executable,
            str(PREFLIGHT),
            "validate-sweep-config",
            "--config-path",
            str(CANONICAL_CONFIG),
            "--default-config-path",
            str(PUFFER_DEFAULT_CONFIG),
        ]
    )
    if canonical.returncode == 0:
        failures.append("canonical fight_caves.ini was accepted as a sweep config")

    launch = run(
        [
            str(TRAIN_SH),
            "sweep",
            "--config",
            str(COMBINED_SWEEP_CONFIG.relative_to(RUNESCAPE_DIR)),
            "--tag",
            "sweep-preflight-test",
            "--wandb-group",
            "sweep-preflight-test",
            "--validate-only",
        ],
        env={**os.environ, "PYTHON_BIN": sys.executable},
    )
    if launch.returncode != 0:
        failures.append(
            "train.sh rejected the intended validation-only sweep launch: "
            f"{(launch.stderr or launch.stdout).strip()}"
        )
    elif "Validation-only complete" not in launch.stdout:
        failures.append("train.sh validation-only launch omitted completion evidence")
    if PUFFER_MIRROR.read_bytes() != mirror_before:
        failures.append("validation-only sweep launch modified the Puffer runtime mirror")

    bad_launch = run(
        [
            str(TRAIN_SH),
            "sweep",
            "--config",
            str(CANONICAL_CONFIG),
            "--tag",
            "sweep-preflight-test",
            "--wandb-group",
            "sweep-preflight-test",
            "--validate-only",
        ],
        env={**os.environ, "PYTHON_BIN": sys.executable},
    )
    if bad_launch.returncode == 0:
        failures.append("train.sh accepted canonical fight_caves.ini in sweep mode")

    with tempfile.TemporaryDirectory(prefix="fc-sweep-preflight-") as tmp:
        tmpdir = Path(tmp)
        source_text = COMBINED_SWEEP_CONFIG.read_text(encoding="utf-8")
        cases = {
            "missing-sweep-only.ini": source_text.replace(
                "sweep_only = train/vf_coef, train/vf_clip_coef, "
                "train/max_grad_norm, train/beta1, policy/hidden_size, "
                "policy/num_layers, vec/total_agents\n",
                "",
            ),
            "variable-budget.ini": source_text.replace(
                "sweep_only = ", "sweep_only = train/total_timesteps, ", 1
            ),
        }
        for name, text in cases.items():
            path = tmpdir / name
            path.write_text(text, encoding="utf-8")
            proc = run(
                [
                    sys.executable,
                    str(PREFLIGHT),
                    "validate-sweep-config",
                    "--config-path",
                    str(path),
                    "--default-config-path",
                    str(PUFFER_DEFAULT_CONFIG),
                ]
            )
            if proc.returncode == 0:
                failures.append(f"sweep preflight accepted {name}")

    return fail("sweep launch preflight", failures)


def main(argv: list[str]) -> int:
    if len(argv) != 3:
        print(
            f"usage: {argv[0]} "
            "<compiled_preflight|preflight_rejections|manifest_schema2|"
            "consumer_order|active_configs|sweep_launch_preflight> <fixture-so>",
            file=sys.stderr,
        )
        return 2

    fixture_so = Path(argv[2]).resolve()
    if not fixture_so.is_file():
        print(f"compiled contract fixture is missing: {fixture_so}", file=sys.stderr)
        return 2

    cases = {
        "compiled_preflight": test_compiled_preflight,
        "preflight_rejections": test_preflight_rejections,
        "manifest_schema2": test_manifest_schema2,
        "consumer_order": test_consumer_order,
        "active_configs": test_active_configs,
        "sweep_launch_preflight": test_sweep_launch_preflight,
    }
    case = cases.get(argv[1])
    if case is None:
        print(f"unknown CONTRACT-004 case: {argv[1]}", file=sys.stderr)
        return 2
    return case(fixture_so)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
