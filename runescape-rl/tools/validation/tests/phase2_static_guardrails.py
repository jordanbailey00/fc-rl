#!/usr/bin/env python3
"""Static Phase 2 guardrails that should stay outside fc-core."""

from __future__ import annotations

import configparser
from pathlib import Path
import re
import sys


RUNESCAPE_DIR = Path(__file__).resolve().parents[3]


def analytics_no_global_metrics() -> int:
    """Desired state: training analytics are per-env/per-thread, not globals."""
    paths = [
        RUNESCAPE_DIR / "fc-training" / "fight_caves.h",
        RUNESCAPE_DIR / "fc-training" / "binding.c",
    ]
    patterns = [
        re.compile(r"\bfloat\s+g_(?:sum|fires|n_|max|most)"),
        re.compile(r"\bextern\s+float\s+g_"),
        re.compile(r"\bg_n_analytics\b"),
    ]

    hits: list[str] = []
    for path in paths:
        text = path.read_text(encoding="utf-8")
        for lineno, line in enumerate(text.splitlines(), 1):
            if any(pattern.search(line) for pattern in patterns):
                hits.append(f"{path.relative_to(RUNESCAPE_DIR)}:{lineno}: {line.strip()}")

    if hits:
        print("FAIL: global analytics counters still present:")
        for hit in hits[:20]:
            print(f"  {hit}")
        if len(hits) > 20:
            print(f"  ... {len(hits) - 20} more")
        return 1

    print("PASS: analytics counters are no longer global training state")
    return 0


def live_safespot_reward_zero() -> int:
    """Live configs should not pay a direct reward for shallow safespot attacks."""
    paths = [
        RUNESCAPE_DIR / "config" / "fight_caves.ini",
        RUNESCAPE_DIR.parent / "pufferlib_4" / "config" / "fight_caves.ini",
    ]

    failures: list[str] = []
    for path in paths:
        parser = configparser.ConfigParser()
        parser.read(path, encoding="utf-8")
        if not parser.has_option("env", "shape_safespot_attack_reward"):
            failures.append(f"{path.relative_to(RUNESCAPE_DIR.parent)}: missing shape_safespot_attack_reward")
            continue
        value = parser.getfloat("env", "shape_safespot_attack_reward")
        if abs(value) > 0.000001:
            failures.append(
                f"{path.relative_to(RUNESCAPE_DIR.parent)}: shape_safespot_attack_reward={value}"
            )

    if failures:
        print("FAIL: live configs still enable direct safespot reward:")
        for failure in failures:
            print(f"  {failure}")
        return 1

    print("PASS: live configs keep direct safespot reward disabled")
    return 0


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print("usage: phase2_static_guardrails.py <analytics_no_global_metrics|live_safespot_reward_zero>", file=sys.stderr)
        return 2
    if argv[1] == "analytics_no_global_metrics":
        return analytics_no_global_metrics()
    if argv[1] == "live_safespot_reward_zero":
        return live_safespot_reward_zero()
    print(f"unknown guardrail: {argv[1]}", file=sys.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
