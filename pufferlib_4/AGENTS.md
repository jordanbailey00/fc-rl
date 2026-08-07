# Vendored PufferLib Boundary

`pufferlib_4` is an upstream PufferLib 4.0 dependency, not the default location for FC-RL-specific implementation work.

## Default rule

Do not modify this directory for an FC-RL mechanic, observation, action, mask, reward, config, analytics, environment, or build issue when the change can be implemented in:

- `runescape-rl/fc-core`;
- `runescape-rl/fc-training`;
- `runescape-rl/config`;
- `runescape-rl/tools`.

Do not reformat, reorganize, upgrade, or clean up vendored code as incidental work.

## Exception

Modify PufferLib only when the task explicitly requests an upstream-level change and there is clear evidence that the FC adapter cannot solve it without an incorrect workaround.

For an approved PufferLib change:

- keep the patch minimal and generic;
- do not embed Fight Caves-specific behavior in a generic trainer path;
- document why the existing extension surface is insufficient;
- keep the upstream patch separable from FC-RL adapter changes;
- test both generic PufferLib behavior and the FC-RL integration;
- record any divergence from the upstream PufferLib version.

Generated configs, logs, checkpoints, build products, and W&B data under this tree are not source files and should not be committed unless explicitly required.
