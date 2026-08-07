# Configuration Instructions

These rules apply to canonical FC-RL experiment configuration.

## Source of truth

`runescape-rl/config/fight_caves.ini` is the canonical default live Fight Caves configuration. `train.sh` copies `$CONFIG_PATH` into PufferLib's config location; `$CONFIG_PATH` defaults to the canonical file but may intentionally select an experiment configuration. `pufferlib_4/config/fight_caves.ini` is a generated runtime mirror, not an independently maintained source.

## Configuration discipline

- Do not add a key merely to avoid making a design decision in code.
- Before adding a knob, determine whether an existing key should be reused, renamed, consolidated, or deleted.
- Prefer one canonical behavior over multiple compatibility modes when there is no active requirement for both.
- Every live key must have exactly one intended meaning and a real reader. It should also appear in manifests/logs or other reproducibility output when it affects a run.
- Delete retired keys across config, parser/defaults, structs, logs, manifests, validation, and docs. Do not leave silent no-op keys.
- Keep defaults in code and canonical config aligned. A difference must be intentional and documented.
- Validate ranges and invalid combinations at startup. Do not silently clamp or substitute values unless that behavior is part of the documented contract.

## Experiment integrity

- Do not mix an unrelated mechanics change, reward redesign, observation change, and hyperparameter promotion into one experiment unless the task explicitly defines them as one versioned baseline.
- Preserve enough provenance to reproduce a run: code revision, active loadout, supplies, observation/action/reward versions, backend build identity, and resolved trainer settings.
- Comments should explain why a value is canonical and cite the relevant internal run/version identifier when known. Do not accumulate stale narratives above changed values.

## Version strings

Update observation, action, or reward version strings when policy-visible semantics change, including layout/order, masking, firing conditions, scaling, clipping contract, or active reward definition.

Do not bump semantic version strings for formatting, comments, diagnostics-only additions, or behavior-preserving refactors. When compatibility is intentionally preserved, add a regression test proving it.

## Validation

For config changes, run the relevant static/config guardrails and generate or inspect a run manifest without starting a long training run when possible. Verify that the canonical file, copied runtime file, parser, defaults, and manifest agree.
