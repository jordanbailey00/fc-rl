#!/usr/bin/env python3
"""Run the parity-fix targeted mutation audit in an isolated source snapshot."""

from __future__ import annotations

import argparse
from dataclasses import asdict, dataclass
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import time
from typing import Any


@dataclass(frozen=True)
class Mutation:
    mutation_id: str
    description: str
    relative_path: str
    before: str
    after: str
    tests: tuple[str, ...]
    occurrences: int = 1
    allow_compile_kill: bool = False


MUTATIONS = (
    Mutation(
        "npc_jad_max_swap",
        "swap Jad ranged and magic maximum hits",
        "fc-core/src/fc_npc.c",
        ".melee_max_hit_tenths = 970, .ranged_max_hit_tenths = 970,\n"
        "        .magic_max_hit_tenths = 950,",
        ".melee_max_hit_tenths = 970, .ranged_max_hit_tenths = 950,\n"
        "        .magic_max_hit_tenths = 970,",
        ("parity_npc_001_exact_stat_table", "parity_npc_002_style_maximum_selection"),
    ),
    Mutation(
        "npc_all_styles_attack_level",
        "use melee Attack for every NPC style",
        "fc-core/src/fc_npc.c",
        "case ATTACK_MELEE: return stats->att_level;\n"
        "        case ATTACK_RANGED: return stats->ranged_level;\n"
        "        case ATTACK_MAGIC: return stats->magic_level;",
        "case ATTACK_MELEE: return stats->att_level;\n"
        "        case ATTACK_RANGED: return stats->att_level;\n"
        "        case ATTACK_MAGIC: return stats->att_level;",
        ("parity_npc_003_exact_offensive_rolls", "parity_npc_004_real_dual_style_routing"),
    ),
    Mutation(
        "def_melee_all_crush",
        "map stab and slash defence to crush",
        "fc-core/src/fc_combat.c",
        "case FC_ATTACK_TYPE_STAB:   def_bonus = p->defence_stab; break;\n"
        "        case FC_ATTACK_TYPE_SLASH:  def_bonus = p->defence_slash; break;",
        "case FC_ATTACK_TYPE_STAB:   def_bonus = p->defence_crush; break;\n"
        "        case FC_ATTACK_TYPE_SLASH:  def_bonus = p->defence_crush; break;",
        ("parity_def_001_sota_exact_rolls", "parity_def_002_equipment_field_isolation"),
    ),
    Mutation(
        "def_plus_nine",
        "change the Defence invisible boost from +8 to +9",
        "fc-core/src/fc_combat.c",
        "eff_def = p->defence_level + 8;",
        "eff_def = p->defence_level + 9;",
        ("parity_def_001_sota_exact_rolls", "parity_def_003_skill_isolation_truncation"),
    ),
    Mutation(
        "def_magic_combined_floor",
        "combine the two Magic-defence truncations",
        "fc-core/src/fc_combat.c",
        "eff_def = 3 * p->defence_level / 10 +\n"
        "                  7 * p->magic_level / 10 + 8;",
        "eff_def = (3 * p->defence_level + 7 * p->magic_level) / 10 + 8;",
        ("parity_def_003_skill_isolation_truncation",),
    ),
    Mutation(
        "bowfa_consumes_target_magic",
        "let Bowfa accuracy and damage consume target Magic",
        "fc-core/src/fc_combat.c",
        "fc_crystal_modifiers_bp(p->crystal_piece_mask,\n"
        "                                &accuracy_bp, &damage_bp);\n"
        "        (void)damage_bp;",
        "fc_crystal_modifiers_bp(p->crystal_piece_mask,\n"
        "                                &accuracy_bp, &damage_bp);\n"
        "        if (target) accuracy_bp += fc_tbow_target_magic_level(target);\n"
        "        (void)damage_bp;",
        ("parity_tbow_003_effect_gating", "parity_cry_002_weapon_cross_product"),
    ),
    Mutation(
        "generic_enters_tbow",
        "let generic Ranged weapons enter the TBow branch",
        "fc-core/src/fc_combat.c",
        "p->weapon_kind == FC_WEAPON_TWISTED_BOW && target",
        "p->weapon_kind != FC_WEAPON_BOW_OF_FAERDHINEN && target",
        ("parity_tbow_003_effect_gating",),
        occurrences=2,
    ),
    Mutation(
        "tbow_collapse_staged_integer",
        "collapse TBow's staged integer division",
        "fc-core/src/fc_combat.c",
        "int64_t inner = 3 * magic / 10;\n"
        "    int64_t delta = inner - 100;",
        "int64_t inner = 3 * magic / 10;\n"
        "    int64_t delta = (3 * magic - 1000) / 10;",
        ("parity_tbow_002_exhaustive_integer_oracle",),
    ),
    Mutation(
        "crystal_partial_gets_full_bonus",
        "apply the full crystal bonus for any partial set",
        "fc-core/src/fc_combat.c",
        "int mask = crystal_piece_mask & FC_CRYSTAL_PIECE_ALL;",
        "int mask = (crystal_piece_mask & FC_CRYSTAL_PIECE_ALL)\n"
        "        ? FC_CRYSTAL_PIECE_ALL : 0;",
        ("parity_cry_001_all_piece_masks",),
    ),
    Mutation(
        "crystal_applies_to_all_ranged",
        "apply crystal modifiers to TBow and generic Ranged",
        "fc-core/src/fc_combat.c",
        "} else if (p->weapon_kind == FC_WEAPON_BOW_OF_FAERDHINEN) {",
        "}\n\n    if (p->crystal_piece_mask != 0) {",
        ("parity_cry_002_weapon_cross_product",),
        occurrences=2,
    ),
    Mutation(
        "npc_damage_tenths_range",
        "sample NPC damage from max_hit_tenths + 1",
        "fc-core/src/fc_combat.c",
        "return fc_rng_int(state, final_max_hit_hp + 1) * 10;",
        "return fc_rng_int(state, final_max_hit_hp * 10 + 1);",
        ("parity_dmg_001_exact_raw_roll_mapping", "parity_dmg_006_storage_consumers"),
    ),
    Mutation(
        "player_damage_uniform_one_to_max",
        "make player damage uniform over 1 through max",
        "fc-core/src/fc_combat.c",
        "int rolled_hp = fc_rng_int(state, final_max_hit_hp + 1);\n"
        "    if (rolled_hp == 0) rolled_hp = 1;",
        "int rolled_hp = fc_rng_int(state, final_max_hit_hp) + 1;",
        ("parity_dmg_001_exact_raw_roll_mapping", "parity_dmg_002_independent_rng_oracle"),
    ),
    Mutation(
        "npc_zero_clamped_to_one",
        "clamp an accurate NPC zero to one HP",
        "fc-core/src/fc_combat.c",
        "return fc_rng_int(state, final_max_hit_hp + 1) * 10;",
        "int rolled_hp = fc_rng_int(state, final_max_hit_hp + 1);\n"
        "    if (rolled_hp == 0) rolled_hp = 1;\n"
        "    return rolled_hp * 10;",
        ("parity_dmg_007_minimum_hit_functional",),
    ),
    Mutation(
        "player_zero_reroll",
        "reroll a player zero and consume another RNG draw",
        "fc-core/src/fc_combat.c",
        "if (rolled_hp == 0) rolled_hp = 1;",
        "while (rolled_hp == 0) {\n"
        "        rolled_hp = fc_rng_int(state, final_max_hit_hp + 1);\n"
        "    }",
        ("parity_dmg_002_independent_rng_oracle",),
    ),
    Mutation(
        "ordinary_attack_uses_live_prayer",
        "use live Prayer instead of the tick-start snapshot",
        "fc-core/src/fc_npc.c",
        "queued->prayer_snapshot = p->prayer_at_tick_start;",
        "queued->prayer_snapshot = p->prayer;",
        ("parity_pray_006_tick_start_snapshot",),
        occurrences=2,
    ),
    Mutation(
        "direct_switch_is_free_flick",
        "treat a direct protection-Prayer switch as a free flick",
        "fc-core/src/fc_prayer.c",
        "int performed_flick = transition != NULL &&\n"
        "        transition->explicit_off_then_on &&\n"
        "        transition->off_performed &&\n"
        "        transition->on_succeeded;",
        "int performed_flick = transition != NULL &&\n"
        "        transition->prior_prayer != PRAYER_NONE &&\n"
        "        transition->actual_final_prayer != PRAYER_NONE &&\n"
        "        transition->final_state_changed;",
        ("parity_pray_003_drain_decision_table",),
    ),
    Mutation(
        "prayer_drain_greater_equal",
        "change the Prayer drain threshold from greater-than to greater-or-equal",
        "fc-core/src/fc_prayer.c",
        "while (p->prayer_drain_counter > resistance) {",
        "while (p->prayer_drain_counter >= resistance) {",
        ("parity_pray_004_resistance_persistence",),
    ),
    Mutation(
        "manual_off_resets_fraction",
        "reset the fractional Prayer counter on manual OFF",
        "fc-core/src/fc_prayer.c",
        "result.off_requested = (p->prayer != PRAYER_NONE);\n"
        "            p->prayer = PRAYER_NONE;",
        "result.off_requested = (p->prayer != PRAYER_NONE);\n"
        "            p->prayer = PRAYER_NONE;\n"
        "            p->prayer_drain_counter = 0;",
        ("parity_pray_004_resistance_persistence",),
    ),
    Mutation(
        "flick_resets_fraction",
        "reset the fractional Prayer counter on an explicit flick",
        "fc-core/src/fc_prayer.c",
        "result.explicit_off_then_on = 1;\n"
        "            p->prayer = PRAYER_NONE;",
        "result.explicit_off_then_on = 1;\n"
        "            p->prayer = PRAYER_NONE;\n"
        "            p->prayer_drain_counter = 0;",
        ("parity_pray_004_resistance_persistence", "parity_pray_007_repeated_flick_trace"),
    ),
    Mutation(
        "potion_resets_fraction",
        "reset the fractional Prayer counter on a non-depleting potion",
        "fc-core/src/fc_tick.c",
        "p->current_prayer += restore;\n"
        "        if (p->current_prayer > p->max_prayer) p->current_prayer = p->max_prayer;\n"
        "        p->prayer_doses_remaining--;",
        "p->current_prayer += restore;\n"
        "        if (p->current_prayer > p->max_prayer) p->current_prayer = p->max_prayer;\n"
        "        p->prayer_drain_counter = 0;\n"
        "        p->prayer_doses_remaining--;",
        ("parity_pray_005_centralized_depletion",),
    ),
    Mutation(
        "depletion_preserves_fraction",
        "preserve the fractional Prayer counter after depletion",
        "fc-core/src/fc_prayer.c",
        "    p->prayer_drain_counter = 0;\n}",
        "}",
        ("parity_pray_005_centralized_depletion",),
    ),
    Mutation(
        "tz_kih_bypasses_cleanup",
        "deplete Prayer through Tz-Kih without centralized cleanup",
        "fc-core/src/fc_combat.c",
        "int actual_drain = fc_prayer_apply_loss_tenths(p, drain);",
        "int prayer_before = p->current_prayer;\n"
        "                p->current_prayer -= drain;\n"
        "                if (p->current_prayer < 0) p->current_prayer = 0;\n"
        "                int actual_drain = prayer_before - p->current_prayer;",
        ("parity_pray_005_centralized_depletion", "parity_dmg_008_tz_kih_coupling"),
    ),
    Mutation(
        "jad_post_lock_rewrites_snapshot",
        "let a post-lock Jad action rewrite the locked snapshot",
        "fc-core/src/fc_tick.c",
        "if (!hit->active || hit->prayer_snapshot >= 0 ||\n"
        "            hit->prayer_lock_tick < 0 ||",
        "if (!hit->active || hit->prayer_lock_tick < 0 ||",
        ("parity_pray_008_jad_delayed_lock",),
    ),
    Mutation(
        "jad_minimum_delay_two",
        "queue a minimum-delay Jad hit at two under same-tick aging",
        "fc-core/src/fc_npc.c",
        "if (use_style != ATTACK_MELEE && delay < 3) delay = 3;",
        "if (use_style != ATTACK_MELEE && delay < 2) delay = 2;",
        ("parity_pray_008_jad_delayed_lock",),
    ),
    Mutation(
        "puffer_mask_old_size",
        "leave the Puffer mask at its old 31-value expression",
        "fc-core/include/fc_contracts.h",
        "#define FC_PUFFER_MASK_SIZE     (FC_MOVE_DIM + FC_ATTACK_DIM + FC_PRAYER_DIM)",
        "#define FC_PUFFER_MASK_SIZE     (FC_MOVE_DIM + FC_ATTACK_DIM + 5)",
        ("parity_contract_001_exact_dimensions", "parity_contract_002_puffer_mask_values"),
        allow_compile_kill=True,
    ),
    Mutation(
        "full_mask_old_size",
        "leave the full mask at its old 166-value expression",
        "fc-core/include/fc_contracts.h",
        "#define FC_ACTION_MASK_SIZE     (FC_MOVE_DIM + FC_ATTACK_DIM + FC_PRAYER_DIM + FC_EAT_DIM + FC_DRINK_DIM + FC_MOVE_TARGET_X_DIM + FC_MOVE_TARGET_Y_DIM)  /* 169 */",
        "#define FC_ACTION_MASK_SIZE     (FC_MOVE_DIM + FC_ATTACK_DIM + 5 + FC_EAT_DIM + FC_DRINK_DIM + FC_MOVE_TARGET_X_DIM + FC_MOVE_TARGET_Y_DIM)  /* stale 166 */",
        ("parity_contract_001_exact_dimensions", "parity_contract_003_single_env_canaries"),
        allow_compile_kill=True,
    ),
    Mutation(
        "hash_omits_prayer_counter",
        "omit prayer_drain_counter from the canonical hash",
        "fc-core/src/fc_hash.c",
        "    FC_HASH_I32(player->prayer_drain_counter);\n",
        "",
        ("parity_det_001_field_coverage",),
    ),
    Mutation(
        "hash_omits_pending_lock",
        "omit pending prayer_lock_tick from the canonical hash",
        "fc-core/src/fc_hash.c",
        "    FC_HASH_I32(hit->prayer_lock_tick);\n",
        "",
        ("parity_det_001_field_coverage",),
    ),
    Mutation(
        "hash_omits_route",
        "omit a route coordinate from the canonical hash",
        "fc-core/src/fc_hash.c",
        "        FC_HASH_I32(player->route_x[i]);\n",
        "",
        ("parity_det_001_field_coverage",),
    ),
    Mutation(
        "hash_omits_weapon_kind",
        "omit weapon_kind from the canonical hash",
        "fc-core/src/fc_hash.c",
        "    FC_HASH_I32(player->weapon_kind);\n",
        "",
        ("parity_det_001_field_coverage",),
    ),
    Mutation(
        "replay_omits_reward_runtime",
        "omit reward runtime from the wrapper replay record",
        "fc-validation/tests/parity_fix_replay.c",
        "    print_runtime(output, runtime);\n",
        "    (void)runtime;\n",
        ("parity_det_002_same_version_replay",),
    ),
    Mutation(
        "evaluator_falls_back_random",
        "let evaluator checkpoint/model errors fall back to random behavior",
        "fc-viewer/eval_viewer.py",
        "        except Exception as exc:\n"
        "            print(\n"
        "                checkpoint_diagnostic(\n"
        "                    exc, checkpoint_path, parameter_bytes, contract\n"
        "                ),\n"
        "                file=sys.stderr,\n"
        "            )\n"
        "            return 1",
        "        except Exception as exc:\n"
        "            print(\n"
        "                checkpoint_diagnostic(\n"
        "                    exc, checkpoint_path, parameter_bytes, contract\n"
        "                ),\n"
        "                file=sys.stderr,\n"
        "            )\n"
        "            policy = None",
        ("parity_contract_004_evaluator_hard_failures",),
    ),
    Mutation(
        "latest_considers_incompatible",
        "let latest consider an incompatible pre-parity checkpoint",
        "tools/validation/contract_preflight.py",
        "        except ContractError:\n"
        "            continue\n"
        "        for candidate in marker.parent.rglob(\"*.bin\"):",
        "        except ContractError:\n"
        "            pass\n"
        "        for candidate in marker.parent.rglob(\"*.bin\"):",
        ("parity_contract_004_latest_checkpoint_filter",),
    ),
    Mutation(
        "config_accepts_schema_one",
        "accept a schema-1 parity configuration/manifest request",
        "tools/validation/contract_preflight.py",
        "if values[\"manifest_schema_version\"] != 2:",
        "if values[\"manifest_schema_version\"] not in (1, 2):",
        ("parity_contract_004_preflight_rejections", "parity_contract_004_manifest_schema2"),
    ),
    Mutation(
        "checkpoint_accepts_schema_one",
        "accept a schema-1 checkpoint sidecar",
        "tools/validation/contract_preflight.py",
        "if marker.get(\"checkpoint_contract_schema_version\") != 2:",
        "if marker.get(\"checkpoint_contract_schema_version\") not in (1, 2):",
        ("parity_contract_004_latest_checkpoint_filter",),
    ),
)


class AuditFailure(RuntimeError):
    pass


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    digest.update(path.read_bytes())
    return digest.hexdigest()


def write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(
        json.dumps(payload, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    temporary.replace(path)


class Recorder:
    def __init__(self, raw_dir: Path):
        self.raw_dir = raw_dir
        self.raw_dir.mkdir(parents=True, exist_ok=True)
        self.records: list[dict[str, Any]] = []

    def run(self, label: str, command: list[str]) -> subprocess.CompletedProcess[str]:
        started = time.monotonic()
        completed = subprocess.run(
            command,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        elapsed = time.monotonic() - started
        log = self.raw_dir / f"{len(self.records) + 1:03d}-{label}.log"
        log.write_text(completed.stdout, encoding="utf-8")
        self.records.append(
            {
                "label": label,
                "command": command,
                "exit_code": completed.returncode,
                "elapsed_seconds": elapsed,
                "raw_log": str(log.resolve()),
            }
        )
        return completed


def snapshot_ignore(source: Path):
    def ignore(directory: str, names: list[str]) -> set[str]:
        path = Path(directory).resolve()
        relative = path.relative_to(source)
        ignored = {name for name in names if name == "__pycache__"}
        if relative == Path("."):
            ignored.update(
                name for name in names
                if name.startswith("build") or name in {
                    ".venv", "data", "demo-env", "reference",
                }
            )
        elif relative == Path("fc-validation/tests"):
            ignored.add("baselines")
        elif relative == Path("fc-viewer"):
            ignored.update({"assets", "raylib"})
        elif relative == Path("tools"):
            ignored.update(name for name in names if name not in {"validation", "AGENTS.md"})
        return ignored

    return ignore


def copy_snapshot(source: Path, workspace: Path) -> Path:
    destination = workspace / "runescape-rl"
    shutil.copytree(source, destination, ignore=snapshot_ignore(source))
    source_workspace = source.parent
    puffer_destination = workspace / "pufferlib_4"
    puffer_destination.mkdir()
    for relative in (Path("config"), Path("src")):
        candidate = source_workspace / "pufferlib_4" / relative
        if candidate.is_dir():
            shutil.copytree(candidate, puffer_destination / relative)
    return destination


def replace_exact(path: Path, mutation: Mutation) -> bytes:
    original = path.read_bytes()
    text = original.decode("utf-8")
    count = text.count(mutation.before)
    if count != mutation.occurrences:
        raise AuditFailure(
            f"{mutation.mutation_id}: expected {mutation.occurrences} exact "
            f"source matches in {mutation.relative_path}, found {count}"
        )
    path.write_text(text.replace(mutation.before, mutation.after), encoding="utf-8")
    return original


def test_regex(tests: tuple[str, ...]) -> str:
    return "^(" + "|".join(re.escape(test) for test in tests) + ")$"


def read_test_inventory(recorder: Recorder, build: Path) -> tuple[str, ...]:
    result = recorder.run(
        "baseline-test-inventory",
        ["ctest", "--test-dir", str(build), "--show-only=json-v1"],
    )
    if result.returncode != 0:
        raise AuditFailure("isolated CTest inventory failed")
    try:
        payload = json.loads(result.stdout)
        names = tuple(test["name"] for test in payload["tests"])
    except (json.JSONDecodeError, KeyError, TypeError) as exc:
        raise AuditFailure(f"invalid isolated CTest inventory: {exc}") from exc
    if len(names) != len(set(names)):
        raise AuditFailure("isolated CTest inventory contains duplicate test names")
    return names


def require_exact_selection(
    registered: tuple[str, ...], requested: tuple[str, ...], context: str
) -> None:
    if len(requested) != len(set(requested)):
        raise AuditFailure(f"{context}: requested duplicate CTest names")
    pattern = re.compile(test_regex(requested))
    selected = tuple(name for name in registered if pattern.fullmatch(name))
    if set(selected) != set(requested) or len(selected) != len(requested):
        missing = sorted(set(requested) - set(selected))
        unexpected = sorted(set(selected) - set(requested))
        raise AuditFailure(
            f"{context}: CTest selection mismatch; expected={len(requested)}, "
            f"selected={len(selected)}, missing={missing}, unexpected={unexpected}"
        )


def metadata() -> dict[str, Any]:
    return {
        "platform": platform.platform(),
        "hostname": platform.node(),
        "logical_cpus": os.cpu_count(),
        "python": sys.version,
    }


def selected_mutations(only: list[str]) -> tuple[Mutation, ...]:
    if not only:
        return MUTATIONS
    requested = set(only)
    selected = tuple(m for m in MUTATIONS if m.mutation_id in requested)
    missing = sorted(requested - {m.mutation_id for m in selected})
    if missing:
        raise AuditFailure(f"unknown mutation ids: {', '.join(missing)}")
    return selected


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--artifact", type=Path, required=True)
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--only", action="append", default=[])
    args = parser.parse_args()
    if args.jobs <= 0:
        parser.error("--jobs must be positive")

    source = args.source.resolve()
    artifact = args.artifact.resolve()
    raw_dir = artifact.with_suffix("")
    recorder = Recorder(raw_dir)
    mutations = selected_mutations(args.only)
    original_hashes = {
        relative: sha256_file(source / relative)
        for relative in sorted({m.relative_path for m in mutations})
    }
    payload: dict[str, Any] = {
        "schema": 1,
        "gate": "targeted-mutation-audit",
        "status": "running",
        "source": str(source),
        "metadata": metadata(),
        "source_hashes_before": original_hashes,
        "mutation_count": len(mutations),
        "mutations": [],
    }
    started = time.monotonic()

    try:
        with tempfile.TemporaryDirectory(prefix="fc-parity-mutations-") as temporary:
            workspace = Path(temporary)
            snapshot = copy_snapshot(source, workspace)
            git_init = recorder.run(
                "baseline-git-init",
                ["git", "-C", str(workspace), "init", "--quiet"],
            )
            if git_init.returncode != 0:
                raise AuditFailure("isolated baseline Git initialization failed")
            git_add = recorder.run(
                "baseline-git-add",
                [
                    "git", "-C", str(workspace), "add", "--",
                    "runescape-rl", "pufferlib_4",
                ],
            )
            if git_add.returncode != 0:
                raise AuditFailure("isolated baseline Git staging failed")
            git_commit = recorder.run(
                "baseline-git-commit",
                [
                    "git", "-C", str(workspace),
                    "-c", "user.name=Fight Caves Validation",
                    "-c", "user.email=fight-caves-validation.invalid",
                    "commit", "--quiet", "-m", "mutation audit snapshot",
                ],
            )
            if git_commit.returncode != 0:
                raise AuditFailure("isolated baseline Git commit failed")
            build = workspace / "build"
            configure = recorder.run(
                "baseline-configure",
                [
                    "cmake", "-S", str(snapshot), "-B", str(build),
                    "-DCMAKE_BUILD_TYPE=Release",
                    "-DFC_ACTIVE_LOADOUT=FC_LOADOUT_SOTA_TBOW",
                ],
            )
            if configure.returncode != 0:
                raise AuditFailure("isolated baseline configure failed")
            baseline_build = recorder.run(
                "baseline-build", ["cmake", "--build", str(build), f"-j{args.jobs}"]
            )
            if baseline_build.returncode != 0:
                raise AuditFailure("isolated baseline build failed")

            all_tests = tuple(dict.fromkeys(test for m in mutations for test in m.tests))
            registered_tests = read_test_inventory(recorder, build)
            require_exact_selection(registered_tests, all_tests, "focused baseline")
            for mutation in mutations:
                require_exact_selection(
                    registered_tests, mutation.tests, mutation.mutation_id
                )
            baseline_tests = recorder.run(
                "baseline-focused-tests",
                [
                    "ctest", "--test-dir", str(build), "--output-on-failure",
                    "-R", test_regex(all_tests),
                ],
            )
            if baseline_tests.returncode != 0:
                raise AuditFailure("isolated focused baseline tests are not green")

            for index, mutation in enumerate(mutations, start=1):
                path = snapshot / mutation.relative_path
                original = replace_exact(path, mutation)
                result: dict[str, Any] = {
                    **asdict(mutation),
                    "index": index,
                    "status": "running",
                }
                try:
                    build_result = recorder.run(
                        f"{mutation.mutation_id}-build",
                        ["cmake", "--build", str(build), f"-j{args.jobs}"],
                    )
                    if build_result.returncode != 0:
                        if not mutation.allow_compile_kill:
                            result["status"] = "harness_failure"
                            result["reason"] = "mutation did not compile"
                            payload["mutations"].append(result)
                            raise AuditFailure(
                                f"{mutation.mutation_id}: unexpected compile failure"
                            )
                        result["status"] = "killed_by_compile_guard"
                    else:
                        test_result = recorder.run(
                            f"{mutation.mutation_id}-tests",
                            [
                                "ctest", "--test-dir", str(build),
                                "--output-on-failure", "-R", test_regex(mutation.tests),
                            ],
                        )
                        if test_result.returncode == 0:
                            result["status"] = "survived"
                            result["reason"] = "all focused tests passed"
                        elif "FAIL" not in test_result.stdout:
                            result["status"] = "harness_failure"
                            result["reason"] = "focused test stopped without an assertion failure"
                        else:
                            result["status"] = "killed_by_test"
                    payload["mutations"].append(result)
                finally:
                    path.write_bytes(original)
                    restore = recorder.run(
                        f"{mutation.mutation_id}-restore-build",
                        ["cmake", "--build", str(build), f"-j{args.jobs}"],
                    )
                    if restore.returncode != 0:
                        raise AuditFailure(
                            f"{mutation.mutation_id}: restore rebuild failed"
                        )

                if result["status"] == "survived":
                    raise AuditFailure(
                        f"{mutation.mutation_id}: mutation survived its focused tests"
                    )
                if result["status"] == "harness_failure":
                    raise AuditFailure(
                        f"{mutation.mutation_id}: {result['reason']}"
                    )
                print(
                    f"[{index:02d}/{len(mutations):02d}] {mutation.mutation_id}: "
                    f"{result['status']}"
                )

        payload["status"] = "passed"
    except AuditFailure as exc:
        payload["status"] = "failed"
        payload["error"] = str(exc)

    final_hashes = {
        relative: sha256_file(source / relative) for relative in original_hashes
    }
    payload["source_hashes_after"] = final_hashes
    payload["source_unchanged"] = final_hashes == original_hashes
    payload["elapsed_seconds"] = time.monotonic() - started
    payload["commands"] = recorder.records
    if not payload["source_unchanged"]:
        payload["status"] = "failed"
        payload["error"] = "original source hashes changed during isolated mutation audit"
    write_json(artifact, payload)

    if payload["status"] != "passed":
        print(f"FAIL: {payload['error']}", file=sys.stderr)
        return 1
    print(
        f"PASS targeted mutation audit: {len(mutations)}/{len(mutations)} killed; "
        f"elapsed={payload['elapsed_seconds']:.2f}s artifact={artifact}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
