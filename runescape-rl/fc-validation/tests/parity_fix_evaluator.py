#!/usr/bin/env python3
"""Focused evaluator-side checks for the parity action/buffer contract."""

from __future__ import annotations

import importlib.util
import io
from pathlib import Path
import sys

import numpy as np


RUNESCAPE_DIR = Path(__file__).resolve().parents[2]
EVALUATOR_PATH = RUNESCAPE_DIR / "fc-viewer" / "eval_viewer.py"


def load_evaluator():
    spec = importlib.util.spec_from_file_location("fc_eval_viewer", EVALUATOR_PATH)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load evaluator module from {EVALUATOR_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_contract_dimensions(fixture_so: Path) -> int:
    evaluator = load_evaluator()
    actual = evaluator.load_compiled_policy_contract(fixture_so)
    expected = (285, [17, 9, 8], 34, 319)
    if actual != expected:
        print(f"FAIL CONTRACT-002: evaluator contract {actual!r}, expected {expected!r}")
        return 1

    print("PASS CONTRACT-002: evaluator derives 319 inputs and {17,9,8} actions")
    return 0


def test_action_and_line_buffers(fixture_so: Path) -> int:
    evaluator = load_evaluator()
    policy_obs_size, act_dims, mask_size, total_size = (
        evaluator.load_compiled_policy_contract(fixture_so)
    )
    del policy_obs_size

    exact_values = " ".join(str(i / 1000.0) for i in range(total_size)) + "\n"

    class ReadProc:
        def __init__(self, text: str):
            self.stdout = io.StringIO(text)

    parsed = evaluator.read_obs_line(ReadProc(exact_values), total_size)
    if parsed is None or parsed.shape != (319,) or parsed.dtype != np.float32:
        print("FAIL CONTRACT-003: evaluator did not consume exactly one 319-float line")
        return 1
    if evaluator.read_obs_line(ReadProc("0 " * (total_size - 1) + "\n"), total_size) is not None:
        print("FAIL CONTRACT-003: evaluator accepted a short observation/mask line")
        return 1

    calls: list[tuple[int, np.ndarray]] = []
    original_choice = evaluator.np.random.choice

    def choose_last(dim: int, p: np.ndarray):
        calls.append((dim, p.copy()))
        return dim - 1

    evaluator.np.random.choice = choose_last
    try:
        actions = evaluator.sample_masked(
            [np.zeros(dim, dtype=np.float32) for dim in act_dims],
            np.ones(mask_size, dtype=np.float32),
            act_dims,
            deterministic=False,
        )
    finally:
        evaluator.np.random.choice = original_choice

    if actions != [16, 8, 7] or [dim for dim, _ in calls] != act_dims:
        print(
            "FAIL CONTRACT-002: evaluator random sampler did not use all "
            f"{{17,9,8}} values: actions={actions}, dims={[dim for dim, _ in calls]}"
        )
        return 1

    class WriteProc:
        def __init__(self):
            self.stdin = io.StringIO()

    proc = WriteProc()
    evaluator.send_actions(proc, actions)
    if proc.stdin.getvalue() != "16 8 7\n":
        print(
            "FAIL CONTRACT-003: evaluator emitted the wrong three-head action stride: "
            f"{proc.stdin.getvalue()!r}"
        )
        return 1

    print("PASS CONTRACT-003: evaluator line/action buffers use 319/34/3 strides")
    return 0


def main(argv: list[str]) -> int:
    if len(argv) != 3:
        print(
            f"usage: {argv[0]} "
            "<contract_dimensions|action_and_line_buffers> <fixture-so>",
            file=sys.stderr,
        )
        return 2
    fixture_so = Path(argv[2]).resolve()
    if not fixture_so.is_file():
        print(f"compiled contract fixture is missing: {fixture_so}", file=sys.stderr)
        return 2
    if argv[1] == "contract_dimensions":
        return test_contract_dimensions(fixture_so)
    if argv[1] == "action_and_line_buffers":
        return test_action_and_line_buffers(fixture_so)
    print(f"unknown evaluator contract test: {argv[1]}", file=sys.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
