# Fight Caves Validation Tooling

This directory holds reproducibility and validation tooling for training runs.
Tools here may read configs, build stamps, logs, or drive focused tests, but they
must not become part of the `fc-core` simulator logic.

Phase 1 tooling starts with `run_manifest.py`, which records the exact code,
config, loadout, supplies, and trainer contract used for a run.

Phase 2 guardrail tests should live in this validation/test layer or existing
test targets, with `fc-core` used as the system under test rather than a place
to embed one-off validation behavior.
