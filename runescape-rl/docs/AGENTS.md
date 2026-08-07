# Documentation Instructions

These rules apply to design documents, plans, experiment records, run history, and technical references.

## Preserve provenance

- Treat completed run results, hashes, timestamps, decisions, and historical configuration records as evidence. Do not rewrite past results to match the current implementation.
- Prefer appending a correction or superseding note over silently changing a historical claim.
- Clearly label proposed, accepted, implemented, validated, rejected, superseded, and historical material.
- Do not describe planned behavior as implemented or a single training result as a generally established conclusion.

## Canonical versus historical docs

- Keep current reference documentation in this directory aligned with the live repository.
- Keep run history and sweep history chronological and reproducible.
- Move obsolete implementation plans to a clearly historical/superseded state rather than leaving them as ambiguous active instructions.
- Do not duplicate detailed contract tables when they can be generated from or linked to the authoritative code. When a human-readable summary is useful, identify the code contract it summarizes.

## Writing changes

- Update documentation in the same patch when behavior, commands, module ownership, or policy-visible contracts change.
- Remove stale instructions and dead command examples rather than adding another contradictory section.
- Keep explanations concrete: source of truth, invariant, reason, validation, and compatibility impact.
- Avoid large generated narratives that obscure actionable current information.
