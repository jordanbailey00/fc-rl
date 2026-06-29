# Fight Caves Revamp Tracking

This doc tracks the next set of proposed changes for the Fight Caves RL environment. The goal is to improve learning by simplifying the reward contract, removing exploitable incentives, making key game state easier to observe, and reducing invalid-action noise.

We should implement these one by one where possible so training results stay interpretable.

## Step 0: Benchmark

Pre-revamp benchmark:

- W&B run id: `9lqts4k1`
- Purpose: baseline run before any `fc_revamp` implementation work.
- Use this as the main comparison point for each revamp phase below.
- Config: `runescape-rl/config/experiments/fight_caves_phase7_simplified_no_supplies_1b.ini`
- Loadout: `FC_LOADOUT_SOTA_TBOW`
- Supplies: no food, no prayer potions
- Steps: 1.5B requested via `--train.total-timesteps 1500000000`
- Notes: this includes the passive stall/target diagnostics commit, but no
  `fc_revamp` behavior changes.

## Step Run Tracking

Use `9lqts4k1` as the single pre-revamp benchmark. Each implementation below
gets its own run id so comparisons stay one-change-at-a-time.

| Step | Change Tested | W&B Run ID | Steps | Notes |
| --- | --- | --- | --- | --- |
| Step 0 | Pre-`fc_revamp` simplified no-supplies benchmark | `9lqts4k1` | 1.5B | Observation v2, no NPC type one-hot fields. |
| Step 1 | Explicit NPC type one-hot observations | `ql0yis9k` | 1.5B | Observation v3. Same loadout, rewards, actions, no-supplies config, and hparams as benchmark. |
| Step 2 | Kill/progress reward only, no per-hit damage reward, general NPC-heal penalty | `eifkgdut` | 1.5B | Step 2 attempt, tested and rejected. It removed direct damage farming, but made combat feedback too sparse and caused no-target/no-attack stalling. |
| Step 2 | Net required-work progression reward | `8dvpjsfx` | 1.5B | Step 2 attempt, tested and rejected in current scaling. It removed direct damage farming, but the reward scale made shorter early-death episodes score better than longer partial clears. |
| Step 2 | Net required-work progression with restored correct-prayer rewards and nonzero Jad-kill reward | `cfuyizo1` | 1.5B | Tested and rejected as the active path. It improved over pure net-progress, but correct-prayer reward dominated the objective and produced no-target/no-progress prayer farming. |
| Step 2 | Post-`cfuyizo1` low-prayer reward retry | `50xy19bw` | 1.5B | Tested and rejected as the active path. Lowering correct-prayer reward stopped prayer reward domination, but wave/NPC progress regressed and no-target/no-progress behavior remained. |
| Step 2 | Local current-wave progress scaling with low correct-prayer reward | `7fq4f7jf` | 1.5B | Current active Step 2 basis. It fixed most no-target/no-progress stalling versus `50xy19bw`, but final wave still trails the pre-revamp benchmark. |

## Current Step Order

0. Step 0: benchmark run. Completed with run `9lqts4k1`.
1. Step 1: add explicit NPC type one-hot observations. Completed with run
   `ql0yis9k`.
2. Step 2: reward revamp. This is the current active work.
   - The kill-only/no-damage attempt was tested with run `eifkgdut` and
     rejected.
   - The net required-work progression attempt was tested with run `8dvpjsfx`
     and rejected in its current scaling.
   - The net-progress plus restored correct-prayer retry was tested with run
     `cfuyizo1` and rejected as the active path.
   - Post-`cfuyizo1` cleanup zeroed redundant `w_jad_kill` and zeroed
     `w_correct_jad_prayer`.
   - The low-prayer retry was tested with run `50xy19bw` and rejected as the
     active path.
   - Current retry is still Step 2: make net progress use local current-wave
     reward scaling so wave progress is valuable enough without increasing the
     low correct-prayer reward.
3. Step 3: expose prayer decision deadline observations.
4. Step 4: hard-apply action masks safely.
5. Step 5: give the policy cleaner control over target persistence and
   auto-pathing.

## Step 2 Attempt: Replace Per-Hit Reward With NPC Kill Reward

Status: Tested 2026-06-27, but rejected as the active reward path

Current issue:

- Rewarding damage per hit can reward repeated damage even if the NPC does not die.
- This can encourage loops like damaging an NPC that gets healed instead of finishing the wave.
- This likely contributes to the wave-23 slow/stall behavior.

Proposed change:

- Remove direct reward for every damaging hit.
- Reward meaningful outcomes instead:
  - NPC killed
  - wave completed
  - Jad killed
  - cave completed
- Apply a negative reward when healing happens on any NPC, including Yt-MejKot
  healing itself or another NPC.

Implementation notes:

- Active configs set `w_damage_dealt=0.0`, so hits no longer pay scalar reward
  just because damage landed.
- Existing kill, wave-clear, Jad-kill, cave-complete, damage-taken,
  prayer-correctness, invalid-action, and tick rewards remain unchanged.
- Added `shape_npc_heal_penalty=-0.3`, which penalizes each actual NPC heal proc
  that restores HP.
- Set active `shape_jad_heal_penalty=0.0` because the new general heal penalty
  already covers Jad heals; this avoids double-counting one Jad healer proc.
- Added `rwd_npc_heal_total` and `rwd_npc_heal_fires` metrics so the next run
  shows exactly when the new heal penalty fires.
- Added a guardrail test for Yt-MejKot actual healing: the penalty fires only
  after HP is restored, not for a zero-effect heal attempt.

Training result:

- W&B run id: `eifkgdut`
- Compared against benchmark run id: `9lqts4k1` and step-1 run id
  `ql0yis9k`.
- The new reward contract was active: `w_damage_dealt=0.0`,
  `shape_npc_heal_penalty=-0.3`, `shape_jad_heal_penalty=0.0`, and
  `reward_version=fight_caves_v38_fc_revamp_step2_kill_heal_no_damage`.
- The new heal penalty fired: final `rwd_npc_heal_fires=68.30` and
  `rwd_npc_heal_total=-20.49`.
- Stalling was not fixed. The previous damage-farming stall changed into a
  no-target/no-attack stall:
  - Final `wave_reached`: `28.04` benchmark, `28.05` step 1, `21.68` step 2.
  - Final `npcs_slayed`: `107.45` benchmark, `107.83` step 1, `77.09` step 2.
  - Final `episode_length`: `29341.5` benchmark, `46287.7` step 1, `14569.2`
    step 2.
  - Final `max_wave_ticks`: `11124.5` benchmark, `19087.5` step 1, `3478.9`
    step 2. This is lower, but mostly because the agent reached earlier waves
    and failed sooner, not because it cleared the old stall cleanly.
  - Final `no_progress_ticks`: `2156.0` benchmark, `2827.1` step 1, `6974.1`
    step 2.
  - Final no-progress rate by episode length: `7.35%` benchmark, `6.11%`
    step 1, `47.87%` step 2.
  - Final `no_target_ticks`: `160.7` benchmark, `160.6` step 1, `12185.2`
    step 2.
  - Final no-target rate by episode length: `0.55%` benchmark, `0.35%`
    step 1, `83.64%` step 2.
  - Final `action_attack_target_ticks`: `20285.6` benchmark, `31749.7` step 1,
    `1781.3` step 2.
  - Final `invalid_attack`: `132.0` benchmark, `53.1` step 1, `600.0` step 2.
- Direct damage farming was removed:
  - Final `rwd_damage_dealt_total=0.0`.
  - Final total NPC damage fell from `320021.3` benchmark and `517063.8`
    step 1 to `27222.6` step 2.
  - Final Tok-Xil damage fell from `247929.5` benchmark and `463701.9` step 1
    to `10197.0` step 2.
- Interpretation: removing per-hit damage reward did stop the old repeated
  damage incentive, but it made combat feedback too sparse. The agent no longer
  reliably holds or attacks targets, so the dominant failure became disengaged
  no-target stalling instead of in-combat Tok-Xil/Yt-MejKot stalling.
- Decision: do not keep this as the main reward design. Keep useful pieces from
  the implementation where they help the next design:
  - Keep NPC type observations from step 1.
  - Keep actual-healing counters/logging if useful for diagnostics.
  - Disable scalar `shape_npc_heal_penalty` in the next reward design because
    net progress already makes healing bad naturally.
  - Replace kill-only/no-damage reward with a dense net-progression reward.

Expected benefit:

- Removes the incentive to farm repeated damage.
- Makes the agent care more about finishing enemies and waves.
- Avoids prescribing a specific tactic or position.
- Makes NPC healing directly bad for the agent, so letting Yt-MejKot extend a
  wave is punished without forcing a specific target order.

Risk:

- Learning may become sparser because the agent gets less feedback during combat.
- This should be paired with clean scaling so kill/wave rewards are strong enough to learn from.
- Need to ensure the heal penalty is based on actual healing applied, not just a
  heal attempt that restored zero HP.

## Step 2 Attempt: Net Required-Work Progression Reward

Status: Tested 2026-06-27, but rejected in current scaling

Current issue:

- The old damage reward paid the agent for landing hits, even when those hits did
  not move the cave closer to completion.
- The tested kill-only/no-damage reward removed that exploit, but made feedback
  too sparse. The agent stopped attacking often enough and shifted into
  no-target/no-progress behavior.
- Positive prayer reward can become an income source while the agent is being
  attacked, especially if it is not also making wave progress.

Proposed change:

- Reward net cave progress, not gross damage and not only kills.
- Define required work as the HP still required to finish the current objective.
- Compute current wave progress from required work:
  - `current_wave_progress = 1 - required_work_remaining / required_work_at_wave_start`
- Compute cave progress from waves cleared plus current-wave progress:
  - `cave_progress = (waves_cleared + current_wave_progress) / 63`
- Each tick, reward only the change in cave progress:
  - `progress_reward = progress_scale * (cave_progress_after - cave_progress_before)`
- Keep a small HP-loss penalty, a tiny tick cost, and a delayed no-progress
  penalty.
- Keep terminal outcome rewards:
  - cave complete: positive terminal reward
  - player death: negative terminal reward

Required-work rules:

- Normal NPCs count as their current HP.
- Tz-Kek parent counts as current parent HP plus the latent HP of the two small
  Tz-Keks it will spawn. This prevents fake progress when the parent dies.
- Spawned small Tz-Keks count as their current HP.
- Yt-MejKot healing increases required work, so damage that gets healed back is
  not profitable.
- On Jad, count Jad HP as required work.
- Do not count Yt-HurKot healer HP as mandatory Jad-wave work because killing
  Jad completes the cave. If healers restore Jad HP, Jad required work rises
  naturally.

Initial reward settings:

- Set legacy per-hit damage reward to zero.
- Set direct NPC-kill, wave-clear, and Jad-kill bonuses to zero for the first
  trial because progress already pays for kills and wave clearing.
- Set positive correct-prayer rewards to zero for the first trial. Correct
  prayer is still valuable because it prevents damage and death.
- Set scalar NPC-heal and Jad-heal penalties to zero for the first trial because
  healing already creates negative progress.
- Start with delayed no-progress penalties instead of episode truncation. This
  avoids punishing legitimate slow Fight Caves tactics too aggressively.

Progress observations:

- The design should expose progress state to the policy because the new reward
  depends on it.
- Add observation fields for:
  - normalized cave progress
  - normalized current-wave progress
  - normalized required-work remaining for the current wave
  - normalized ticks since positive progress
- These observations do not tell the agent monster mechanics directly. They only
  tell it whether its recent actions are moving the cave closer to completion.

Diagnostics to add before training:

- `required_work_remaining`
- `required_work_start`
- `cave_progress`
- `current_wave_progress`
- `progress_delta`
- `progress_reward`
- `ticks_since_positive_progress`
- `positive_progress_ticks`
- `zero_progress_ticks`
- `negative_progress_ticks`
- `gross_damage_dealt`
- `net_required_work_removed`
- `gross_damage_to_net_progress_ratio`
- `npc_healing_total`
- `mejkot_healing_total`
- `jad_healing_total`
- `preclip_reward`
- `postclip_reward`
- `positive_clip_count`
- `negative_clip_count`

Implementation checklist:

1. Cleanly replace the rejected step 2 active reward path.
   - Keep step 1 NPC type observations enabled.
   - Keep actual-healing counters/logging if useful for diagnostics.
   - Keep the rejected step 2 run `eifkgdut` documented as a failed experiment,
     not as the active baseline.
   - Set scalar `shape_npc_heal_penalty=0.0` in active configs for this trial.
   - Set scalar `shape_jad_heal_penalty=0.0` in active configs for this trial.
   - Set legacy `w_damage_dealt=0.0` in active configs.
   - Set direct `w_npc_kill=0.0`, `w_wave_clear=0.0`, and `w_jad_kill=0.0`
     in active configs for the first progression-reward trial.
   - Set `w_correct_jad_prayer=0.0` and `w_correct_danger_prayer=0.0` in
     active configs for the first progression-reward trial.

2. Add required-work accounting in the validation/reward layer.
   - Add a helper that computes current required work from the current `FcState`.
   - For normal alive wave NPCs, add `current_hp`.
   - For alive Tz-Kek parents, add parent `current_hp` plus two small Tz-Kek
     max-HP values as latent future work.
   - For alive small Tz-Keks, add small Tz-Kek `current_hp`.
   - For wave 63, count only alive Jad `current_hp` as mandatory work.
   - For wave 63, do not count Yt-HurKot healer HP as mandatory work.
   - If the cave is complete, required work is `0.0`.
   - Clamp impossible negative work to `0.0`.

3. Add progression runtime state.
   - Track `required_work_at_wave_start`.
   - Track previous `cave_progress`.
   - Track `ticks_since_positive_progress`.
   - Track positive, zero, and negative progress tick counts for diagnostics.
   - Reset all progression runtime state on environment reset.
   - Recompute `required_work_at_wave_start` whenever a new wave starts.
   - On wave transition, make sure cave progress does not jump backward because
     the next wave spawned.

4. Replace scalar reward composition for the active config.
   - Compute `current_wave_progress` from required work.
   - Compute `cave_progress` from cleared waves plus current-wave progress.
   - Compute `progress_delta = cave_progress_after - cave_progress_before`.
   - Add `progress_reward = progress_scale * progress_delta`.
   - Keep a small damage-taken penalty based on HP lost this tick.
   - Keep a tiny tick cost.
   - Add delayed no-progress penalties based on `ticks_since_positive_progress`.
   - Do not truncate episodes from no-progress in the first trial.
   - Keep cave-complete and player-death terminal rewards.
   - Preserve reward clipping metrics so we can see whether the clamp is hiding
     useful signal.

5. Add progress observations.
   - Add normalized cave progress.
   - Add normalized current-wave progress.
   - Add normalized required-work remaining for the current wave.
   - Add normalized ticks since positive progress.
   - Bump the observation contract version.
   - Update observation-size constants and Python/native binding assumptions.
   - Add guardrail tests proving the new observation fields are present,
     normalized, and reset correctly.

6. Add metrics/logging.
   - Log required work start and remaining.
   - Log cave progress, current-wave progress, and progress delta.
   - Log progress reward separately from damage-taken, tick cost, no-progress,
     completion, and death reward channels.
   - Log positive, zero, and negative progress tick counts.
   - Log gross damage dealt and net required work removed.
   - Log gross-damage-to-net-progress ratio.
   - Log total NPC healing, Yt-MejKot healing, and Jad healing.
   - Log preclip reward, postclip reward, positive clip count, and negative
     clip count.

7. Add validation tests before training.
   - Damaging a normal NPC lowers required work by the damage amount.
   - Healing a normal NPC raises required work by the healed amount.
   - Killing a Tz-Kek parent does not falsely zero required work before the
     small Tz-Keks are handled.
   - Spawned small Tz-Keks count as required work.
   - Damaging Jad lowers required work.
   - Yt-HurKot healer HP does not count as required work on wave 63.
   - Yt-HurKot healing Jad raises required work.
   - Clearing a wave makes current-wave progress reach `1.0`.
   - Cave completion makes cave progress reach `1.0`.
   - Idling/walking with no HP/work change increments the no-progress timer.
   - Positive progress resets the no-progress timer.
   - Negative progress does not reset the no-progress timer.
   - Ordinary nonterminal progress ticks should not repeatedly clip to `+1` or
     `-1`.

8. Training trial setup.
   - Use the same no-supplies Tbow SOTA loadout as the benchmark.
   - Use the same hparams as the pre-revamp benchmark unless explicitly changed.
   - Use the active local-progress reward version name:
     `fight_caves_v38_fc_revamp_step2_local_progress_low_prayer`.
   - Use a new observation version name that includes progress observations.
   - Compare the run against pre-revamp benchmark `9lqts4k1`, step 1
     `ql0yis9k`, and rejected kill-only step 2 `eifkgdut`.
   - Primary success checks: higher wave reached, higher NPCs slain, lower
     no-target rate, lower no-progress rate, better net progress per gross
     damage, and no new reward-clipping problem.

Implementation notes:

- Added required-work accounting to the reward layer.
- Added progression runtime state for required work at wave start, previous
  cave progress, ticks since positive progress, and positive/zero/negative
  progress tick counts.
- Added scalar reward channels for `progress`, `no_progress`, and
  `cave_complete`.
- Active configs now use:
  - `w_progress=1.0`
  - `w_damage_taken=-0.25`
  - `w_tick_penalty=-0.0001`
  - `w_cave_complete=1.0`
  - `w_player_death=-1.0`
  - `w_correct_danger_prayer=0.005`
  - direct damage, NPC kill, wave clear, Jad kill, Jad-prayer, Jad-heal, and
    NPC-heal scalar rewards set to `0.0`
  - delayed no-progress penalties at 800, 1600, and 2400 ticks
- Added four progress observation fields:
  - normalized cave progress
  - normalized current-wave progress
  - normalized required-work remaining
  - normalized ticks since positive progress
- Observation contract version changed to
  `fight_caves_puffer_policy_obs_v4_npc_type_progress_mask_heads_0_2_no_supplies`.
- Puffer-facing observation size changed from 217 to 221 floats.
- Initial reward version was `fight_caves_v38_fc_revamp_step2_net_progress`;
  active retry reward version is
  `fight_caves_v38_fc_revamp_step2_local_progress_low_prayer`.
- Added progress diagnostics for required work, progress delta/reward,
  positive/zero/negative progress ticks, gross damage, net required-work
  removed, healing totals, and preclip/postclip reward counts.
- Added guardrail tests for required-work damage/healing, Tz-Kek latent small
  work, Jad/healer accounting, no-progress timer behavior, reward clip sanity,
  and progress observations.

Validation:

- `cmake --build runescape-rl/build-phase2 --target phase2_guardrails_core phase2_guardrails_training fc_viewer -j2`
- `ctest --test-dir runescape-rl/build-phase2 --output-on-failure`
- Result: 20/20 tests passed after the logging-budget guardrail was added.
- First attempted training run `dlmyzmgs` aborted after epoch 1 with
  `malloc(): invalid size (unsorted)`. Cause was PufferLib's fixed-size log
  dict overflowing after the new progress metrics and reward channels were
  added. Fixed on the Fight Caves side by dropping redundant `rwd_*_fires`
  exports while keeping reward totals and the new progress/NPC diagnostics.
  PufferLib source remains unchanged.
- Post-fix GPU smoke run with W&B disabled completed 2.1M steps and passed the
  previous crash point.

Training result:

- W&B run id: `8dvpjsfx`
- Compared against pre-revamp benchmark `9lqts4k1`, step 1 `ql0yis9k`, and
  rejected kill-only step 2 `eifkgdut`.
- Final result was a major regression:
  - Final `wave_reached`: `28.04` benchmark, `28.05` step 1, `21.68`
    rejected step 2, `10.72` net-progress step 2.
  - Final `npcs_slayed`: `107.45` benchmark, `107.83` step 1, `77.09`
    rejected step 2, `33.33` net-progress step 2.
  - Final `episode_length`: `29341.5` benchmark, `46287.7` step 1,
    `14569.2` rejected step 2, `1095.8` net-progress step 2.
- The run did briefly learn a near-benchmark policy:
  - Best moving average reached `wave_reached=27.86` around 463M steps.
  - That policy had `episode_length=20186.7`, `no_progress_ticks=15283.0`,
    `no_target_ticks=14404.0`, and `attack_when_ready_rate=0.074`.
  - Its `postclip_reward=-8.67`, so PPO had a strong incentive to move away
    from it.
- The final policy died early but had a much better objective value:
  - Final `postclip_reward=-0.31`.
  - Final reward terms were `progress=+1.30`, `player_death=-1.00`,
    `damage_taken=-0.44`, `tick=-0.11`, `no_progress=-0.06`, and
    `invalid_action=-0.02`.
  - This means the current reward made clearing about 10 waves and dying look
    better than surviving longer with slow/no-progress behavior.
- Root cause:
  - `cave_progress` is normalized over all 63 waves, so one fully cleared wave
    is worth only `w_progress / 63 = 8 / 63 = 0.127` reward.
  - The extra progress reward for getting from about wave 11 to about wave 28
    was only about `+2.18`, but the longer attempt accumulated much larger
    no-progress, tick, and damage penalties.
  - Zeroing positive prayer rewards also removed most direct learning signal
    for protection prayer. Final `prayer_switches` fell to `2.4`, with
    `wrong_prayer_hits=106.3` and `no_prayer_hits=113.2`.
- No evidence that the required-work accounting itself broke:
  - Final `progress_reward=1.298` matches `w_progress * cave_progress`
    (`8 * 0.162`).
  - `gross_damage_to_net_progress_ratio=1.28` final and `1.18` at best wave,
    so repeated healed-damage farming was not the dominant behavior.
  - Final `npc_healing_total=3.69`, so healing was not the final regression
    driver.
- Decision:
  - Keep the diagnostics and required-work accounting as useful tooling.
  - Do not keep this reward scaling as the active training objective.
  - Next attempt should make wave/local progress much more valuable than early
    death, and should not remove all prayer/attack feedback before the policy
    has learned reliable survival and combat.

Retry training result:

- W&B run id: `cfuyizo1`
- Change from `8dvpjsfx`: restored `w_correct_jad_prayer=1.5` and
  `w_correct_danger_prayer=0.25`; set `w_jad_kill=1.0`.
- Compared against `8dvpjsfx`, this fixed the worst early-death collapse:
  - Final `wave_reached`: `10.72` to `23.54`.
  - Final `npcs_slayed`: `33.33` to `85.84`.
  - Final `episode_length`: `1095.8` to `13212.8`.
- It still regressed versus the benchmark and step 1:
  - Benchmark `wave_reached=28.04`; retry `wave_reached=23.54`.
  - Benchmark `npcs_slayed=107.45`; retry `npcs_slayed=85.84`.
- The retry created a new objective problem:
  - Final `rwd_correct_danger_prayer_total=86.87`.
  - Final `postclip_reward=63.97` despite the agent still dying every episode.
  - Final `rwd_progress_total=2.92`, so prayer reward dominated actual cave
    progress by about 30x.
- The policy behavior was still bad:
  - Final `no_target_ticks=10982.2`, or `83.1%` of the episode.
  - Final `no_progress_ticks=9749.5`, or `73.8%` of the episode.
  - Final `action_attack_target_ticks=1377.2`, or `10.4%` of the episode.
  - Final `action_prayer_cmd_ticks=10593.8`, or `80.2%` of the episode.
  - Final `no_progress_prayer_cmd_ticks=7748.6`, so most no-progress time was
    spent issuing prayer commands.
- Jad-kill reward had no measured effect:
  - `reached_wave_63=0`
  - `jad_kill_rate=0`
  - `rwd_jad_kill_total=0`
  - `dmg_to_tztok_jad=0`
- Decision:
  - Do not keep unconditional correct-prayer reward in this net-progress setup.
  - A positive prayer signal can help survival, but if it pays while the agent is
    not progressing, it becomes the main reward source.
  - Step 2 still needs a reward objective where fighting/progress dominates
    prayer income.

Post-run config cleanup:

- `w_jad_kill` and `w_cave_complete` are redundant in this backend because Jad
  death immediately sets `TERMINAL_CAVE_COMPLETE`.
- Kept `w_cave_complete=1.0` because cave completion is the final objective.
- Set `w_jad_kill=0.0`.
- Set `w_correct_jad_prayer=0.0`.
- Set `w_correct_danger_prayer=0.005` for the next Step 2 retry.

Low-prayer retry result:

- W&B run id: `50xy19bw`
- Config: `w_correct_danger_prayer=0.005`, `w_correct_jad_prayer=0.0`,
  `w_jad_kill=0.0`, `w_cave_complete=1.0`.
- Correct-prayer reward no longer dominated:
  - `rwd_correct_danger_prayer_total=0.39`
  - `rwd_progress_total=2.30`
  - `postclip_reward=-0.73`
- The training result still regressed:
  - Final `wave_reached=18.69`
  - Final `npcs_slayed=65.89`
  - Final `episode_length=5594.2`
  - Best moving average reached only `wave_reached=25.07`, then fell back.
- The failure mode remained no-target/no-progress behavior:
  - Final `no_target_ticks=4143.1`, or `74.1%` of the episode.
  - Final `no_progress_ticks=3963.1`, or `70.8%` of the episode.
  - Final `action_attack_target_ticks=826.2`, or `14.8%` of the episode.
  - Final `attack_when_ready_rate=0.25`.
- Decision:
  - `w_correct_danger_prayer=0.005` solved prayer reward domination, but did
    not solve Step 2.
  - The remaining issue is that the active reward objective still does not make
    target acquisition, attacking, and wave progress valuable enough.
  - Step 2 likely needs a stronger combat/progress structure, such as local wave
    progress, small kill/wave milestones, or progress-gated prayer reward.

Local progress-scaling retry:

- Status: Tested 2026-06-28 with run `7fq4f7jf`.
- Keep the low correct-prayer reward from run `50xy19bw`:
  `w_correct_danger_prayer=0.005`.
- Keep Jad prayer and Jad-kill reward at zero:
  `w_correct_jad_prayer=0.0` and `w_jad_kill=0.0`.
- Keep cave completion reward as the terminal success reward:
  `w_cave_complete=1.0`.
- Keep the same progress observations and diagnostics from the net-progress
  implementation.
- Fix progress scaling by making the scalar reward use local current-wave
  progress instead of cave progress divided across all 63 waves:
  - Previous scalar reward: `progress_reward = cave_progress_delta * 8.0`.
  - Previous full-wave reward: `8 / 63 = 0.127`.
  - New scalar reward: `progress_reward = cave_progress_delta * 63 * 1.0`.
  - New full-wave reward: about `1.0`.
- The logged `progress_delta` remains cave-normalized for continuity with prior
  runs. The reward channel converts it to local wave scale internally.
- Reward version:
  `fight_caves_v38_fc_revamp_step2_local_progress_low_prayer`.
- Hypothesis: this should make clearing additional waves and finishing NPC work
  more valuable than early death, while keeping prayer reward too small to be
  farmed as the main objective.

Training result:

- W&B run id: `7fq4f7jf`.
- Compared against low-prayer global-progress retry `50xy19bw`, this was a
  large behavioral improvement:
  - Final `wave_reached`: `18.69` to `22.99`.
  - Final `npcs_slayed`: `65.89` to `84.61`.
  - Final `no_target_ticks`: `4143.1` to `333.0`.
  - Final no-target rate: `74.1%` to `14.2%`.
  - Final `no_progress_ticks`: `3963.1` to `477.8`.
  - Final no-progress rate: `70.8%` to `20.4%`.
  - Final `action_attack_target_ticks`: `826.2` to `1180.7`.
  - Final `attack_when_ready_rate`: `0.25` to `0.80`.
- Compared against pre-revamp benchmark `9lqts4k1`, it still regressed on final
  wave depth:
  - Final `wave_reached`: `28.04` benchmark vs `22.99` local-progress.
  - Final `npcs_slayed`: `107.45` benchmark vs `84.61` local-progress.
  - Final `episode_length`: `29341.5` benchmark vs `2344.1` local-progress.
- Peak during the run was much closer to benchmark:
  - Peak `wave_reached=27.99` at `1.374B` steps.
  - At that peak, `npcs_slayed=105.81`, `episode_length=3642.1`,
    `postclip_reward=25.54`, no-target rate `25.2%`, no-progress rate `27.1%`,
    and `attack_when_ready_rate=0.65`.
- The reward objective was no longer dominated by prayer:
  - Final `rwd_progress_total=22.40`.
  - Final `rwd_correct_danger_prayer_total=0.58`.
  - Final `rwd_no_progress_total=-0.08`.
  - Final `rwd_player_death_total=-1.0`.
- The old stall exploit was mostly gone:
  - Final `max_wave_ticks=290.9` on wave `20.1`.
  - Final `gross_damage_to_net_progress_ratio=1.21`, similar to prior
    net-progress runs, so gross damage was mostly translating into actual
    required-work removal.
  - Final `npc_healing_total=168.7`, all from Yt-MejKot, but it did not dominate
    the objective because progress reward still reached `22.40`.
- Remaining bad behavior:
  - The agent still died before late waves every episode.
  - Final `no_prayer_hits=298.8` and `wrong_prayer_hits=36.3`, worse than the
    benchmark's `no_prayer_hits=195.2` and `wrong_prayer_hits=30.8`.
  - Final `no_target_ticks=333.0` is much better than prior Step 2 attempts, but
    still not benchmark-level (`160.7`).
  - The run peaked near benchmark around `1.374B` steps, then settled lower by
    the final checkpoint, so late-training stability is still a concern.
- Decision:
  - Keep local current-wave progress scaling as the active Step 2 reward basis.
  - Do not revert to global cave-progress scaling.
  - The next bottleneck is no longer the severe no-target/no-progress stall.
    The next likely work is improving survival/prayer actionability and then
    target/action control if it remains a measurable issue.

Expected benefit:

- Keeps dense combat feedback: damaging an NPC pays immediately if it reduces
  required work.
- Removes the old repeated-damage exploit: damage that gets healed back becomes
  negative or zero net progress.
- Avoids the step 2 no-target stall by giving useful feedback before an NPC
  dies.
- Does not prescribe a playstyle. Kiting, trapping, safespotting, target
  switching, and healer handling can all work if they reduce required work.

Risk:

- Bad scaling could still flatten learning under the `-1, 1` reward clamp.
- Required-work accounting must handle Tz-Kek splits, Jad healers, wave
  transition timing, and cave completion correctly.
- Exposing progress observations changes the observation contract and needs
  guardrail tests.

## Step 2 Design Note: Restructure Rewards Around The `-1, 1` Clamp

Status: Fold into the net required-work progression reward

Current issue:

- If many small reward terms stack up, they can overpower the intended hierarchy.
- Completion, wave progress, and kills should matter more than shaping terms.
- The current structure can make local behavior look valuable even when the agent is not progressing.

Proposed change:

- Design the scalar reward so it intentionally fits the `-1, 1` clamp.
- Make the reward hierarchy explicit:
  - cave completion is most important
  - Jad kill / reaching wave 63 is next
  - wave completion is next
  - NPC kills are next
  - small penalties only clean up behavior

Expected benefit:

- Makes reward meaning clearer.
- Reduces cases where small repeated rewards dominate real progress.
- Makes training comparisons easier to interpret.

Risk:

- Bad scaling could make rewards too sparse or too flat.
- This should be tested with short runs before longer sweeps.

## Step 1: Add Explicit NPC Type One-Hot Observations

Status: Completed 2026-06-26

Current issue:

- The agent sees visible NPC position, HP, distance, LOS, attack timer, telegraph style, and pending hit timing/style.
- The agent does not currently see the actual NPC identity directly.
- That forces it to infer identity from indirect clues like HP, attack style, and position.

Proposed change:

- Add one-hot NPC type flags to each visible NPC slot:
  - Tz-Kih
  - Tz-Kek
  - small Tz-Kek
  - Tok-Xil
  - Yt-MejKot
  - Ket-Zek
  - TzTok-Jad
  - Yt-HurKot

Implementation notes:

- Added eight one-hot identity fields to every visible NPC observation slot.
- Per-NPC slot size changed from 12 floats to 20 floats.
- Policy observation size changed from 122 floats to 186 floats.
- Puffer-facing observation size changed from 153 floats to 217 floats.
- Observation contract version changed to
  `fight_caves_puffer_policy_obs_v3_npc_type_mask_heads_0_2_no_supplies`.
- Rewards, combat, pathing, action heads, action masks, loadout, and
  hyperparameters were not changed.
- Added a guardrail test that verifies each visible NPC type sets exactly one
  identity bit and empty slots keep all identity bits off.

Training result:

- W&B run id: `ql0yis9k`
- Compared against benchmark run id: `9lqts4k1`
- No Jad progress change: both runs had `reached_wave_63=0` and
  `jad_kill_rate=0`.
- Final average wave was effectively unchanged: `28.0440` benchmark vs
  `28.0527` step 1.
- Final NPCs slain was effectively unchanged: `107.4461` benchmark vs
  `107.8298` step 1.
- Step 1 produced longer episodes and longer stalled waves:
  `episode_length=29341.5` to `46287.7`, and `max_wave_ticks=11124.5` to
  `19087.5`.
- Prayer and invalid-action behavior improved on this run:
  `wrong_prayer_hits=30.81` to `23.37`, `no_prayer_hits=195.22` to `186.07`,
  `invalid_attack=132.04` to `53.14`, and `invalid_move=38.84` to `9.56`.
- The agent did much more Tok-Xil damage without clearing further:
  `dmg_to_tok_xil=247929.5` to `463701.9`, while total wave/NPC progress stayed
  flat. This suggests the core stall/damage-farming behavior was not solved by
  NPC identity obs alone.

Expected benefit:

- Lets the agent learn type-specific mechanics and target priority more directly.
- Makes observations closer to OSRS, where a player visually knows which monster is present.
- Does not directly give mechanics or rules to the agent.

Important clarification:

- This does not tell the agent "Tz-Kih drains prayer" or "Yt-MejKot heals."
- It only tells the agent which NPC type it is seeing.
- The agent still has to learn mechanics from outcomes, such as prayer points dropping near Tz-Kih or NPC HP increasing when Yt-MejKot heals.

Risk:

- Low implementation risk.
- Observation size changes, so contract tests and training bindings must be updated carefully.

## Step 3: Expose Prayer Decision Deadline

Status: Not started

Current issue:

- Pending-hit observations tell the agent when a hit will land.
- The more useful question is often: what is the last tick the agent can switch prayer and still block this hit?
- Impact timing is not always the same as decision deadline.

Proposed change:

- Add observation features that expose prayer decision deadline information.
- Keep this as observation state, not reward logic.

Expected benefit:

- Makes prayer learning more actionable.
- Should help mixed-style waves and Jad.
- Reduces cases where the agent technically sees an incoming hit but reacts too late.

Risk:

- Need to ensure the exposed deadline matches the actual combat resolution logic.
- Incorrect timing information would teach the wrong behavior.

## Step 4: Hard-Apply Action Masks

Status: Not started

Current issue:

- Invalid actions are currently part of the learning problem.
- The agent spends capacity learning which actions are impossible.
- Invalid-action penalties add reward clutter.

Proposed change:

- Apply action masks so impossible actions cannot be selected by the policy.
- Remove invalid-action reward penalties once masking is trusted.

Expected benefit:

- Simplifies the learning task.
- Removes invalid-action noise from training.
- Lets the reward focus on Fight Caves behavior instead of action-interface cleanup.

Risk:

- Masking bugs can silently break learning.
- Needs direct validation that every legal action remains selectable and every illegal action is actually masked.

## Step 5: Give Policy Real Control Over Target Persistence And Auto-Pathing

Status: Not started

Current issue:

- Current target persistence and auto-pathing may make disengaging, retargeting, and organic kiting harder than necessary.
- The policy may be trapped into behavior patterns created by backend assistance rather than its own decisions.

Proposed change:

- Review target persistence behavior.
- Review auto-pathing behavior.
- Expose cleaner policy control over when to keep target, drop target, move, attack, or path toward a target.

Expected benefit:

- Better target priority learning.
- Better disengage and reposition behavior.
- More natural kiting and wave cleanup.

Validation metrics to add before implementation:

- Keep the metric set focused because PufferLib's log dict has a fixed budget.
- These metrics should be diagnostics only and must not change training
  behavior.
- Existing useful metrics:
  - `no_target_ticks`
  - `target_held_ticks`
  - `target_in_range_los_ticks`
  - `target_out_of_range_or_los_ticks`
  - `action_attack_none_ticks`
  - `action_attack_target_ticks`
  - `attack_when_ready_rate`
  - `no_progress_no_target_ticks`
  - `no_progress_has_target_ticks`
  - `target_ticks_*` by NPC type
  - `dmg_to_*` by NPC type
- Proposed target-control metrics:
  - `target_acquire_count`: no-target to valid-target transitions.
  - `target_drop_count`: valid-target to no-target transitions while NPCs
    remain.
  - `target_switch_count`: valid-target to different-valid-target transitions.
  - `target_dropped_by_death_count`: target disappeared because it died.
  - `target_dropped_while_alive_count`: selected target was dropped while still
    alive.
  - `avg_target_lifetime_ticks`: average continuous ticks holding the same
    target.
  - `avg_retarget_latency_ticks`: average ticks from losing/killing a target to
    acquiring the next target.
  - `no_target_visible_npcs_ticks`: no target selected while at least one
    visible NPC exists.
  - `attack_none_with_target_ticks`: policy chose no attack while a target was
    selected.
  - `attack_none_without_target_ticks`: policy chose no attack while no target
    was selected.
  - `stale_target_ticks`: selected target is dead, invalid, invisible, or no
    longer attackable/useful.
  - `target_in_range_no_attack_ticks`: target is in range and line of sight,
    weapon is ready, but no attack attempt happens.
- Proposed auto-pathing metrics:
  - `route_active_ticks`: backend route/path is active.
  - `route_started_count`: new auto-path routes started.
  - `route_completed_count`: routes that reached their destination.
  - `route_cancelled_count`: routes cancelled or interrupted.
  - `route_stale_ticks`: route exists but target/destination is dead, changed,
    or no longer useful.
  - `route_overrode_move_cmd_ticks`: policy issued movement, but an active route
    controlled movement instead.
  - `approach_target_ticks`: auto-approach-to-target behavior is active.
  - `approach_success_count`: approach led to range/LOS or an attack attempt.
  - `approach_timeout_count`: approach stayed active too long without attack or
    progress.
- Validation signals after Step 5:
  - `no_target_visible_npcs_ticks` should go down.
  - `avg_retarget_latency_ticks` should go down.
  - `target_in_range_no_attack_ticks` should go down.
  - `route_stale_ticks` should go down.
  - `route_overrode_move_cmd_ticks` should go down if policy movement control
    improves.
  - `approach_success_count` should go up relative to route starts.
  - `action_attack_target_ticks / episode_length` should go up.
  - `no_progress_no_target_ticks` should go down.
  - `net_required_work_removed`, `wave_reached`, and `npcs_slayed` should
    improve without a new reward exploit.

Risk:

- Highest implementation risk of this list.
- Touches core control semantics.
- Should come after reward, observation, and masking changes are cleaner.

## Notes

- Immediate next work is still Step 2: redesign the reward objective after the
  rejected low-prayer run `50xy19bw`.
- Reward-clamp work should be handled as part of Step 2, not as a separate
  implementation step.
- NPC type observations are already complete and should stay enabled for all
  future runs.
- Step 5 should probably be last because it changes how the agent controls
  the character.
