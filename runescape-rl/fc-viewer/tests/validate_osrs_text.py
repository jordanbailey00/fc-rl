#!/usr/bin/env python3
"""Reject viewer text paths that bypass the cache-backed OSRS fonts."""

from __future__ import annotations

import re
from pathlib import Path


SOURCE_ROOT = Path(__file__).resolve().parents[1] / "src"
BANNED = re.compile(r"\b(?:DrawText|MeasureText|GetFontDefault|DrawFPS)\s*\(")


def main() -> int:
    failures: list[str] = []
    for path in sorted((*SOURCE_ROOT.glob("*.c"), *SOURCE_ROOT.glob("*.h"))):
        for line_number, line in enumerate(path.read_text().splitlines(), 1):
            if BANNED.search(line):
                failures.append(f"{path.name}:{line_number}: {line.strip()}")
    if failures:
        raise SystemExit(
            "viewer text must use an explicit OSRS Font or fc_osrs_text:\n"
            + "\n".join(failures)
        )
    print("viewer OSRS font guard passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
