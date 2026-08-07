#!/usr/bin/env python3
"""Validate the machine-readable report produced by post-fix TRAIN-001.

The post-fix training workstream owns instrumentation and report production.
This validation-side consumer fixes the acceptance schema without adding any
test-only branch to the simulator or trainer hot path.
"""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import sys
from typing import Any


FINITE_CHANNELS = (
    "observations",
    "logits",
    "rewards",
    "advantages",
    "returns",
    "policy_loss",
    "value_loss",
    "entropy",
    "kl",
    "gradients",
)


class HealthFailure(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise HealthFailure(message)


def require_number(value: Any, name: str) -> float:
    require(isinstance(value, (int, float)), f"{name} must be numeric")
    number = float(value)
    require(math.isfinite(number), f"{name} must be finite")
    return number


def validate_memory(report: dict[str, Any], tolerance_mb: float) -> None:
    memory = report.get("memory")
    require(isinstance(memory, dict), "memory section is missing")
    require(memory.get("warmup_complete") is True,
            "memory sampling must begin after allocator/cudagraph warmup")
    for key in ("rss_mb", "gpu_mb"):
        samples = memory.get(key)
        require(isinstance(samples, list) and len(samples) >= 3,
                f"memory.{key} must contain at least three post-warmup samples")
        numbers = [require_number(value, f"memory.{key}") for value in samples]
        growth = numbers[-1] - numbers[0]
        monotonic = all(right > left for left, right in zip(numbers, numbers[1:]))
        require(not monotonic or growth <= tolerance_mb,
                f"memory.{key} grew monotonically by {growth:.3f} MB, "
                f"above tolerance {tolerance_mb:.3f} MB")
    require(memory.get("plateau_pass") is True,
            "producer-side memory plateau check did not pass")


def validate_report(
    report: dict[str, Any],
    expected_transitions: int,
    expected_updates: int,
    memory_tolerance_mb: float,
) -> None:
    require(report.get("schema") == 1, "TRAIN-001 report schema must be 1")
    require(report.get("cold_start") is True, "TRAIN-001 must be a cold start")
    require(report.get("health_instrumentation") is True,
            "health instrumentation was not enabled")
    require(int(report.get("completed_transitions", -1)) == expected_transitions,
            "transition budget did not complete exactly")
    require(int(report.get("completed_updates", -1)) == expected_updates,
            "PPO update budget did not complete exactly")

    finite = report.get("finite")
    require(isinstance(finite, dict), "finite-value result section is missing")
    for channel in FINITE_CHANNELS:
        require(finite.get(channel) is True,
                f"non-finite or unverified training channel: {channel}")

    masks = report.get("native_masks")
    require(isinstance(masks, dict), "native-mask result section is missing")
    require(masks.get("rollout_valid") is True,
            "native masks were invalid during rollout sampling")
    require(masks.get("ppo_recompute_valid") is True,
            "native masks were invalid during PPO recomputation")
    require(masks.get("every_head_has_legal_action") is True,
            "an action head had no legal action")
    require(int(masks.get("invalid_buffer_events", -1)) == 0,
            "native action-mask/buffer errors were recorded")

    action_counts = report.get("prayer_action_counts")
    require(isinstance(action_counts, list) and len(action_counts) == 8,
            "prayer_action_counts must contain exactly eight entries")
    for action, count in enumerate(action_counts):
        require(int(count) > 0, f"prayer action {action} was never sampled")
    invalid_counts = report.get("invalid_prayer_action_counts")
    require(isinstance(invalid_counts, list) and len(invalid_counts) == 8,
            "invalid_prayer_action_counts must contain exactly eight entries")
    for action in (5, 6, 7):
        require(int(invalid_counts[action]) == 0,
                f"flick prayer action {action} was classified as invalid by ID")

    metrics = report.get("metrics")
    require(isinstance(metrics, dict), "episode/reward metrics are missing")
    require(int(metrics.get("episodes", 0)) > 0, "no episode metric was emitted")
    require(int(metrics.get("reward_channels_populated", 0)) > 0,
            "reward-channel metrics were not populated")

    checkpoint = report.get("checkpoint")
    require(isinstance(checkpoint, dict), "checkpoint result section is missing")
    require(checkpoint.get("saved") is True, "checkpoint save failed")
    require(checkpoint.get("loaded_by_evaluator") is True,
            "evaluator did not load the saved checkpoint")
    require(checkpoint.get("evaluator_random_fallback") is False,
            "evaluator fell back to random behavior")
    require(int(checkpoint.get("size_bytes", 0)) > 0,
            "checkpoint size was not recorded")

    errors = report.get("errors")
    require(isinstance(errors, list) and not errors,
            f"training health report contains errors: {errors!r}")
    validate_memory(report, memory_tolerance_mb)


def positive_int(text: str) -> int:
    value = int(text)
    if value <= 0:
        raise argparse.ArgumentTypeError("value must be positive")
    return value


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path)
    parser.add_argument("--expected-transitions", type=positive_int, required=True)
    parser.add_argument("--expected-updates", type=positive_int, required=True)
    parser.add_argument("--memory-growth-tolerance-mb", type=float, default=64.0)
    args = parser.parse_args()

    with args.report.open("r", encoding="utf-8") as handle:
        report = json.load(handle)
    require(isinstance(report, dict), "TRAIN-001 report root must be an object")
    validate_report(
        report,
        args.expected_transitions,
        args.expected_updates,
        args.memory_growth_tolerance_mb,
    )
    print(f"PASS: TRAIN-001 health report {args.report}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except HealthFailure as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
