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
    """The authoritative live config should stay on the no-supplies contract."""
    paths = [
        RUNESCAPE_DIR / "config" / "fight_caves.ini",
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
            "w_progress": 0.001,
            "w_damage_taken": -0.25,
            "w_cave_complete": 1.0,
            "w_player_death": -1.0,
            "w_correct_danger_prayer": 0.005,
            "w_prayer_lost": -0.02,
            "w_invalid_action": -0.1,
            "w_tick_penalty": -0.0001,
            "shape_no_progress_penalty_1": -0.001,
            "shape_no_progress_penalty_2": -0.005,
            "shape_no_progress_penalty_3": -0.02,
            "shape_no_attack_base_penalty": -0.005,
            "shape_no_attack_wave_scale": 0.05,
            "shape_npc_heal_penalty": -0.005,
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
            "shape_no_attack_start": 50,
        }
        for key, expected in expected_ints.items():
            if parser.getint("env", key, fallback=-1) != expected:
                failures.append(f"{label}: {key} must be {expected}")

        if parser.get("run", "action_version", fallback="").strip("'\"") != "fight_caves_multidiscrete_3_head_no_supplies_v3_prayer8_stationary_attack_tick":
            failures.append(f"{label}: action_version is not the v3 stationary-attack-tick contract")
        if parser.get("run", "observation_version", fallback="").strip("'\"") != "fight_caves_puffer_policy_obs_v8_prayer_timing_mask8_no_supplies":
            failures.append(f"{label}: observation_version is not the v8 Prayer-timing/mask8 contract")
        if parser.get("run", "reward_version", fallback="").strip("'\"") != "fight_caves_v4_progress_npc_heal_penalty_m0005_prayer_snapshot_flick_drain":
            failures.append(f"{label}: reward_version is not the v4 Prayer-event contract")

        expected_train = {
            "total_timesteps": 750_000_000,
            "anneal_lr": 0,
            "learning_rate": 0.00207567504650331,
            "ent_coef": 0.000625460620549345,
            "gamma": 0.9991261141073255,
            "gae_lambda": 0.9,
            "horizon": 256,
            "minibatch_size": 32768,
            "replay_ratio": 2.055184291514704,
            "clip_coef": 0.05,
            "vf_coef": 0.9336215311545304,
            "vf_clip_coef": 0.16791546282962394,
            "max_grad_norm": 0.1418276517190492,
            "vtrace_rho_clip": 2.0,
            "vtrace_c_clip": 0.9746667741536915,
            "prio_alpha": 0.9110743956381228,
            "prio_beta0": 0.2258134371255269,
            "beta1": 0.9832670364021693,
            "beta2": 0.9995810484472892,
            "eps": 1e-10,
        }
        for key, expected in expected_train.items():
            value = parser.getfloat("train", key, fallback=float("nan"))
            if abs(value - expected) > 0.000001:
                failures.append(f"{label}: train.{key}={value}, expected {expected}")

        if parser.getint("base", "seed", fallback=-1) != 73:
            failures.append(f"{label}: base.seed must be 73")
        if parser.getint("policy", "hidden_size", fallback=-1) != 512:
            failures.append(f"{label}: policy.hidden_size must be 512")
        if parser.getint("policy", "num_layers", fallback=-1) != 3:
            failures.append(f"{label}: policy.num_layers must be 3")

    if failures:
        print("FAIL: live configs are not on the simplified no-supplies contract:")
        for failure in failures:
            print(f"  {failure}")
        return 1

    print("PASS: live config matches the 1nvvx5qu v4.5 baseline")
    return 0


def env_log_key_budget() -> int:
    """Fight Caves env logs must stay under PufferLib's fixed Dict capacity."""
    path = RUNESCAPE_DIR / "fc-training" / "binding.c"
    text = path.read_text(encoding="utf-8")
    failures: list[str] = []

    explicit_keys = len(re.findall(r'dict_set\(out,\s*"', text))
    npc_loop_keys = 5 * (9 - 1)  # five per-NPC-type metrics for NPC types 1..8
    reward_total_keys = 18       # FC_CH_COUNT minus skipped legacy damage_dealt reward log
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


def native_action_mask_contract() -> int:
    """Native rollout sampling and PPO must consume the dedicated mask."""
    puffer_dir = RUNESCAPE_DIR.parent / "pufferlib_4"
    paths = {
        "binding": RUNESCAPE_DIR / "fc-training" / "binding.c",
        "adapter": RUNESCAPE_DIR / "fc-training" / "fight_caves.h",
        "vecenv": puffer_dir / "src" / "vecenv.h",
        "trainer": puffer_dir / "src" / "pufferlib.cu",
        "manifest": RUNESCAPE_DIR / "tools" / "validation" / "run_manifest.py",
    }
    texts = {name: path.read_text(encoding="utf-8") for name, path in paths.items()}
    required = {
        "binding": ["#define MY_ACTION_MASK FC_PUFFER_MASK_SIZE"],
        "adapter": [
            "unsigned char* action_mask",
            "env->action_mask[i] =",
        ],
        "vecenv": [
            "unsigned char* action_mask",
            "unsigned char* gpu_action_mask",
            "env->action_mask = vec->action_mask",
            "vec->gpu_action_mask + agent_start * MY_ACTION_MASK",
        ],
        "trainer": [
            "PrecisionTensor action_mask",
            "PrecisionTensor mb_action_mask",
            "env.action_mask.data",
            "mask_slice.data, mask_stride",
            ".action_mask = has_action_mask ? graph.mb_action_mask.data : nullptr",
            "rollouts.action_mask.data, src.action_mask.data",
        ],
        "manifest": [
            '"action_masks_enforced_by_trainer": True',
        ],
    }

    failures: list[str] = []
    for name, snippets in required.items():
        for snippet in snippets:
            if snippet not in texts[name]:
                failures.append(f"{paths[name].name}: missing {snippet!r}")

    if failures:
        print("FAIL: native action-mask integration is incomplete:")
        for failure in failures:
            print(f"  {failure}")
        return 1

    print("PASS: native masks are wired through rollout sampling and PPO")
    return 0


def viewer_policy_pipe_matches_puffer_contract() -> int:
    """The eval viewer policy pipe must match the trainer's no-supplies heads."""
    viewer_path = RUNESCAPE_DIR / "fc-viewer" / "src" / "viewer.c"
    eval_path = RUNESCAPE_DIR / "fc-viewer" / "eval_viewer.py"
    binding_path = RUNESCAPE_DIR / "fc-training" / "binding.c"
    contract_path = RUNESCAPE_DIR / "fc-core" / "include" / "fc_contracts.h"
    viewer = viewer_path.read_text(encoding="utf-8")
    eval_py = eval_path.read_text(encoding="utf-8")
    binding = binding_path.read_text(encoding="utf-8")
    contract = contract_path.read_text(encoding="utf-8")
    failures: list[str] = []

    for token in [
        "#define FC_PUFFER_NUM_ATNS",
        "#define FC_PUFFER_ACT_SIZES",
        "#define FC_PUFFER_MASK_SIZE",
        "#define FC_PUFFER_OBS_SIZE",
    ]:
        if token not in contract:
            failures.append(f"fc_contracts.h missing shared Puffer contract token: {token}")

    if "#define ACT_SIZES FC_PUFFER_ACT_SIZES" not in binding:
        failures.append("binding.c must derive ACT_SIZES from FC_PUFFER_ACT_SIZES")
    if re.search(r"#define\s+ACT_SIZES\s+\{\s*\d", binding):
        failures.append("binding.c still hard-codes numeric Puffer action sizes")

    read_fn = re.search(
        r"static int read_policy_actions\(ViewerState\* v\) \{.*?\n\}",
        viewer,
        flags=re.DOTALL,
    )
    write_fn = re.search(
        r"static void write_obs_to_pipe\(ViewerState\* v\) \{.*?\n\}",
        viewer,
        flags=re.DOTALL,
    )
    if not read_fn:
        failures.append("viewer.c: read_policy_actions function not found")
    elif "FC_PUFFER_NUM_ATNS" not in read_fn.group(0):
        failures.append("viewer.c: policy pipe must read FC_PUFFER_NUM_ATNS actions")
    if read_fn and 'scanf("%d %d %d %d %d"' in read_fn.group(0):
        failures.append("viewer.c: policy pipe still reads old eat/drink action heads")

    if not write_fn:
        failures.append("viewer.c: write_obs_to_pipe function not found")
    else:
        body = write_fn.group(0)
        if "FC_PUFFER_MASK_SIZE" not in body:
            failures.append("viewer.c: policy pipe mask must be sized from FC_PUFFER_MASK_SIZE")
        if "FC_EAT_DIM" in body or "FC_DRINK_DIM" in body:
            failures.append("viewer.c: policy pipe still emits supply masks")

    if "contract_preflight" not in eval_py:
        failures.append("eval_viewer.py must consume the shared compiled contract")
    if "puffer_action_dims" not in eval_py:
        failures.append("eval_viewer.py must derive decoder heads from the compiled dump")
    if "parse_header_definitions" in eval_py:
        failures.append("eval_viewer.py must not reproduce contract arithmetic from headers")
    if '"FC_EAT_DIM"' in eval_py or '"FC_DRINK_DIM"' in eval_py:
        failures.append("eval_viewer.py: contract loader still includes supply action heads")
    if "mask_5head" in eval_py or "mask5" in eval_py:
        failures.append("eval_viewer.py: stale 5-head mask naming remains")

    if failures:
        print("FAIL: eval viewer policy pipe does not match the Puffer contract:")
        for failure in failures:
            print(f"  {failure}")
        return 1

    print("PASS: eval viewer policy pipe matches the 3-head Puffer contract")
    return 0


def viewer_training_parity_contract() -> int:
    """Viewer replay should not silently drift from training env setup."""
    viewer_path = RUNESCAPE_DIR / "fc-viewer" / "src" / "viewer.c"
    binding_path = RUNESCAPE_DIR / "fc-training" / "binding.c"
    viewer = viewer_path.read_text(encoding="utf-8")
    binding = binding_path.read_text(encoding="utf-8")
    failures: list[str] = []

    training_keys = set(re.findall(r'dict_get_unsafe\(kwargs,\s*"([^"]+)"\)', binding))
    viewer_keys = set(re.findall(r'strcmp\(key,\s*"([^"]+)"\)', viewer))
    missing_keys = sorted(training_keys - viewer_keys)
    if missing_keys:
        failures.append(
            "viewer.c config parser is missing training env keys: " +
            ", ".join(missing_keys)
        )

    for snippet in [
        "v->initial_sharks = 0;",
        "v->initial_prayer_doses = 0;",
        "fc_reward_runtime_begin_episode(&v->reward_runtime, &v->state);",
        "fc_reward_sync_progress_state(&v->state, &v->reward_runtime);",
    ]:
        if snippet not in viewer:
            failures.append(f"viewer.c missing parity snippet: {snippet}")

    reset_fn = re.search(
        r"static void reset_ep\(ViewerState\* v\) \{.*?\n\}",
        viewer,
        flags=re.DOTALL,
    )
    if not reset_fn:
        failures.append("viewer.c: reset_ep function not found")
    elif "update_reward_breakdown(v);" in reset_fn.group(0):
        failures.append("viewer.c: reset_ep should not advance reward runtime before first action")

    update_calls = len(re.findall(r"\bupdate_reward_breakdown\(", viewer))
    if update_calls != 2:
        failures.append(
            "viewer.c: update_reward_breakdown should appear only as its definition "
            "and the post-fc_step training-parity call"
        )

    if failures:
        print("FAIL: eval viewer replay is stale relative to training:")
        for failure in failures:
            print(f"  {failure}")
        return 1

    print("PASS: eval viewer replay setup matches training env setup")
    return 0


def standalone_prayer_action_range() -> int:
    """The standalone sampler must consume the shared prayer dimension."""
    path = RUNESCAPE_DIR / "fc-training" / "fight_caves.c"
    text = path.read_text(encoding="utf-8")
    assignment = re.search(
        r"env\.actions\[2\]\s*=\s*(.*?);", text, flags=re.DOTALL
    )

    if assignment is None:
        print("FAIL CONTRACT-002: standalone prayer-action assignment was not found")
        return 1

    expression = assignment.group(1)
    if "rand() % FC_PRAYER_DIM" not in expression:
        print(
            "FAIL CONTRACT-002: standalone prayer sampler does not use "
            "[0, FC_PRAYER_DIM)"
        )
        print(f"  {path.relative_to(RUNESCAPE_DIR)}: {expression.strip()}")
        return 1

    print("PASS CONTRACT-002: standalone prayer sampler uses FC_PRAYER_DIM")
    return 0


def parity_stale_literals() -> int:
    """Runnable parity paths must not retain the superseded five-action contract."""
    source_roots = [
        RUNESCAPE_DIR / "fc-core",
        RUNESCAPE_DIR / "fc-training",
        RUNESCAPE_DIR / "fc-viewer" / "src",
        RUNESCAPE_DIR / "fc-viewer" / "eval_viewer.py",
        RUNESCAPE_DIR / "fc-validation" / "tests",
        RUNESCAPE_DIR / "tools" / "validation",
        RUNESCAPE_DIR / "train.sh",
        RUNESCAPE_DIR / "config" / "fight_caves.ini",
        *sorted((RUNESCAPE_DIR / "config" / "experiments").glob("*.ini")),
    ]
    allowed_suffixes = {".c", ".h", ".py", ".sh", ".ini"}
    archived_parts = {"baselines", "temporary", "__pycache__"}
    paths: list[Path] = []
    for root in source_roots:
        candidates = root.rglob("*") if root.is_dir() else [root]
        for path in candidates:
            if not path.is_file() or path.suffix not in allowed_suffixes:
                continue
            if archived_parts.intersection(path.parts):
                continue
            paths.append(path)

    # Build historical strings in pieces so the scanner does not have to
    # exempt its own exact-match definitions.
    old_versions = [
        "fight_caves_multidiscrete_3_head_no_supplies_" + "v1",
        "fight_caves_puffer_policy_obs_" + "v7_npc_prayer_drain_healing_aggro_kill_events_mask_heads_0_2_no_supplies",
        "fight_caves_v3_progress_npc_heal_penalty_" + "m0005",
    ]
    patterns = [
        ("prayer" + "5", re.compile(r"\bprayer" + r"5\b", re.IGNORECASE)),
        (
            "Prayer dimension 5",
            re.compile(
                r"(?:#define|_Static_assert|assert|expected).*"
                r"(?:PRAYER|prayer)[A-Za-z0-9_ ]*(?:DIM|dimension|dims).*"
                r"(?:==|!=|=|,|\[)\s*5\b"
            ),
        ),
        (
            "old mask size",
            re.compile(
                r"(?:#define|_Static_assert|assert|expected).*"
                r"(?:MASK|mask)[A-Za-z0-9_ ]*(?:==|!=|=|,|\[)\s*(?:31|166)\b"
            ),
        ),
        (
            "old observation size",
            re.compile(
                r"(?:#define|_Static_assert|assert|expected).*"
                r"(?:OBS|obs|observation)[A-Za-z0-9_ ]*(?:==|!=|=|,|\[)\s*(?:316|471)\b"
            ),
        ),
        (
            "Prayer modulo five",
            re.compile(r"(?:prayer|actions\s*\[\s*2\s*\]).*%\s*5\b", re.IGNORECASE),
        ),
    ]

    hits: list[str] = []
    for path in sorted(set(paths)):
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        for lineno, line in enumerate(text.splitlines(), 1):
            labels = [label for label, pattern in patterns if pattern.search(line)]
            labels.extend(
                f"superseded version {version!r}"
                for version in old_versions
                if version in line
            )
            for label in labels:
                hits.append(
                    f"{path.relative_to(RUNESCAPE_DIR)}:{lineno}: {label}: {line.strip()}"
                )

    if hits:
        print("FAIL CONTRACT-004: stale parity literals remain in runnable/current paths:")
        for hit in hits:
            print(f"  {hit}")
        return 1

    print("PASS CONTRACT-004: runnable/current paths contain no stale parity literals")
    return 0


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print("usage: phase2_static_guardrails.py <analytics_no_global_metrics|live_no_supplies_simplified_config|env_log_key_budget|native_action_mask_contract|viewer_policy_pipe_matches_puffer_contract|viewer_training_parity_contract|standalone_prayer_action_range|parity_stale_literals>", file=sys.stderr)
        return 2
    if argv[1] == "analytics_no_global_metrics":
        return analytics_no_global_metrics()
    if argv[1] == "live_no_supplies_simplified_config":
        return live_no_supplies_simplified_config()
    if argv[1] == "env_log_key_budget":
        return env_log_key_budget()
    if argv[1] == "native_action_mask_contract":
        return native_action_mask_contract()
    if argv[1] == "viewer_policy_pipe_matches_puffer_contract":
        return viewer_policy_pipe_matches_puffer_contract()
    if argv[1] == "viewer_training_parity_contract":
        return viewer_training_parity_contract()
    if argv[1] == "standalone_prayer_action_range":
        return standalone_prayer_action_range()
    if argv[1] == "parity_stale_literals":
        return parity_stale_literals()
    print(f"unknown guardrail: {argv[1]}", file=sys.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
