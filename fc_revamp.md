# Fight Caves Revamp Tracking

This doc tracks the next set of proposed changes for the Fight Caves RL environment. The goal is to improve learning by simplifying the reward contract, removing exploitable incentives, making key game state easier to observe, and reducing invalid-action noise.

We should implement these one by one where possible so training results stay interpretable.

## Benchmark Runs

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

## Training Run Tracking

Use `9lqts4k1` as the single pre-revamp benchmark. Each implementation below
gets its own run id so comparisons stay one-change-at-a-time.

| Stage | Change Tested | W&B Run ID | Steps | Notes |
| --- | --- | --- | --- | --- |
| Benchmark | Pre-`fc_revamp` simplified no-supplies config | `9lqts4k1` | 1.5B | Observation v2, no NPC type one-hot fields. |
| Step 1 | Explicit NPC type one-hot observations | `ql0yis9k` | 1.5B | Observation v3. Same loadout, rewards, actions, no-supplies config, and hparams as benchmark. |

## Proposed Order

1. Add explicit NPC type one-hot observations. Completed 2026-06-26.
2. Replace per-hit reward with NPC kill/progress rewards and NPC-heal penalty.
3. Rescale rewards around the `-1, 1` clamp.
4. Expose prayer decision deadline observations.
5. Hard-apply action masks in native PufferLib.
6. Give the policy real control over target persistence and auto-pathing.

## 1. Replace Per-Hit Reward With NPC Kill Reward

Status: Not started

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

## 2. Restructure Rewards Around The `-1, 1` Clamp

Status: Not started

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

## 3. Add Explicit NPC Type One-Hot Observations

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

## 4. Expose Prayer Decision Deadline

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

## 5. Hard-Apply Action Masks In Native PufferLib

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

## 6. Give Policy Real Control Over Target Persistence And Auto-Pathing

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

Risk:

- Highest implementation risk of this list.
- Touches core control semantics.
- Should come after reward, observation, and masking changes are cleaner.

## Notes

- Changes 1 and 2 are tightly connected and should likely be tested together.
- Change 3 is low risk and high value, so it is a good first implementation target.
- Change 6 should probably be last because it changes how the agent controls the character.
