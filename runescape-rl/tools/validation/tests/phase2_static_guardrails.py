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
    """Live configs should stay on the local-progress no-supplies reward/action contract."""
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
        "w_damage_dealt",
        "w_npc_kill",
        "w_wave_clear",
        "w_jad_kill",
        "w_correct_jad_prayer",
        "shape_unnecessary_prayer_penalty",
        "shape_wave_stall_start",
        "shape_wave_stall_ramp_interval",
        "shape_wave_stall_base_penalty",
        "shape_wave_stall_cap",
        "shape_jad_heal_penalty",
        "shape_npc_heal_penalty",
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

        expected_floats = {
            "w_progress": 1.0,
            "w_damage_taken": -0.25,
            "w_cave_complete": 1.0,
            "w_player_death": -1.0,
            "w_correct_danger_prayer": 0.005,
            "w_tick_penalty": -0.0001,
            "shape_no_progress_penalty_1": -0.001,
            "shape_no_progress_penalty_2": -0.005,
            "shape_no_progress_penalty_3": -0.02,
        }
        for key, expected in expected_floats.items():
            if not parser.has_option("env", key):
                failures.append(f"{label}: missing net-progress key: {key}")
                continue
            value = parser.getfloat("env", key)
            if abs(value - expected) > 0.000001:
                failures.append(f"{label}: {key}={value}, expected {expected}")

        expected_ints = {
            "shape_no_progress_start_1": 800,
            "shape_no_progress_start_2": 1600,
            "shape_no_progress_start_3": 2400,
        }
        for key, expected in expected_ints.items():
            if parser.getint("env", key, fallback=-1) != expected:
                failures.append(f"{label}: {key} must be {expected}")

        if parser.get("run", "action_version", fallback="").strip("'\"") != "fight_caves_multidiscrete_3_head_no_supplies_v1":
            failures.append(f"{label}: action_version is not the 3-head no-supplies contract")
        if parser.get("run", "observation_version", fallback="").strip("'\"") != "fight_caves_puffer_policy_obs_v4_npc_type_progress_mask_heads_0_2_no_supplies":
            failures.append(f"{label}: observation_version is not the v4 NPC-type/progress no-supplies contract")
        if parser.get("run", "reward_version", fallback="").strip("'\"") != "fight_caves_v38_fc_revamp_step2_local_progress_low_prayer":
            failures.append(f"{label}: reward_version is not the fc_revamp step-2 local-progress low-prayer contract")

    if failures:
        print("FAIL: live configs are not on the simplified no-supplies contract:")
        for failure in failures:
            print(f"  {failure}")
        return 1

    print("PASS: live configs use the local-progress no-supplies reward/action contract")
    return 0


def env_log_key_budget() -> int:
    """Fight Caves env logs must stay under PufferLib's fixed Dict capacity."""
    path = RUNESCAPE_DIR / "fc-training" / "binding.c"
    text = path.read_text(encoding="utf-8")
    failures: list[str] = []

    explicit_keys = len(re.findall(r'dict_set\(out,\s*"', text))
    npc_loop_keys = 5 * (9 - 1)  # five per-NPC-type metrics for NPC types 1..8
    reward_total_keys = 17       # FC_CH_COUNT after net-progress reward channels
    puffer_added_keys = 1        # static_vec_log appends "n" after my_log
    total = explicit_keys + npc_loop_keys + reward_total_keys + puffer_added_keys

    if "rwd_keys_fires" in text:
        failures.append("reward fire-count exports are enabled; this exceeds the log budget")
    if total > 128:
        failures.append(f"estimated env log keys={total}, expected <=128")

    if failures:
        print("FAIL: Fight Caves env logs exceed PufferLib fixed Dict budget:")
        for failure in failures:
            print(f"  {failure}")
        return 1

    print(f"PASS: Fight Caves env log key budget is {total}/128")
    return 0


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print("usage: phase2_static_guardrails.py <analytics_no_global_metrics|live_no_supplies_simplified_config|env_log_key_budget>", file=sys.stderr)
        return 2
    if argv[1] == "analytics_no_global_metrics":
        return analytics_no_global_metrics()
    if argv[1] == "live_no_supplies_simplified_config":
        return live_no_supplies_simplified_config()
    if argv[1] == "env_log_key_budget":
        return env_log_key_budget()
    print(f"unknown guardrail: {argv[1]}", file=sys.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
