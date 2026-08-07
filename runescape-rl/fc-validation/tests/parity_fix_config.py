#!/usr/bin/env python3
"""Focused contract-version checks for parity-fix configuration work."""

from __future__ import annotations

import configparser
import hashlib
from pathlib import Path
import re
import sys


RUNESCAPE_DIR = Path(__file__).resolve().parents[2]
WORKSPACE_DIR = RUNESCAPE_DIR.parent
CANONICAL_CONFIG = RUNESCAPE_DIR / "config" / "fight_caves.ini"
PUFFER_MIRROR = WORKSPACE_DIR / "pufferlib_4" / "config" / "fight_caves.ini"

EXPECTED_OBSERVATION_VERSION = (
    "fight_caves_puffer_policy_obs_v8_prayer_timing_mask8_no_supplies"
)
EXPECTED_ACTION_VERSION = (
    "fight_caves_multidiscrete_3_head_no_supplies_v2_prayer8"
)
EXPECTED_CANONICAL_REWARD_VERSION = (
    "fight_caves_v4_progress_npc_heal_penalty_m0005_"
    "prayer_snapshot_flick_drain"
)

RUNNABLE_EXPERIMENT_REWARD_VERSIONS = {
    "fight_caves_il0xq0uf_hp_regen_60s_1p5b.ini":
        EXPECTED_CANONICAL_REWARD_VERSION,
    "fight_caves_mmyxbyn4_hp_regen_60s_1p5b.ini":
        EXPECTED_CANONICAL_REWARD_VERSION,
    "fight_caves_v1_mechanics_hparam_sweep_750m.ini": (
        "fight_caves_v38_fc_revamp_step2_raw_work_progress_"
        "prayer_conserve_no_attack_prayer_snapshot_flick_drain"
    ),
    "v3_simple_reward_sweep1.ini": EXPECTED_CANONICAL_REWARD_VERSION,
}

# These hashes normalize the three semantic version lines and map manifest
# schema 2 back to its prior schema-1 value. They prove that the planned
# contract-identifier migration does not silently tune rewards, trainer
# settings, or experiment provenance in the same workstream.
EXPECTED_NONVERSION_HASHES = {
    "config/fight_caves.ini":
        "2e9fcc454cdc9869667bcff9cf374e542b5fd1e64bd679a99d4f0113e2557090",
    "config/experiments/fight_caves_il0xq0uf_hp_regen_60s_1p5b.ini":
        "72cb5ce5ab31bd438fcfae8ccd93928c98c1065a5f18024723746d7615849213",
    "config/experiments/fight_caves_mmyxbyn4_hp_regen_60s_1p5b.ini":
        "343833b08ce9422e0dd2a16165f9b42c8ecb990481530c7affc0d7a2d1ac94e2",
    "config/experiments/fight_caves_v1_mechanics_hparam_sweep_750m.ini":
        "e5ea0e795bff03aa4bf6735f8f8024242a79087c7d6cb127ffd7d74c96bb707e",
    "config/experiments/v3_simple_reward_sweep1.ini":
        "9c4c7d3cbe2904475724cd018f61f94908ae5b083658468fccea6f2ad4a7d6d1",
}

VERSION_LINE_RE = re.compile(
    r"^(\s*(?:observation_version|action_version|reward_version)\s*=).*$",
    flags=re.MULTILINE,
)
MANIFEST_SCHEMA_LINE_RE = re.compile(
    r"^(\s*manifest_schema_version\s*=\s*)\d+\s*$",
    flags=re.MULTILINE,
)


def read_versions(path: Path) -> dict[str, str]:
    parser = configparser.ConfigParser()
    loaded = parser.read(path, encoding="utf-8")
    if not loaded or not parser.has_section("run"):
        raise RuntimeError(f"missing readable [run] section: {path}")
    return {
        key: parser.get("run", key, fallback="").strip("'\"")
        for key in ("observation_version", "action_version", "reward_version")
    }


def version_failures(path: Path, expected_reward: str) -> list[str]:
    try:
        actual = read_versions(path)
    except RuntimeError as exc:
        return [str(exc)]

    expected = {
        "observation_version": EXPECTED_OBSERVATION_VERSION,
        "action_version": EXPECTED_ACTION_VERSION,
        "reward_version": expected_reward,
    }
    label = path.relative_to(WORKSPACE_DIR)
    return [
        f"{label}: {key}={actual[key]!r}, expected {value!r}"
        for key, value in expected.items()
        if actual[key] != value
    ]


def report(name: str, failures: list[str]) -> int:
    if failures:
        print(f"FAIL CONFIG-{name}: parity contract versions are stale:")
        for failure in failures:
            print(f"  {failure}")
        return 1
    print(f"PASS CONFIG-{name}: parity contract versions are exact")
    return 0


def test_active_versions() -> int:
    return report(
        "001",
        version_failures(CANONICAL_CONFIG, EXPECTED_CANONICAL_REWARD_VERSION),
    )


def test_runnable_experiment_versions() -> int:
    experiment_dir = RUNESCAPE_DIR / "config" / "experiments"
    failures: list[str] = []
    actual_names = {path.name for path in experiment_dir.glob("*.ini")}
    expected_names = set(RUNNABLE_EXPERIMENT_REWARD_VERSIONS)
    if actual_names != expected_names:
        failures.append(
            "top-level runnable experiment inventory changed: "
            f"actual={sorted(actual_names)}, expected={sorted(expected_names)}"
        )
    for name, reward_version in RUNNABLE_EXPERIMENT_REWARD_VERSIONS.items():
        failures.extend(version_failures(experiment_dir / name, reward_version))
    return report("002", failures)


def test_runtime_mirror_versions() -> int:
    failures = version_failures(PUFFER_MIRROR, EXPECTED_CANONICAL_REWARD_VERSION)
    if not PUFFER_MIRROR.is_file():
        failures.append(f"runtime mirror is missing: {PUFFER_MIRROR}")
    elif PUFFER_MIRROR.read_bytes() != CANONICAL_CONFIG.read_bytes():
        failures.append(
            "Puffer runtime mirror is not byte-identical to canonical config"
        )
    return report("003", failures)


def test_nonversion_values_preserved() -> int:
    failures: list[str] = []
    for relative, expected_hash in EXPECTED_NONVERSION_HASHES.items():
        path = RUNESCAPE_DIR / relative
        text = path.read_text(encoding="utf-8")
        normalized = VERSION_LINE_RE.sub(r"\1 <CONTRACT_VERSION>", text)
        normalized = MANIFEST_SCHEMA_LINE_RE.sub(r"\g<1>1", normalized)
        actual_hash = hashlib.sha256(normalized.encode("utf-8")).hexdigest()
        if actual_hash != expected_hash:
            failures.append(
                f"{relative}: non-version content hash {actual_hash}, "
                f"expected {expected_hash}"
            )

    if failures:
        print("FAIL CONFIG-004: contract migration changed non-contract config content:")
        for failure in failures:
            print(f"  {failure}")
        return 1
    print("PASS CONFIG-004: canonical and runnable config values are preserved")
    return 0


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(
            f"usage: {argv[0]} "
            "<active_versions|runnable_experiment_versions|"
            "runtime_mirror_versions|nonversion_values_preserved>",
            file=sys.stderr,
        )
        return 2
    cases = {
        "active_versions": test_active_versions,
        "runnable_experiment_versions": test_runnable_experiment_versions,
        "runtime_mirror_versions": test_runtime_mirror_versions,
        "nonversion_values_preserved": test_nonversion_values_preserved,
    }
    case = cases.get(argv[1])
    if case is None:
        print(f"unknown config parity test: {argv[1]}", file=sys.stderr)
        return 2
    return case()


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
