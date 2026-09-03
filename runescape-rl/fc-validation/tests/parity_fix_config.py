#!/usr/bin/env python3
"""Focused contract-version checks for parity-fix configuration work."""

from __future__ import annotations

import configparser
from pathlib import Path
import sys


RUNESCAPE_DIR = Path(__file__).resolve().parents[2]
WORKSPACE_DIR = RUNESCAPE_DIR.parent
CANONICAL_CONFIG = RUNESCAPE_DIR / "config" / "fight_caves.ini"
PUFFER_MIRROR = WORKSPACE_DIR / "pufferlib_4" / "config" / "fight_caves.ini"

EXPECTED_OBSERVATION_VERSION = (
    "fight_caves_puffer_policy_obs_v9_run_energy_prayer_timing_mask8_no_supplies"
)
EXPECTED_ACTION_VERSION = (
    "fight_caves_multidiscrete_3_head_no_supplies_v4_run_energy_prayer8_stationary_attack_tick"
)
EXPECTED_CANONICAL_REWARD_VERSION = (
    "fight_caves_v4_progress_npc_heal_penalty_m0005_"
    "prayer_snapshot_flick_drain"
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


def test_runtime_mirror_versions() -> int:
    failures = version_failures(PUFFER_MIRROR, EXPECTED_CANONICAL_REWARD_VERSION)
    if not PUFFER_MIRROR.is_file():
        failures.append(f"runtime mirror is missing: {PUFFER_MIRROR}")
    elif PUFFER_MIRROR.read_bytes() != CANONICAL_CONFIG.read_bytes():
        failures.append(
            "Puffer runtime mirror is not byte-identical to canonical config"
        )
    return report("003", failures)


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(
            f"usage: {argv[0]} "
            "<active_versions|runtime_mirror_versions>",
            file=sys.stderr,
        )
        return 2
    cases = {
        "active_versions": test_active_versions,
        "runtime_mirror_versions": test_runtime_mirror_versions,
    }
    case = cases.get(argv[1])
    if case is None:
        print(f"unknown config parity test: {argv[1]}", file=sys.stderr)
        return 2
    return case()


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
