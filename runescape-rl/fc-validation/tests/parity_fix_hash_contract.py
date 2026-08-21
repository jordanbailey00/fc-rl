#!/usr/bin/env python3
"""Static ownership/build-path assertions for the canonical state hash."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
RUNESCAPE = ROOT / "runescape-rl"


def read(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8")


def canonical_hash_ownership() -> int:
    failures: list[str] = []

    api = read("runescape-rl/fc-core/include/fc_api.h")
    version = re.search(r"^#define\s+FC_STATE_HASH_VERSION\s+(\d+)u?\s*$", api, re.MULTILINE)
    if version is None:
        failures.append("fc_api.h does not define numeric FC_STATE_HASH_VERSION")
    elif int(version.group(1)) != 2:
        failures.append(
            f"FC_STATE_HASH_VERSION is {version.group(1)}, expected canonical version 2"
        )

    core_hash_path = RUNESCAPE / "fc-core/src/fc_hash.c"
    if not core_hash_path.is_file():
        failures.append("fc-core/src/fc_hash.c does not exist")
        core_hash = ""
    else:
        core_hash = core_hash_path.read_text(encoding="utf-8")
        if not re.search(r"\bfc_state_hash\s*\(", core_hash):
            failures.append("fc-core/src/fc_hash.c does not define fc_state_hash")
        raw_patterns = (
            r"sizeof\s*\(\s*FcState\s*\)",
            r"\(\s*(?:const\s+)?(?:unsigned\s+char|uint8_t)\s*\*\s*\)\s*state",
        )
        if any(re.search(pattern, core_hash) for pattern in raw_patterns):
            failures.append("core hash appears to serialize raw FcState bytes")

    core_cmake = read("runescape-rl/fc-core/CMakeLists.txt")
    if "src/fc_hash.c" not in core_cmake:
        failures.append("fc_core target does not compile src/fc_hash.c")

    training_header = read("runescape-rl/fc-training/fight_caves.h")
    if '#include "fc_hash.c"' not in training_header:
        failures.append("direct-included Puffer backend does not compile fc_hash.c")

    training_build = "runescape-rl/fc-training/build.sh"
    if "FC_NO_HASH" in read(training_build):
        failures.append(f"{training_build} still disables hashing with FC_NO_HASH")

    viewer_cmake = read("runescape-rl/fc-viewer/CMakeLists.txt")
    if "src/fc_hash.c" in viewer_cmake:
        failures.append("viewer still compiles a viewer-local fc_hash.c")

    viewer_hash_path = RUNESCAPE / "fc-viewer/src/fc_hash.c"
    if viewer_hash_path.exists():
        viewer_hash = viewer_hash_path.read_text(encoding="utf-8")
        if re.search(r"\bfc_state_hash\s*\(", viewer_hash):
            failures.append("viewer-local file still defines an independent fc_state_hash")
        if "FNV_OFFSET" in viewer_hash or "FNV_PRIME" in viewer_hash:
            failures.append("viewer-local file still owns a hashing algorithm")

    debug_header = read("runescape-rl/fc-viewer/src/fc_debug.h")
    debug_source = read("runescape-rl/fc-viewer/src/fc_debug.c")
    if not re.search(r"uint32_t\s+state_hash_version\s*;", debug_header):
        failures.append("action-trace metadata does not record state_hash_version")
    if not re.search(
        r"trace->state_hash_version\s*=\s*FC_STATE_HASH_VERSION\s*;",
        debug_source,
    ):
        failures.append("action-trace initialization does not capture FC_STATE_HASH_VERSION")

    if failures:
        print("FAIL DET-001 ownership/build-path contract:")
        for failure in failures:
            print(f"  - {failure}")
        return 1

    print("PASS DET-001: core owns canonical hash in every compilation path")
    return 0


CASES = {"ownership": canonical_hash_ownership}


def main() -> int:
    if len(sys.argv) != 2 or sys.argv[1] not in CASES:
        print(f"usage: {Path(sys.argv[0]).name} <{'|'.join(CASES)}>", file=sys.stderr)
        return 2
    return CASES[sys.argv[1]]()


if __name__ == "__main__":
    raise SystemExit(main())
