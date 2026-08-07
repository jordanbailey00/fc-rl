# fc-viewer Instructions

These rules apply to the Raylib viewer, UI, assets, replay presentation, and debug tooling.

## Ownership

The viewer presents and controls the simulator; it does not define simulator behavior.

- Translate keyboard, mouse, UI, replay, and policy input into canonical `FC_NUM_ACTION_HEADS` actions.
- Advance behavior through `fc_step`/`fc_reset` and consume core snapshots/events.
- Do not directly mutate combat, movement, NPC, wave, reward, timer, collision, or prayer state to make the viewer behave differently from training.
- Visual interpolation, animation state, projectiles, effects, camera state, and UI state must never feed back into simulation results.

## Avoid duplicate logic

Do not reimplement or approximate:

- pathfinding or movement legality;
- distance, footprint, collision, or line-of-sight rules;
- attack legality, damage, prayer, healing, death, or wave logic;
- observation offsets, action-mask rules, reward calculations, or NPC slot ordering.

Consume core APIs and contract constants. If the viewer lacks information, add a clean read-only core snapshot/event API rather than recreating the mechanic locally.

## File and subsystem design

- Keep `viewer.c` as orchestration, not the automatic home for every new feature.
- Place cohesive UI behavior in the existing UI modules, asset decoding/loading in asset modules, and diagnostics in debug modules.
- Extract a new module when it owns a real lifecycle or coherent data model. Do not create wrapper-only files that merely move lines without reducing coupling.
- When replacing a panel, loader, replay path, or visual system, remove the old state, resources, update path, draw path, and cleanup path together.
- Every allocated resource must have one clear owner and cleanup path, including partial-load failure paths.

## Runtime and assets

- Load, decode, and build reusable assets outside the simulation/render hot loop. Cache stable resources rather than reopening files or rebuilding meshes each frame.
- Keep missing-asset behavior explicit. Do not silently substitute a misleading visual when the absence indicates a broken build or data contract.
- Rendering may interpolate between authoritative ticks but must display the same underlying state at tick boundaries.
- Use stable entity identity for animation/effect tracking. Do not bind long-lived visuals only to a transient visible-slot number.

## Diagnostics and contracts

- Observation, mask, action, and reward panels must use `fc_contracts.h` and the same core writers/breakdowns as training.
- Debug overlays may reveal pathing, collision, LOS, routes, and reward events, but must remain read-only.
- A contract change requires updating labels, dimensions, replay/input mapping, and relevant debug panels in the same patch.
- Keep policy replay behavior distinct from human-control behavior while feeding both through the canonical action interface.

## Validation

```bash
cmake -S runescape-rl -B runescape-rl/build -DCMAKE_BUILD_TYPE=Release
cmake --build runescape-rl/build -j --target fc_viewer test_headless validate_assets
ctest --test-dir runescape-rl/build -R 'test_headless|validate_assets' --output-on-failure
```

For visual-only changes that cannot be fully automated, still run headless and asset tests, then report the exact manual viewer scenarios exercised. Do not describe a visual check as completed unless it was actually performed.
