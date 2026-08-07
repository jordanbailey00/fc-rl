#!/usr/bin/env python3
"""Compare permanent same-version and linked-core/direct-include traces."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
from typing import Iterable


class ReplayFailure(RuntimeError):
    pass


TICK_FIELD_ORDER = (
    "record",
    "trace",
    "index",
    "actions",
    "state_hash",
    "rng",
    "terminal",
    "player",
    "player_pending",
    "npcs",
    "observation",
    "float_mask",
    "native_mask",
    "reward_features",
    "scalar_reward",
    "reward_runtime",
)
REWARD_RUNTIME_COMPONENTS = 15


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def sha256_paths(paths: Iterable[Path], root: Path) -> str:
    digest = hashlib.sha256()
    for path in sorted(paths):
        digest.update(path.relative_to(root).as_posix().encode("utf-8"))
        digest.update(b"\0")
        digest.update(path.read_bytes())
        digest.update(b"\0")
    return digest.hexdigest()


def parse_fields(line: str) -> dict[str, str]:
    fields: dict[str, str] = {"record": line.split("|", 1)[0]}
    for item in line.rstrip("\n").split("|")[1:]:
        key, separator, value = item.partition("=")
        fields[key] = value if separator else ""
    return fields


def validate_trace_schema(lines: list[str], trace_name: str) -> None:
    if not lines:
        raise ReplayFailure(f"{trace_name}: trace output is empty")
    tick_count = 0
    for line_number, line in enumerate(lines[1:], start=2):
        if not line.startswith("tick|"):
            continue
        tick_count += 1
        items = line.split("|")
        field_order = ("record",) + tuple(
            item.partition("=")[0] for item in items[1:]
        )
        if field_order != TICK_FIELD_ORDER:
            raise ReplayFailure(
                f"{trace_name}: incomplete tick schema at line={line_number}: "
                f"expected={TICK_FIELD_ORDER}, actual={field_order}"
            )
        runtime = items[-1].partition("=")[2]
        components = runtime.split(",") if runtime else []
        if (len(components) != REWARD_RUNTIME_COMPONENTS or
                any(not component for component in components)):
            raise ReplayFailure(
                f"{trace_name}: incomplete reward_runtime at line={line_number}: "
                f"expected={REWARD_RUNTIME_COMPONENTS} nonempty components, "
                f"actual={len(components)}"
            )
    if tick_count == 0:
        raise ReplayFailure(f"{trace_name}: trace contains no tick records")


def run_trace(binary: Path, output: Path) -> list[str]:
    command = [str(binary.resolve()), "--output", str(output.resolve())]
    completed = subprocess.run(
        command,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if completed.returncode != 0:
        raise ReplayFailure(
            f"trace command failed ({completed.returncode}): {command!r}\n"
            f"{completed.stdout}"
        )
    lines = output.read_text(encoding="utf-8").splitlines()
    validate_trace_schema(lines, output.name)
    return lines


def compare_lines(left: list[str], right: list[str], left_name: str,
                  right_name: str) -> None:
    if not left or not right:
        raise ReplayFailure("trace output is empty")
    left_meta = parse_fields(left[0])
    right_meta = parse_fields(right[0])
    comparable_left = {k: v for k, v in left_meta.items() if k != "backend"}
    comparable_right = {k: v for k, v in right_meta.items() if k != "backend"}
    if comparable_left != comparable_right:
        raise ReplayFailure(
            "compiled trace contract mismatch:\n"
            f"  {left_name}: {comparable_left}\n"
            f"  {right_name}: {comparable_right}"
        )
    if len(left) != len(right):
        raise ReplayFailure(
            f"trace length mismatch: {left_name}={len(left)}, "
            f"{right_name}={len(right)}"
        )

    for line_number, (left_line, right_line) in enumerate(
        zip(left[1:], right[1:]), start=2
    ):
        if left_line == right_line:
            continue
        left_fields = parse_fields(left_line)
        right_fields = parse_fields(right_line)
        actions = left_fields.get("actions", "not-a-tick-record")
        trace = left_fields.get("trace", "unknown")
        tick = left_fields.get("index", "unknown")
        differences = []
        for key in sorted(set(left_fields) | set(right_fields)):
            if left_fields.get(key) != right_fields.get(key):
                differences.append(
                    f"  {key}: {left_name}={left_fields.get(key)!r}, "
                    f"{right_name}={right_fields.get(key)!r}"
                )
        versions = {
            key: left_meta[key]
            for key in (
                "observation_version",
                "action_version",
                "reward_version",
                "prayer_timing_version",
                "state_hash_version",
            )
        }
        raise ReplayFailure(
            f"first replay mismatch at line={line_number}, trace={trace}, "
            f"tick={tick}, actions={actions}, versions={versions}:\n"
            + "\n".join(differences)
        )


def write_artifact(kind: str, artifact: Path, commands: list[list[str]],
                   traces: list[Path], metadata: dict[str, str]) -> None:
    runescape_root = Path(__file__).resolve().parents[2]
    repository_root = runescape_root.parent
    source_paths = [
        *sorted((runescape_root / "fc-core" / "include").glob("*.h")),
        *sorted((runescape_root / "fc-core" / "src").glob("*.c")),
        runescape_root / "fc-training" / "fight_caves.h",
        runescape_root / "fc-training" / "fight_caves.c",
    ]
    config_path = runescape_root / "config" / "fight_caves.ini"
    payload = {
        "schema": 1,
        "kind": kind,
        "comparison_pass": True,
        "contract": metadata,
        "backend_source_sha256": sha256_paths(source_paths, repository_root),
        "config_path": str(config_path),
        "config_sha256": sha256_file(config_path),
        "commands": commands,
        "traces": [
            {
                "path": str(path.resolve()),
                "sha256": sha256_file(path),
                "line_count": sum(1 for _ in path.open(encoding="utf-8")),
            }
            for path in traces
        ],
    }
    artifact.parent.mkdir(parents=True, exist_ok=True)
    temporary = artifact.with_name(artifact.name + ".tmp")
    temporary.write_text(
        json.dumps(payload, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    temporary.replace(artifact)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="case", required=True)

    same = subparsers.add_parser("same_version")
    same.add_argument("linked_binary", type=Path)
    same.add_argument("artifact", type=Path)

    cross = subparsers.add_parser("core_puffer")
    cross.add_argument("linked_binary", type=Path)
    cross.add_argument("puffer_binary", type=Path)
    cross.add_argument("artifact", type=Path)

    args = parser.parse_args()
    artifact = args.artifact.resolve()
    prefix = artifact.with_suffix("")

    if args.case == "same_version":
        binaries = [args.linked_binary, args.linked_binary]
        names = ["linked_run_1", "linked_run_2"]
    else:
        binaries = [args.linked_binary, args.puffer_binary]
        names = ["linked_fc_core", "puffer_direct_include"]

    traces = [Path(f"{prefix}-{name}.trace") for name in names]
    lines = [run_trace(binary, trace) for binary, trace in zip(binaries, traces)]
    compare_lines(lines[0], lines[1], names[0], names[1])
    meta = parse_fields(lines[0][0])
    meta.pop("backend", None)
    commands = [
        [str(binary.resolve()), "--output", str(trace.resolve())]
        for binary, trace in zip(binaries, traces)
    ]
    write_artifact(args.case, artifact, commands, traces, meta)
    print(
        f"PASS {args.case}: {len(lines[0]) - 1} deterministic records; "
        f"artifact={artifact}"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ReplayFailure as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
