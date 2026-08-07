# Local TODO

This file is intentionally local-only and ignored by git. Use it as a working list
for project follow-ups that should not become public repo documentation yet.

## Current Baseline: v4_simple_reward

- The authoritative live config is `runescape-rl/config/fight_caves.ini`.
- It promotes the exact `mmyxbyn4` trainer recipe to a 1.5B-step budget with
  current 60-second HP regeneration mechanics.
- Use W&B run `l9o32hhz` as the seed-73 empirical comparison baseline.
- The source and synchronized Puffer INIs are byte-identical with SHA-256
  `509af1168dca12f1c9f72ffc51d31bc2e7c7476536785915f20b034418127514`.
- Full derivation, exact config, trajectory, and final metrics are recorded at
  the top of `runescape-rl/docs/run_history.md`.

## Immediate Next Steps: Validate v4 Before Stage 2

This section is the authoritative next work.

Planning document for all proposed sweep stages:
`v3_simple_reward_sweep.md`. Stage 1 is complete. Stage 2 is the planned value
and optimizer sweep, but it must use `v4_simple_reward` as its fixed baseline
and must not start until the validation gate below passes.

### 1. Visually Evaluate the New Baseline

- [ ] Select useful checkpoints from W&B baseline run `l9o32hhz`, including an
  early/transition policy, a strong late policy near the sampled 1.254B Jad
  peak, and the final 1.499B policy.
- [ ] Replay each selected checkpoint in the eval viewer, preferably across
  multiple episodes rather than drawing conclusions from one cave.
- [ ] Validate learned behavior: movement and combat timing, targeting and
  target priority, prayer switching and conservation, attack activity,
  safespot/LOS use, wave progression, Yt-MejKot handling, Jad prayer, healer
  aggro, healer targeting, and healer/Jad interactions.
- [ ] Compare visible behavior against `l9o32hhz` metrics so apparent issues
  are supported or contradicted quantitatively.
- [ ] Record checkpoint paths, observed behavior, and any suspected mechanics
  defects before changing code or configuration.

### 2. Test the Environment for Defects

- [ ] Build a focused mechanics checklist covering natural HP regeneration,
  prayer drain and protection, player/NPC attack timing, damage calculation,
  movement speed, pathfinding, collision, LOS, safespots, NPC footprints,
  target/aggro persistence, wave composition, split NPCs, Yt-MejKot healing,
  Jad attacks, Yt-HurKot spawn/aggro/pathing/healing, deaths, resets, rewards,
  observations, and hard action masks.
- [ ] Exercise the checklist with targeted validation-module tests and focused
  playable-viewer scenarios. Keep diagnostic tooling outside `fc-core`.
- [ ] Compare uncertain mechanics against OSRS references and the local
  RuneScape reference repositories before labeling behavior defective.
- [ ] Verify the 100-tick natural HP regeneration correction remains active:
  no HP before tick 100 and exactly 1 HP on tick 100.
- [ ] Check for additional accidental simplifications or timing errors similar
  to the old 10-tick HP regeneration defect.
- [ ] Document each finding as confirmed correct, intentional simplification,
  confirmed defect, or unresolved. Do not fix behavior based only on visual
  suspicion.
- [ ] For every confirmed defect, add a failing guardrail first, implement the
  smallest correction, rerun the full validation suite, and assess whether the
  change invalidates `l9o32hhz` as the empirical baseline.

### 3. Stage 2 Go/No-Go Gate

- [ ] Confirm no known training-relevant mechanics defect remains unresolved.
- [ ] Confirm the viewer accurately represents backend actions well enough for
  behavioral evaluation.
- [ ] Confirm rewards, observations, action masks, loadout, seed, policy,
  backend build, live INI, and synchronized Puffer INI match the documented
  `v4_simple_reward` contract.
- [ ] If validation changes backend behavior, rerun `v4_simple_reward` before
  sweeping so Stage 2 has a valid post-fix baseline.
- [ ] If the gate passes without training-impacting changes, configure Stage 2
  from `v3_simple_reward_sweep.md` around the v4 trainer recipe. Sweep only
  `vf_coef`, `vf_clip_coef`, `max_grad_norm`, and `beta1`; keep the environment,
  rewards, observations, actions, architecture, loadout, seed, and all selected
  Stage 1 trainer values fixed.
- [ ] Decide and document the Stage 2 trial count and per-trial timestep budget
  before creating its INI. Do not mix environment fixes or reward changes into
  that sweep.

## Repo / PufferLib PR Readiness

- Clean up the entire repo so it is ready to PR into PufferLib on GitHub.
  - Align file layout, build scripts, config style, docs, tests, and asset handling
    with PufferLib conventions.
  - Organize assets, UI, and viewer frontend data into clean installable/downloadable
    packs instead of relying on loose local clutter.
  - Provide a script or setup path that installs required Fight Caves assets when a
    user runs the environment.
  - Keep raw cache inputs local/external unless there is a deliberate reason to ship
    them.
  - Ensure the environment can build, train, evaluate, and render from a clean clone
    using documented commands.
