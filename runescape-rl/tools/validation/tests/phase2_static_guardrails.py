#!/usr/bin/env python3
"""Static Phase 2 guardrails that should stay outside fc-core."""

from __future__ import annotations

import configparser
from pathlib import Path
import re
import sys


RUNESCAPE_DIR = Path(__file__).resolve().parents[3]


def analytics_no_global_metrics() -> int:
    """Desired state: training analytics are per-env/per-thread, not globals."""
    paths = [
        RUNESCAPE_DIR / "fc-training" / "fight_caves.h",
        RUNESCAPE_DIR / "fc-training" / "binding.c",
    ]
    patterns = [
        re.compile(r"\bfloat\s+g_(?:sum|fires|n_|max|most)"),
        re.compile(r"\bextern\s+float\s+g_"),
        re.compile(r"\bg_n_analytics\b"),
    ]

    hits: list[str] = []
    for path in paths:
        text = path.read_text(encoding="utf-8")
        for lineno, line in enumerate(text.splitlines(), 1):
            if any(pattern.search(line) for pattern in patterns):
                hits.append(f"{path.relative_to(RUNESCAPE_DIR)}:{lineno}: {line.strip()}")

    if hits:
        print("FAIL: global analytics counters still present:")
        for hit in hits[:20]:
            print(f"  {hit}")
        if len(hits) > 20:
            print(f"  ... {len(hits) - 20} more")
        return 1

    print("PASS: analytics counters are no longer global training state")
    return 0


def live_no_supplies_simplified_config() -> int:
    """Live configs should stay on the simplified no-supplies reward/action contract."""
    paths = [
        RUNESCAPE_DIR / "config" / "fight_caves.ini",
        RUNESCAPE_DIR.parent / "pufferlib_4" / "config" / "fight_caves.ini",
    ]
    removed_reward_keys = [
        "shape_food_waste_scale",
        "shape_pot_waste_scale",
        "shape_wrong_prayer_penalty",
        "shape_npc_melee_penalty",
        "shape_wasted_attack_penalty",
        "shape_kiting_reward",
        "shape_kiting_min_dist",
        "shape_kiting_max_dist",
        "shape_safespot_attack_reward",
        "shape_resource_threat_window",
    ]
    zero_reward_keys = [
        "shape_unnecessary_prayer_penalty",
        "shape_wave_stall_start",
        "shape_wave_stall_ramp_interval",
        "shape_wave_stall_base_penalty",
        "shape_wave_stall_cap",
    ]

    failures: list[str] = []
    for path in paths:
        parser = configparser.ConfigParser()
        parser.read(path, encoding="utf-8")
        label = path.relative_to(RUNESCAPE_DIR.parent)

        for key in removed_reward_keys:
            if parser.has_option("env", key):
                failures.append(f"{label}: removed reward key still present: {key}")

        for key in zero_reward_keys:
            if not parser.has_option("env", key):
                failures.append(f"{label}: missing zeroed reward key: {key}")
                continue
            value = parser.getfloat("env", key)
            if abs(value) > 0.000001:
                failures.append(f"{label}: {key}={value}, expected 0")

        for key in ["initial_sharks", "initial_prayer_doses"]:
            if parser.getint("env", key, fallback=-1) != 0:
                failures.append(f"{label}: {key} must be 0")

        if parser.get("run", "action_version", fallback="").strip("'\"") != "fight_caves_multidiscrete_3_head_no_supplies_v1":
            failures.append(f"{label}: action_version is not the 3-head no-supplies contract")
        if parser.get("run", "observation_version", fallback="").strip("'\"") != "fight_caves_puffer_policy_obs_v2_mask_heads_0_2_no_supplies":
            failures.append(f"{label}: observation_version is not the no-supplies mask-heads-0-2 contract")

    if failures:
        print("FAIL: live configs are not on the simplified no-supplies contract:")
        for failure in failures:
            print(f"  {failure}")
        return 1

    print("PASS: live configs use the simplified no-supplies reward/action contract")
    return 0


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print("usage: phase2_static_guardrails.py <analytics_no_global_metrics|live_no_supplies_simplified_config>", file=sys.stderr)
        return 2
    if argv[1] == "analytics_no_global_metrics":
        return analytics_no_global_metrics()
    if argv[1] == "live_no_supplies_simplified_config":
        return live_no_supplies_simplified_config()
    print(f"unknown guardrail: {argv[1]}", file=sys.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
