# Tooling Instructions

These rules apply to validation, reproducibility, analysis, build-support, and operational scripts under `tools/`.

## Boundary

Tools may inspect configs, manifests, logs, checkpoints, build stamps, binaries, and simulator outputs. They must not become part of gameplay state transitions or contain a second definition of simulator mechanics.

## Script design

- Make inputs, outputs, defaults, and side effects explicit.
- Resolve paths from declared repository/script roots rather than assuming the caller's working directory.
- Fail loudly on missing, malformed, stale, or incompatible inputs. Do not silently continue with partial provenance.
- Keep deterministic tools deterministic: stable sorting, explicit seeds, stable serialization, and no dependence on local timezone or unordered iteration.
- Prefer structured output for data consumed by other tools. Preserve schema versions for manifests and machine-readable reports.
- Use shell for orchestration and environment setup; move nontrivial parsing, validation, and data transformation into focused Python code.
- Do not add network access, uploads, destructive cleanup, or training execution as an implicit side effect of an inspection command.

## Avoid duplicated contracts

Consume repository constants, generated metadata, manifests, or a single shared mapping where practical. Do not independently retype observation dimensions, action layouts, reward channel names, config defaults, or loadout identities in multiple scripts.

If a static check intentionally mirrors a contract, keep the mirror narrow, identify its authoritative source, and add a failure message that tells maintainers what must be synchronized.

## Reproducibility and history

- Treat existing run manifests and historical logs as immutable evidence.
- New schema fields must be backward-compatible for readers or accompanied by an explicit migration/version branch.
- Distinguish missing data from a numeric zero or false value.
- Do not rewrite historical metrics to match current naming; translate at read time when necessary.

## Refactoring and tests

- Remove superseded scripts, flags, and duplicated parsers when replacing them.
- Add focused tests for parsers, path resolution, schema validation, and failure behavior.
- For build or training-launch scripts, preserve a side-effect-free help/introspection path.
- Report the exact command, input, and output artifact used for any generated conclusion.
