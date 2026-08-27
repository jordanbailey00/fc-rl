# FC-RL documentation index

This directory separates live reference material from historical evidence.
Source code and the active INI remain authoritative when a prose summary and a
compiled contract disagree.

## Current sources of truth

- [`../../README.md`](../../README.md) — current architecture, commands,
  baseline, policy input, rewards, and repository status.
- [`../../TODO.md`](../../TODO.md) — unfinished work only.
- [`../config/fight_caves.ini`](../config/fight_caves.ini) — active environment,
  reward, trainer, policy, and contract configuration.
- [`../fc-core/include/fc_contracts.h`](../fc-core/include/fc_contracts.h) —
  canonical observation, action, reward-feature, and mask dimensions.
- [`../fc-core/include/fc_types.h`](../fc-core/include/fc_types.h) and the public
  headers beside it — simulation state and supported core interfaces.
- [`../tools/validation/README.md`](../tools/validation/README.md) — manifest,
  contract, checkpoint, and source guardrails.

## Historical evidence

- [`run_history.md`](run_history.md) — chronological training configurations
  and results. Older uses of "current" or "SOTA" are historical labels.
- [`sweep_history.md`](sweep_history.md) — chronological sweep records.
- [`../../sweep_top8.md`](../../sweep_top8.md) — detailed analysis of the 130-run
  v4.5 Stage 2 sweep.
- [`../../baseline.md`](../../baseline.md) — fixed 100M behavior-preservation
  results for the refactor and cleanup program.
- [`archive/fc_cleanup_and_parity_history.md`](archive/fc_cleanup_and_parity_history.md)
  — consolidated parity, refactor, dead-code, and documentation history.

Other files under `docs/archive`, and ignored local Markdown files at the
checkout root, are development evidence rather than active instructions. They
are intentionally not linked as requirements for a clean clone.
