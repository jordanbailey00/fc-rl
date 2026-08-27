# Fight Caves Validation Tooling

This directory holds reproducibility and source-level validation tooling for
training and evaluation. It may inspect configs, build stamps, compiled
backends, checkpoints, logs, or first-party source, but it must not become part
of the `fc-core` simulation.

## Current tools

- `run_manifest.py` records the selected source and synchronized INIs, git
  commit and dirty state, backend source/build hashes, loadout, supplies,
  policy contract, checkpoint request, Python executable, and launch command.
- `contract_preflight.py` validates config contracts against the compiled
  backend, prepares contract-specific checkpoint directories and sidecars,
  verifies checkpoint dimensions/parameter bytes, and validates sweep spaces.
- `tests/phase2_static_guardrails.py` checks source/config invariants such as
  normal `fc_core` linkage, the live no-supplies baseline, supported policy
  dimensions, deterministic metric ownership, and removal of retired paths.
- `tests/cuda_arch_guardrails.sh` verifies CUDA architecture selection without
  embedding machine-specific policy in the simulator.

The normal C/CMake behavioral tests live in `fc-validation` and `fc-viewer`
because they exercise compiled code. A future strict-warning/static-analysis/
linker-reachability audit belongs in that validation layer as an explicitly
invoked maintenance target, not an automatic requirement for every change.
