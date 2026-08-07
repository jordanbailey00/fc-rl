#!/usr/bin/env python3
"""Produce and validate the post-fix TRAIN-001 health artifact.

The native probe observes the real compiled ``pufferlib._C.PuffeRL`` buffers
after normal rollout and learner calls.  A separate bounded ``train.sh`` cold
start proves the production launch, preflight, manifest, checkpoint, and
evaluator path without changing PufferLib or enabling diagnostics in the
performance hot path.
"""

from __future__ import annotations

import argparse
import configparser
import importlib.util
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import time
from typing import Any

from parity_perf import check_contract, load_puffer, process_rss_kb, sha256_file
from parity_training_health import validate_report


EXPECTED_OBSERVATION = 319
EXPECTED_MASK = 34
EXPECTED_ACTION_DIMS = [17, 9, 8]
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


class HealthRunFailure(RuntimeError):
    pass


def write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(
        json.dumps(payload, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    temporary.replace(path)


def load_probe(path: Path) -> Any:
    spec = importlib.util.spec_from_file_location("_fc_train_health_probe", path)
    if spec is None or spec.loader is None:
        raise HealthRunFailure(f"cannot import health probe: {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def build_probe(repo_root: Path, puffer_dir: Path, raw_dir: Path) -> Path:
    script = repo_root / "fc-validation/tests/build_parity_training_health_probe.sh"
    environment = os.environ.copy()
    environment["PUFFER_DIR"] = str(puffer_dir)
    environment["PYTHON_BIN"] = sys.executable
    completed = subprocess.run(
        ["bash", str(script)],
        cwd=repo_root,
        env=environment,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    (raw_dir / "probe-build.log").write_text(completed.stdout, encoding="utf-8")
    if completed.returncode != 0:
        raise HealthRunFailure(
            f"health probe build failed with exit {completed.returncode}"
        )
    lines = [line.strip() for line in completed.stdout.splitlines() if line.strip()]
    if not lines:
        raise HealthRunFailure("health probe build did not report an output path")
    output = Path(lines[-1]).resolve()
    if not output.is_file():
        raise HealthRunFailure(f"health probe output is missing: {output}")
    return output


def merge_finite(target: dict[str, bool], values: dict[str, Any]) -> None:
    for key, value in values.items():
        if key in target:
            target[key] = target[key] and value is True


def merge_counts(target: list[int], values: list[Any]) -> None:
    if len(values) != len(target):
        raise HealthRunFailure(
            f"diagnostic count width {len(values)} does not match {len(target)}"
        )
    for index, value in enumerate(values):
        target[index] += int(value)


def memory_sample(backend: Any, gpu_id: int) -> tuple[float, float]:
    utilization = dict(backend.get_utilization(gpu_id))
    rss_mb = process_rss_kb() / 1024.0
    gpu_mb = float(utilization["vram_used_gb"]) * 1024.0
    if not math.isfinite(rss_mb) or not math.isfinite(gpu_mb):
        raise HealthRunFailure("memory sampler produced a non-finite value")
    return rss_mb, gpu_mb


def memory_plateau(samples: list[float], tolerance_mb: float) -> bool:
    growth = samples[-1] - samples[0]
    monotonic = all(right > left for left, right in zip(samples, samples[1:]))
    return not monotonic or growth <= tolerance_mb


def instrumented_health_run(
    puffer_dir: Path,
    probe: Any,
    warmup_updates: int,
    measured_updates: int,
    gpu_id: int,
    memory_tolerance_mb: float,
) -> dict[str, Any]:
    backend, _pufferl, config = load_puffer(puffer_dir)
    contract = check_contract(
        backend,
        config,
        EXPECTED_OBSERVATION,
        EXPECTED_MASK,
        EXPECTED_ACTION_DIMS,
    )
    config["rank"] = 0
    config["world_size"] = 1
    config["gpu_id"] = gpu_id
    config["nccl_id"] = "None"
    config["wandb"] = False
    config["profile"] = False
    os.environ["OMP_NUM_THREADS"] = str(config["vec"]["num_threads"])

    finite = {channel: True for channel in FINITE_CHANNELS}
    prayer_counts = [0] * 8
    invalid_prayer_counts = [0] * 8
    rollout_masks_valid = True
    ppo_masks_valid = True
    every_head_has_legal_action = True
    invalid_buffer_events = 0
    rss_samples: list[float] = []
    gpu_samples: list[float] = []
    total_updates = warmup_updates + measured_updates
    puffer = backend.create_pufferl(config)
    try:
        initial_epoch = int(puffer.epoch)
        for update in range(total_updates):
            backend.rollouts(puffer)
            rollout_result = dict(probe.inspect_rollout(puffer))
            merge_finite(finite, dict(rollout_result["finite"]))
            merge_counts(
                prayer_counts, list(rollout_result["prayer_action_counts"])
            )
            merge_counts(
                invalid_prayer_counts,
                list(rollout_result["invalid_prayer_action_counts"]),
            )
            rollout_masks_valid = (
                rollout_masks_valid and rollout_result["masks_valid"] is True
            )
            every_head_has_legal_action = (
                every_head_has_legal_action
                and rollout_result["every_head_has_legal_action"] is True
            )
            invalid_buffer_events += int(
                rollout_result["invalid_buffer_events"]
            )

            backend.train(puffer)
            train_result = dict(probe.inspect_train(puffer))
            merge_finite(finite, dict(train_result["finite"]))
            ppo_masks_valid = (
                ppo_masks_valid and train_result["masks_valid"] is True
            )
            every_head_has_legal_action = (
                every_head_has_legal_action
                and train_result["every_head_has_legal_action"] is True
            )
            invalid_buffer_events += int(train_result["invalid_buffer_events"])

            if update >= warmup_updates:
                rss_mb, gpu_mb = memory_sample(backend, gpu_id)
                rss_samples.append(rss_mb)
                gpu_samples.append(gpu_mb)

        completed_updates = int(puffer.epoch) - initial_epoch
        log_values = dict(backend.log(puffer))
        model_parameters = int(puffer.num_params())
    finally:
        backend.close(puffer)

    if completed_updates != total_updates:
        raise HealthRunFailure(
            f"compiled learner completed {completed_updates} updates, "
            f"expected {total_updates}"
        )
    batch_size = int(config["vec"]["total_agents"]) * int(
        config["train"]["horizon"]
    )
    environment_log = dict(log_values.get("env", {}))
    public_losses = dict(log_values.get("loss", {}))
    for value in public_losses.values():
        if isinstance(value, (int, float)) and not math.isfinite(float(value)):
            raise HealthRunFailure("public learner log contains a non-finite loss")
    reward_keys = [key for key in environment_log if key.startswith("rwd_")]

    return {
        "contract": contract,
        "configuration": {
            "seed": int(config["seed"]),
            "environment_count": int(config["vec"]["total_agents"]),
            "num_buffers": int(config["vec"]["num_buffers"]),
            "num_threads": int(config["vec"]["num_threads"]),
            "horizon": int(config["train"]["horizon"]),
            "minibatch_size": int(config["train"]["minibatch_size"]),
            "replay_ratio": float(config["train"]["replay_ratio"]),
            "model_parameters": model_parameters,
            "warmup_updates": warmup_updates,
            "measured_updates": measured_updates,
        },
        "completed_updates": completed_updates,
        "completed_transitions": completed_updates * batch_size,
        "finite": finite,
        "native_masks": {
            "rollout_valid": rollout_masks_valid,
            "ppo_recompute_valid": ppo_masks_valid,
            "every_head_has_legal_action": every_head_has_legal_action,
            "invalid_buffer_events": invalid_buffer_events,
        },
        "prayer_action_counts": prayer_counts,
        "invalid_prayer_action_counts": invalid_prayer_counts,
        "metrics": {
            "episodes": int(float(environment_log.get("n", 0))),
            "reward_channels_populated": len(reward_keys),
            "reward_channel_keys": sorted(reward_keys),
        },
        "memory": {
            "warmup_complete": True,
            "rss_mb": rss_samples,
            "gpu_mb": gpu_samples,
            "tolerance_mb": memory_tolerance_mb,
            "plateau_pass": (
                memory_plateau(rss_samples, memory_tolerance_mb)
                and memory_plateau(gpu_samples, memory_tolerance_mb)
            ),
        },
        "public_losses": public_losses,
    }


def cudnn_library_dir() -> str | None:
    try:
        import nvidia.cudnn  # type: ignore
    except ImportError:
        return None
    return str(Path(nvidia.cudnn.__path__[0]) / "lib")


def real_train_and_evaluator(
    repo_root: Path,
    puffer_dir: Path,
    raw_dir: Path,
    total_updates: int,
    batch_size: int,
) -> dict[str, Any]:
    run_token = time.strftime("%Y%m%dT%H%M%SZ", time.gmtime())
    entrypoint_root = raw_dir / f"entrypoint-{run_token}-{os.getpid()}"
    checkpoint_root = entrypoint_root / "checkpoints"
    manifest_dir = entrypoint_root / "manifests"
    log_dir = entrypoint_root / "logs"
    preflight_path = entrypoint_root / "preflight.json"
    environment = os.environ.copy()
    environment.update({
        "PUFFER_DIR": str(puffer_dir),
        "PYTHON_BIN": sys.executable,
        "CONFIG_PATH": str(repo_root / "config/fight_caves.ini"),
        "FC_CHECKPOINT_ROOT": str(checkpoint_root),
        "FC_RUN_MANIFEST_DIR": str(manifest_dir),
        "FC_CONTRACT_PREFLIGHT_PATH": str(preflight_path),
    })
    command = [
        str(repo_root / "train.sh"),
        "--no-wandb",
        "--train.total-timesteps", str(total_updates * batch_size),
        "--log-dir", str(log_dir),
        "--eval-episodes", "1",
    ]
    started = time.monotonic()
    completed = subprocess.run(
        command,
        cwd=repo_root,
        env=environment,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    train_seconds = time.monotonic() - started
    train_log = entrypoint_root / "train-entrypoint.log"
    train_log.write_text(completed.stdout, encoding="utf-8")
    if completed.returncode != 0:
        raise HealthRunFailure(
            f"bounded train.sh job failed with exit {completed.returncode}; "
            f"log={train_log}"
        )

    checkpoints = sorted(checkpoint_root.rglob("*.bin"))
    if not checkpoints:
        raise HealthRunFailure("bounded train.sh job did not save a checkpoint")
    checkpoint = max(checkpoints, key=lambda path: int(path.stem))
    expected_step = total_updates * batch_size
    if int(checkpoint.stem) != expected_step:
        raise HealthRunFailure(
            f"final checkpoint step {checkpoint.stem} does not match "
            f"transition budget {expected_step}"
        )

    fake_viewer = entrypoint_root / "fake-viewer"
    fake_viewer.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
    fake_viewer.chmod(0o755)
    future = time.time() + 60.0
    os.utime(fake_viewer, (future, future))
    backends = sorted((puffer_dir / "pufferlib").glob("_C*.so"))
    if len(backends) != 1:
        raise HealthRunFailure(
            f"expected one compiled Puffer backend, found {len(backends)}"
        )
    evaluator_environment = environment.copy()
    evaluator_environment.update({
        "PUFFERLIB_DIR": str(puffer_dir),
        "FC_VIEWER_PATH": str(fake_viewer),
        "FC_COMPILED_BACKEND_PATH": str(backends[0]),
        "FC_SYNCED_CONFIG_PATH": str(puffer_dir / "config/fight_caves.ini"),
    })
    cudnn_dir = cudnn_library_dir()
    if cudnn_dir:
        inherited = evaluator_environment.get("LD_LIBRARY_PATH", "")
        evaluator_environment["LD_LIBRARY_PATH"] = (
            cudnn_dir if not inherited else f"{cudnn_dir}:{inherited}"
        )
    evaluator_command = [
        sys.executable,
        str(repo_root / "fc-viewer/eval_viewer.py"),
        "--ckpt", str(checkpoint),
        "--deterministic",
        "--episodes", "1",
    ]
    evaluator = subprocess.run(
        evaluator_command,
        cwd=repo_root,
        env=evaluator_environment,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    evaluator_log = entrypoint_root / "evaluator.log"
    evaluator_log.write_text(evaluator.stdout, encoding="utf-8")
    evaluator_ready = "[eval] Policy ready (CPU)" in evaluator.stdout
    random_fallback = "falling back" in evaluator.stdout.lower()
    if evaluator.returncode != 0 or not evaluator_ready or random_fallback:
        raise HealthRunFailure(
            "evaluator did not load and use the bounded training checkpoint; "
            f"exit={evaluator.returncode}, log={evaluator_log}"
        )

    return {
        "saved": True,
        "loaded_by_evaluator": True,
        "evaluator_random_fallback": False,
        "path": str(checkpoint.resolve()),
        "size_bytes": checkpoint.stat().st_size,
        "sha256": sha256_file(checkpoint),
        "checkpoint_step": int(checkpoint.stem),
        "train_command": command,
        "train_elapsed_seconds": train_seconds,
        "train_log": str(train_log.resolve()),
        "evaluator_command": evaluator_command,
        "evaluator_log": str(evaluator_log.resolve()),
    }


def positive_int(text: str) -> int:
    value = int(text)
    if value <= 0:
        raise argparse.ArgumentTypeError("value must be positive")
    return value


def canonical_batch_size(config_path: Path) -> int:
    parser = configparser.ConfigParser(interpolation=None)
    with config_path.open("r", encoding="utf-8") as handle:
        parser.read_file(handle)
    try:
        total_agents = parser.getint("vec", "total_agents")
        horizon = parser.getint("train", "horizon")
    except (configparser.Error, ValueError) as exc:
        raise HealthRunFailure(
            f"cannot derive the canonical training batch size: {exc}"
        ) from exc
    if total_agents <= 0 or horizon <= 0:
        raise HealthRunFailure("canonical environment count/horizon must be positive")
    return total_agents * horizon


def main() -> int:
    default_repo = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=default_repo)
    parser.add_argument("--puffer-dir", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--raw-dir", type=Path)
    parser.add_argument("--warmup-updates", type=positive_int, default=2)
    parser.add_argument("--measured-updates", type=positive_int, default=4)
    parser.add_argument("--gpu-id", type=int, default=0)
    parser.add_argument("--memory-growth-tolerance-mb", type=float, default=64.0)
    args = parser.parse_args()

    if args.measured_updates < 3:
        raise HealthRunFailure(
            "TRAIN-001 requires at least three post-warmup memory samples"
        )
    repo_root = args.repo_root.resolve()
    puffer_dir = (
        args.puffer_dir.resolve()
        if args.puffer_dir
        else (repo_root.parent / "pufferlib_4").resolve()
    )
    output = args.output.resolve()
    raw_dir = (
        args.raw_dir.resolve()
        if args.raw_dir
        else output.with_suffix("").resolve()
    )
    raw_dir.mkdir(parents=True, exist_ok=True)

    total_updates = args.warmup_updates + args.measured_updates
    batch_size = canonical_batch_size(repo_root / "config/fight_caves.ini")
    # Establish the production backend and checkpoint first.  train.sh owns
    # rebuild/preflight decisions, so the later probe cannot accidentally
    # inspect a stale extension that happened to exist before this run.
    checkpoint = real_train_and_evaluator(
        repo_root,
        puffer_dir,
        raw_dir,
        total_updates,
        batch_size,
    )

    probe_path = build_probe(repo_root, puffer_dir, raw_dir)
    # Importing the production backend before the probe is required so the
    # process-wide pybind11 registry owns the canonical PuffeRL Python type.
    backend, _pufferl, _config = load_puffer(puffer_dir)
    del backend, _pufferl, _config
    probe = load_probe(probe_path)

    health = instrumented_health_run(
        puffer_dir,
        probe,
        args.warmup_updates,
        args.measured_updates,
        args.gpu_id,
        args.memory_growth_tolerance_mb,
    )
    if health["completed_updates"] != total_updates:
        raise HealthRunFailure("instrumented and entrypoint update budgets differ")
    if health["completed_transitions"] != total_updates * batch_size:
        raise HealthRunFailure(
            "instrumented and entrypoint transition budgets differ"
        )
    report = {
        "schema": 1,
        "cold_start": True,
        "health_instrumentation": True,
        "probe": {
            "path": str(probe_path),
            "sha256": sha256_file(probe_path),
            "source": str(
                repo_root
                / "fc-validation/tests/parity_training_health_probe.cu"
            ),
        },
        **health,
        "checkpoint": checkpoint,
        "errors": [],
    }
    write_json(output, report)
    validate_report(
        report,
        report["completed_transitions"],
        report["completed_updates"],
        args.memory_growth_tolerance_mb,
    )
    print(
        "PASS: TRAIN-001 completed "
        f"{report['completed_updates']} PPO updates and "
        f"{report['completed_transitions']} transitions; report={output}"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"FAIL: {type(exc).__name__}: {exc}", file=sys.stderr)
        raise SystemExit(1)
