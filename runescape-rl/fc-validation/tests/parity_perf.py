#!/usr/bin/env python3
"""Reproducible parity performance baselines.

The public commands run fresh-process trials and write one JSON artifact:

  core              linked fc_core deterministic workload
  puffer-rollout    synchronous Fight Caves binding/VecEnv batch stepping
  puffer-training   compiled Puffer rollout + PPO learner updates

The hidden ``_puffer-trial`` command exists only so every Puffer trial starts
with a fresh Python/CUDA process. Production mechanics remain unmodified.
"""

from __future__ import annotations

import argparse
import copy
import ctypes
import hashlib
import importlib.metadata
import json
import math
import os
from pathlib import Path
import platform
import resource
import shutil
import statistics
import subprocess
import sys
import threading
import time
from typing import Any, Callable


RESULT_PREFIX = "PARITY_PERF_JSON="
DEFAULT_CV_LIMIT_PCT = 5.0


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def command_output(command: list[str]) -> str | None:
    try:
        completed = subprocess.run(
            command,
            check=True,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
    except (OSError, subprocess.CalledProcessError):
        return None
    return completed.stdout.strip()


def git_value(repo_root: Path, *args: str) -> str | None:
    return command_output(["git", "-C", str(repo_root), *args])


def cpu_model() -> str | None:
    try:
        for line in Path("/proc/cpuinfo").read_text(encoding="utf-8").splitlines():
            if line.startswith("model name"):
                return line.split(":", 1)[1].strip()
    except OSError:
        return None
    return None


def memory_total_kb() -> int | None:
    try:
        for line in Path("/proc/meminfo").read_text(encoding="utf-8").splitlines():
            if line.startswith("MemTotal:"):
                return int(line.split()[1])
    except (OSError, ValueError):
        return None
    return None


def cpu_governors() -> list[str]:
    values: set[str] = set()
    for path in Path("/sys/devices/system/cpu").glob(
        "cpu[0-9]*/cpufreq/scaling_governor"
    ):
        try:
            values.add(path.read_text(encoding="utf-8").strip())
        except OSError:
            continue
    return sorted(values)


def distribution(values: list[float]) -> dict[str, float | int]:
    if not values:
        raise ValueError("cannot summarize an empty trial set")
    mean = statistics.fmean(values)
    sample_variance = statistics.variance(values) if len(values) > 1 else 0.0
    sample_stddev = math.sqrt(sample_variance)
    cv_pct = 100.0 * sample_stddev / mean if mean else math.inf
    return {
        "trials": len(values),
        "median": statistics.median(values),
        "mean": mean,
        "minimum": min(values),
        "maximum": max(values),
        "sample_variance": sample_variance,
        "sample_stddev": sample_stddev,
        "coefficient_of_variation_pct": cv_pct,
    }


def parse_affinity(spec: str) -> set[int] | None:
    if not spec:
        return None
    cpus: set[int] = set()
    for part in spec.split(","):
        part = part.strip()
        if not part:
            continue
        if "-" in part:
            first_text, last_text = part.split("-", 1)
            first = int(first_text)
            last = int(last_text)
            if first > last:
                raise ValueError(f"invalid CPU affinity range: {part}")
            cpus.update(range(first, last + 1))
        else:
            cpus.add(int(part))
    if not cpus:
        raise ValueError("CPU affinity must contain at least one CPU")
    return cpus


def set_affinity(spec: str) -> None:
    cpus = parse_affinity(spec)
    if cpus is not None:
        os.sched_setaffinity(0, cpus)


def process_rss_kb() -> int:
    try:
        for line in Path("/proc/self/status").read_text(encoding="utf-8").splitlines():
            if line.startswith("VmRSS:"):
                return int(line.split()[1])
    except (OSError, ValueError):
        return 0
    return 0


class WindowSampler:
    def __init__(self, gpu_sample: Callable[[], dict[str, Any]] | None = None):
        self._gpu_sample = gpu_sample
        self._stop = threading.Event()
        self._thread = threading.Thread(target=self._run, daemon=True)
        self.peak_rss_kb = 0
        self.gpu_util_pct: list[float] = []
        self.gpu_vram_used_gb: list[float] = []
        self.errors: list[str] = []

    def _sample(self) -> None:
        self.peak_rss_kb = max(self.peak_rss_kb, process_rss_kb())
        if self._gpu_sample is None:
            return
        try:
            values = dict(self._gpu_sample())
            if "gpu_percent" in values:
                self.gpu_util_pct.append(float(values["gpu_percent"]))
            if "vram_used_gb" in values:
                self.gpu_vram_used_gb.append(float(values["vram_used_gb"]))
        except Exception as exc:  # diagnostics must not kill the measured child
            self.errors.append(f"{type(exc).__name__}: {exc}")

    def _run(self) -> None:
        self._sample()
        while not self._stop.wait(0.2):
            self._sample()

    def start(self) -> None:
        self._thread.start()

    def finish(self) -> dict[str, Any]:
        self._stop.set()
        self._thread.join()
        self._sample()
        return {
            "peak_rss_kb_window": self.peak_rss_kb,
            "gpu_util_pct_mean_window": (
                statistics.fmean(self.gpu_util_pct)
                if self.gpu_util_pct else None
            ),
            "gpu_util_pct_max_window": (
                max(self.gpu_util_pct) if self.gpu_util_pct else None
            ),
            "peak_gpu_memory_gb_window": (
                max(self.gpu_vram_used_gb) if self.gpu_vram_used_gb else None
            ),
            "sampler_errors": self.errors,
        }


def hardware_metadata(repo_root: Path, harness_path: Path) -> dict[str, Any]:
    nvcc = shutil.which("nvcc")
    if nvcc is None and Path("/usr/local/cuda/bin/nvcc").exists():
        nvcc = "/usr/local/cuda/bin/nvcc"
    compiler = shutil.which("cc") or "cc"
    try:
        affinity = sorted(os.sched_getaffinity(0))
    except AttributeError:
        affinity = []
    metadata: dict[str, Any] = {
        "captured_at": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
        "repo_root": str(repo_root.resolve()),
        "git_commit": git_value(repo_root, "rev-parse", "HEAD"),
        "git_commit_time": git_value(repo_root, "show", "-s", "--format=%cI", "HEAD"),
        "git_status_porcelain": git_value(repo_root, "status", "--porcelain"),
        "harness_path": str(harness_path.resolve()),
        "harness_sha256": sha256_file(harness_path),
        "hostname": platform.node(),
        "platform": platform.platform(),
        "python": sys.version,
        "python_executable": sys.executable,
        "cpu_model": cpu_model(),
        "logical_cpu_count": os.cpu_count(),
        "process_affinity": affinity,
        "cpu_governors": cpu_governors(),
        "system_memory_total_kb": memory_total_kb(),
        "compiler": command_output([compiler, "--version"]),
        "cuda_compiler": command_output([nvcc, "--version"]) if nvcc else None,
        "gpu": command_output([
            "nvidia-smi",
            "--query-gpu=name,driver_version,memory.total,power.limit",
            "--format=csv,noheader",
        ]),
    }
    for package in ("numpy", "pybind11", "torch"):
        try:
            metadata[f"python_package_{package}"] = importlib.metadata.version(package)
        except importlib.metadata.PackageNotFoundError:
            metadata[f"python_package_{package}"] = None
    return metadata


def parse_result(output: str) -> dict[str, Any]:
    for line in reversed(output.splitlines()):
        if line.startswith(RESULT_PREFIX):
            return json.loads(line[len(RESULT_PREFIX):])
    raise RuntimeError("trial did not emit a PARITY_PERF_JSON result")


def write_artifact(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    with temporary.open("w", encoding="utf-8") as handle:
        json.dump(payload, handle, indent=2, sort_keys=True)
        handle.write("\n")
    os.replace(temporary, path)


def run_trials(
    commands: list[list[str]],
    raw_dir: Path,
) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    raw_dir.mkdir(parents=True, exist_ok=True)
    trials: list[dict[str, Any]] = []
    command_records: list[dict[str, Any]] = []
    for index, command in enumerate(commands, start=1):
        started = time.time()
        completed = subprocess.run(
            command,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        raw_path = raw_dir / f"trial-{index:02d}.log"
        raw_path.write_text(completed.stdout, encoding="utf-8")
        command_records.append({
            "trial": index,
            "command": command,
            "raw_log": str(raw_path.resolve()),
            "exit_code": completed.returncode,
            "wall_seconds_including_startup": time.time() - started,
        })
        if completed.returncode != 0:
            raise RuntimeError(
                f"trial {index} failed with exit {completed.returncode}; "
                f"see {raw_path}"
            )
        trial = parse_result(completed.stdout)
        trial["trial"] = index
        trials.append(trial)
    return trials, command_records


def finalize_artifact(
    *,
    kind: str,
    repo_root: Path,
    harness_path: Path,
    output: Path,
    raw_dir: Path,
    trials: list[dict[str, Any]],
    commands: list[dict[str, Any]],
    primary_metric: str,
    cv_limit_pct: float,
    extra: dict[str, Any],
) -> int:
    summary = distribution([float(trial[primary_metric]) for trial in trials])
    summary["cv_limit_pct"] = cv_limit_pct
    summary["cv_pass"] = (
        float(summary["coefficient_of_variation_pct"]) <= cv_limit_pct
    )
    payload = {
        "schema": 1,
        "kind": kind,
        "retrospective": True,
        "metadata": hardware_metadata(repo_root, harness_path),
        "configuration": extra,
        "commands": commands,
        "trials": trials,
        "primary_metric": primary_metric,
        "summary": summary,
    }
    write_artifact(output, payload)
    print(json.dumps({
        "artifact": str(output.resolve()),
        "kind": kind,
        "summary": summary,
        "raw_dir": str(raw_dir.resolve()),
    }, indent=2, sort_keys=True))
    return 0 if summary["cv_pass"] else 3


def core_command(args: argparse.Namespace) -> int:
    binary = Path(args.binary).resolve()
    if not binary.is_file():
        raise FileNotFoundError(binary)
    base_command = [
        str(binary),
        "--warmup-steps", str(args.warmup_steps),
        "--measure-steps", str(args.measure_steps),
        "--scenario-span", str(args.scenario_span),
        "--seed", str(args.seed),
        "--prayer-limit", str(args.prayer_limit),
    ]
    if args.cpu_affinity:
        taskset = shutil.which("taskset")
        if taskset is None:
            raise RuntimeError("taskset is required when --cpu-affinity is set")
        base_command = [taskset, "-c", args.cpu_affinity, *base_command]
    commands = [list(base_command) for _ in range(args.trials)]
    output = Path(args.output).resolve()
    raw_dir = Path(args.raw_dir).resolve() if args.raw_dir else output.with_suffix("")
    trials, records = run_trials(commands, raw_dir)
    checksums = {trial["checksum"] for trial in trials}
    if len(checksums) != 1:
        raise RuntimeError(f"core trial checksum drift: {sorted(checksums)}")
    return finalize_artifact(
        kind="core",
        repo_root=Path(args.repo_root),
        harness_path=Path(__file__),
        output=output,
        raw_dir=raw_dir,
        trials=trials,
        commands=records,
        primary_metric="sps",
        cv_limit_pct=args.cv_limit_pct,
        extra={
            "binary": str(binary),
            "binary_sha256": sha256_file(binary),
            "build_flags": args.build_flags,
            "workload": args.workload,
            "warmup_steps": args.warmup_steps,
            "measure_steps": args.measure_steps,
            "scenario_span": args.scenario_span,
            "seed": args.seed,
            "prayer_limit": args.prayer_limit,
            "cpu_affinity": args.cpu_affinity,
        },
    )


def parse_action_dims(text: str) -> list[int]:
    values = [int(value.strip()) for value in text.split(",") if value.strip()]
    if not values or any(value <= 0 for value in values):
        raise argparse.ArgumentTypeError("action dimensions must be positive integers")
    return values


def load_puffer(puffer_dir: Path) -> tuple[Any, Any, dict[str, Any]]:
    sys.path.insert(0, str(puffer_dir))
    from pufferlib import _C  # type: ignore
    from pufferlib import pufferl  # type: ignore

    saved_argv = sys.argv
    try:
        sys.argv = [saved_argv[0]]
        config = pufferl.load_config("fight_caves")
    finally:
        sys.argv = saved_argv
    return _C, pufferl, config


def check_contract(
    backend: Any,
    config: dict[str, Any],
    expected_obs: int,
    expected_mask: int,
    expected_dims: list[int],
) -> dict[str, Any]:
    probe_args = copy.deepcopy(config)
    probe_args["vec"]["total_agents"] = 1
    probe_args["vec"]["num_buffers"] = 1
    probe = backend.create_vec(probe_args, gpu=0)
    try:
        actual = {
            "observation_stride": int(probe.obs_size),
            "action_stride": int(probe.num_atns),
            "action_dims": [int(value) for value in probe.act_sizes],
            "mask_stride": sum(int(value) for value in probe.act_sizes),
            "observation_dtype": str(probe.obs_dtype),
            "observation_element_bytes": int(probe.obs_elem_size),
        }
    finally:
        probe.close()
    expected = {
        "observation_stride": expected_obs,
        "action_stride": len(expected_dims),
        "action_dims": expected_dims,
        "mask_stride": expected_mask,
    }
    for key, expected_value in expected.items():
        if actual[key] != expected_value:
            raise RuntimeError(
                f"compiled contract mismatch for {key}: "
                f"expected {expected_value!r}, actual {actual[key]!r}"
            )
    return actual


def rusage_cpu_seconds(value: resource.struct_rusage) -> float:
    return float(value.ru_utime + value.ru_stime)


def finite_tree(value: Any) -> bool:
    if isinstance(value, dict):
        return all(finite_tree(child) for child in value.values())
    if isinstance(value, (list, tuple)):
        return all(finite_tree(child) for child in value)
    if isinstance(value, (int, float)):
        return math.isfinite(float(value))
    return True


def rollout_trial(args: argparse.Namespace) -> dict[str, Any]:
    import numpy as np

    set_affinity(args.cpu_affinity)
    backend, _pufferl, config = load_puffer(Path(args.puffer_dir))
    contract = check_contract(
        backend, config, args.expected_obs, args.expected_mask,
        args.expected_action_dims,
    )
    total_agents = int(config["vec"]["total_agents"])
    prayer_dim = int(contract["action_dims"][2])
    if args.prayer_limit > prayer_dim:
        raise RuntimeError(
            f"prayer limit {args.prayer_limit} exceeds compiled dimension {prayer_dim}"
        )
    os.environ["OMP_NUM_THREADS"] = str(config["vec"]["num_threads"])

    vector = backend.create_vec(config, gpu=0)
    cycle = args.action_cycle
    env_index = np.arange(total_agents, dtype=np.uint32)
    actions = np.empty((cycle, total_agents, 3), dtype=np.float32)
    for tick in range(cycle):
        actions[tick, :, 0] = (env_index * 3 + tick * 5) % contract["action_dims"][0]
        actions[tick, :, 1] = (env_index * 5 + tick * 7) % contract["action_dims"][1]
        actions[tick, :, 2] = (env_index * 7 + tick * 11) % args.prayer_limit

    try:
        vector.reset()
        for tick in range(args.warmup_steps):
            vector.cpu_step(int(actions[tick % cycle].ctypes.data))
        vector.log()

        usage_before = resource.getrusage(resource.RUSAGE_SELF)
        sampler = WindowSampler()
        sampler.start()
        started = time.perf_counter()
        for tick in range(args.measure_steps):
            vector.cpu_step(int(actions[tick % cycle].ctypes.data))
        elapsed = time.perf_counter() - started
        samples = sampler.finish()
        usage_after = resource.getrusage(resource.RUSAGE_SELF)
        env_log = dict(vector.log())

        obs_count = total_agents * int(vector.obs_size)
        obs = (ctypes.c_float * obs_count).from_address(int(vector.obs_ptr))
        checksum = 0
        for index in (0, vector.obs_size - 1, obs_count // 2, obs_count - 1):
            checksum = ((checksum * 16777619) ^ int(obs[index] * 1000000.0)) & 0xFFFFFFFF
    finally:
        vector.close()

    transitions = args.measure_steps * total_agents
    cpu_seconds = rusage_cpu_seconds(usage_after) - rusage_cpu_seconds(usage_before)
    prayer_counts = [0] * args.prayer_limit
    full_cycles, remainder = divmod(args.measure_steps, cycle)
    for value in range(args.prayer_limit):
        per_cycle = int(np.count_nonzero(actions[:, :, 2] == value))
        tail = int(np.count_nonzero(actions[:remainder, :, 2] == value))
        prayer_counts[value] = full_cycles * per_cycle + tail

    result = {
        "kind": "puffer-rollout",
        "contract": contract,
        "warmup_vector_steps": args.warmup_steps,
        "measured_vector_steps": args.measure_steps,
        "total_transitions": transitions,
        "environment_count": total_agents,
        "num_buffers": int(config["vec"]["num_buffers"]),
        "num_threads": int(config["vec"]["num_threads"]),
        "prayer_limit": args.prayer_limit,
        "prayer_action_counts": prayer_counts,
        "elapsed_seconds": elapsed,
        "sps": transitions / elapsed,
        "cpu_seconds": cpu_seconds,
        "cpu_util_pct": 100.0 * cpu_seconds / elapsed,
        "peak_rss_kb_process": resource.getrusage(resource.RUSAGE_SELF).ru_maxrss,
        "reset_count": float(env_log.get("n", 0.0)),
        "reset_rate": float(env_log.get("n", 0.0)) / transitions,
        "environment_log": env_log,
        "final_observation_checksum": f"{checksum:08x}",
        **samples,
    }
    if not finite_tree(result):
        raise RuntimeError("non-finite value in rollout result")
    return result


def training_trial(args: argparse.Namespace) -> dict[str, Any]:
    set_affinity(args.cpu_affinity)
    backend, _pufferl, config = load_puffer(Path(args.puffer_dir))
    contract = check_contract(
        backend, config, args.expected_obs, args.expected_mask,
        args.expected_action_dims,
    )
    config["rank"] = 0
    config["world_size"] = 1
    config["gpu_id"] = args.gpu_id
    config["nccl_id"] = "None"
    config["wandb"] = False
    config["profile"] = False
    os.environ["OMP_NUM_THREADS"] = str(config["vec"]["num_threads"])

    puffer = backend.create_pufferl(config)
    checkpoint_path = Path(args.checkpoint_path)
    checkpoint_path.parent.mkdir(parents=True, exist_ok=True)
    try:
        model_params = int(puffer.num_params())
        for _ in range(args.warmup_updates):
            backend.rollouts(puffer)
            backend.train(puffer)
        backend.log(puffer)

        usage_before = resource.getrusage(resource.RUSAGE_SELF)
        sampler = WindowSampler(lambda: dict(backend.get_utilization(args.gpu_id)))
        sampler.start()
        started = time.perf_counter()
        for _ in range(args.measure_updates):
            backend.rollouts(puffer)
            backend.train(puffer)
        elapsed = time.perf_counter() - started
        samples = sampler.finish()
        usage_after = resource.getrusage(resource.RUSAGE_SELF)
        log_values = dict(backend.log(puffer))

        checkpoint_started = time.perf_counter()
        backend.save_weights(puffer, str(checkpoint_path))
        checkpoint_save_seconds = time.perf_counter() - checkpoint_started
        checkpoint_size = checkpoint_path.stat().st_size
        checkpoint_sha256 = sha256_file(checkpoint_path)
        load_started = time.perf_counter()
        backend.load_weights(puffer, str(checkpoint_path))
        checkpoint_load_seconds = time.perf_counter() - load_started
    finally:
        backend.close(puffer)

    batch_size = int(config["vec"]["total_agents"]) * int(config["train"]["horizon"])
    transitions = args.measure_updates * batch_size
    cpu_seconds = rusage_cpu_seconds(usage_after) - rusage_cpu_seconds(usage_before)
    perf = dict(log_values.get("perf", {}))
    losses = dict(log_values.get("loss", {}))
    env_log = dict(log_values.get("env", {}))
    rollout_seconds = float(perf.get("rollout", 0.0))
    train_seconds = float(perf.get("train", 0.0))
    result = {
        "kind": "puffer-training",
        "contract": contract,
        "warmup_updates": args.warmup_updates,
        "measured_updates": args.measure_updates,
        "completed_updates": args.measure_updates,
        "total_transitions": transitions,
        "environment_count": int(config["vec"]["total_agents"]),
        "num_buffers": int(config["vec"]["num_buffers"]),
        "num_threads": int(config["vec"]["num_threads"]),
        "rollout_horizon": int(config["train"]["horizon"]),
        "minibatch_size": int(config["train"]["minibatch_size"]),
        "replay_ratio": float(config["train"]["replay_ratio"]),
        "seed": int(config["seed"]),
        "hidden_size": int(config["policy"]["hidden_size"]),
        "num_layers": int(config["policy"]["num_layers"]),
        "model_parameters": model_params,
        "elapsed_seconds": elapsed,
        "total_training_sps": transitions / elapsed,
        "rollout_sps": transitions / rollout_seconds if rollout_seconds > 0 else None,
        "rollout_seconds": rollout_seconds,
        "policy_inference_seconds": float(perf.get("eval_gpu", 0.0)),
        "environment_step_seconds": float(perf.get("eval_env", 0.0)),
        "learner_seconds": train_seconds,
        "learner_misc_seconds": float(perf.get("train_misc", 0.0)),
        "learner_forward_seconds": float(perf.get("train_forward", 0.0)),
        "losses": losses,
        "environment_log": env_log,
        "reset_count": float(env_log.get("n", 0.0)),
        "reset_rate": float(env_log.get("n", 0.0)) / transitions,
        "cpu_seconds": cpu_seconds,
        "cpu_util_pct": 100.0 * cpu_seconds / elapsed,
        "peak_rss_kb_process": resource.getrusage(resource.RUSAGE_SELF).ru_maxrss,
        "checkpoint_save_success": True,
        "checkpoint_load_success": True,
        "checkpoint_size_bytes": checkpoint_size,
        "checkpoint_sha256": checkpoint_sha256,
        "checkpoint_save_seconds": checkpoint_save_seconds,
        "checkpoint_load_seconds": checkpoint_load_seconds,
        **samples,
    }
    if not losses:
        raise RuntimeError("training benchmark produced no loss metrics")
    if not finite_tree(result):
        raise RuntimeError("non-finite value in training result")
    checkpoint_path.unlink(missing_ok=True)
    return result


def puffer_trial_command(args: argparse.Namespace) -> int:
    if args.trial_kind == "rollout":
        result = rollout_trial(args)
    elif args.trial_kind == "training":
        result = training_trial(args)
    else:
        raise RuntimeError(f"unsupported trial kind: {args.trial_kind}")
    print(RESULT_PREFIX + json.dumps(result, sort_keys=True), flush=True)
    return 0


def puffer_parent_command(args: argparse.Namespace, kind: str) -> int:
    script = Path(__file__).resolve()
    output = Path(args.output).resolve()
    raw_dir = Path(args.raw_dir).resolve() if args.raw_dir else output.with_suffix("")
    commands: list[list[str]] = []
    for trial in range(1, args.trials + 1):
        command = [
            sys.executable, str(script), "_puffer-trial",
            "--trial-kind", kind,
            "--puffer-dir", str(Path(args.puffer_dir).resolve()),
            "--expected-obs", str(args.expected_obs),
            "--expected-mask", str(args.expected_mask),
            "--expected-action-dims", ",".join(map(str, args.expected_action_dims)),
            "--cpu-affinity", args.cpu_affinity,
            "--gpu-id", str(args.gpu_id),
        ]
        if kind == "rollout":
            command.extend([
                "--warmup-steps", str(args.warmup_steps),
                "--measure-steps", str(args.measure_steps),
                "--action-cycle", str(args.action_cycle),
                "--prayer-limit", str(args.prayer_limit),
            ])
        else:
            checkpoint = raw_dir / f"trial-{trial:02d}-checkpoint.bin"
            command.extend([
                "--warmup-updates", str(args.warmup_updates),
                "--measure-updates", str(args.measure_updates),
                "--checkpoint-path", str(checkpoint),
            ])
        commands.append(command)

    trials, records = run_trials(commands, raw_dir)
    primary = "sps" if kind == "rollout" else "total_training_sps"
    puffer_dir = Path(args.puffer_dir).resolve()
    config_path = puffer_dir / "config" / "fight_caves.ini"
    backends = sorted((puffer_dir / "pufferlib").glob("_C*.so"))
    extra: dict[str, Any] = {
        "puffer_dir": str(puffer_dir),
        "config_path": str(config_path),
        "config_sha256": sha256_file(config_path),
        "backend_paths": [str(path) for path in backends],
        "backend_sha256": {str(path): sha256_file(path) for path in backends},
        "expected_observation_stride": args.expected_obs,
        "expected_mask_stride": args.expected_mask,
        "expected_action_dims": args.expected_action_dims,
        "cpu_affinity": args.cpu_affinity,
        "gpu_id": args.gpu_id,
    }
    if kind == "rollout":
        extra.update({
            "workload": args.workload,
            "warmup_vector_steps": args.warmup_steps,
            "measure_vector_steps": args.measure_steps,
            "action_cycle": args.action_cycle,
            "prayer_limit": args.prayer_limit,
        })
    else:
        extra.update({
            "warmup_updates": args.warmup_updates,
            "measure_updates": args.measure_updates,
            "cold_start": True,
            "health_instrumentation": False,
        })
    return finalize_artifact(
        kind=f"puffer-{kind}",
        repo_root=Path(args.repo_root),
        harness_path=script,
        output=output,
        raw_dir=raw_dir,
        trials=trials,
        commands=records,
        primary_metric=primary,
        cv_limit_pct=args.cv_limit_pct,
        extra=extra,
    )


def positive_int(text: str) -> int:
    value = int(text)
    if value <= 0:
        raise argparse.ArgumentTypeError("value must be positive")
    return value


def add_common_parent_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--repo-root", required=True)
    parser.add_argument("--trials", type=positive_int, default=5)
    parser.add_argument("--output", required=True)
    parser.add_argument("--raw-dir")
    parser.add_argument("--cpu-affinity", default="")
    parser.add_argument("--cv-limit-pct", type=float, default=DEFAULT_CV_LIMIT_PCT)


def add_puffer_contract_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--puffer-dir", required=True)
    parser.add_argument("--expected-obs", type=positive_int, required=True)
    parser.add_argument("--expected-mask", type=positive_int, required=True)
    parser.add_argument(
        "--expected-action-dims", type=parse_action_dims, required=True,
    )
    parser.add_argument("--gpu-id", type=int, default=0)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    core = subparsers.add_parser("core")
    add_common_parent_arguments(core)
    core.add_argument("--binary", required=True)
    core.add_argument("--build-flags", required=True)
    core.add_argument("--workload", choices=("common", "native"), default="common")
    core.add_argument("--warmup-steps", type=positive_int, default=250000)
    core.add_argument("--measure-steps", type=positive_int, default=5000000)
    core.add_argument("--scenario-span", type=positive_int, default=512)
    core.add_argument("--seed", type=positive_int, default=73)
    core.add_argument("--prayer-limit", type=positive_int, required=True)

    rollout = subparsers.add_parser("puffer-rollout")
    add_common_parent_arguments(rollout)
    add_puffer_contract_arguments(rollout)
    rollout.add_argument("--workload", choices=("common", "native"), default="common")
    rollout.add_argument("--warmup-steps", type=positive_int, default=128)
    rollout.add_argument("--measure-steps", type=positive_int, default=2048)
    rollout.add_argument("--action-cycle", type=positive_int, default=256)
    rollout.add_argument("--prayer-limit", type=positive_int, required=True)

    training = subparsers.add_parser("puffer-training")
    add_common_parent_arguments(training)
    add_puffer_contract_arguments(training)
    training.add_argument("--warmup-updates", type=positive_int, default=2)
    training.add_argument("--measure-updates", type=positive_int, default=10)

    trial = subparsers.add_parser("_puffer-trial", help=argparse.SUPPRESS)
    add_puffer_contract_arguments(trial)
    trial.add_argument("--trial-kind", choices=("rollout", "training"), required=True)
    trial.add_argument("--cpu-affinity", default="")
    trial.add_argument("--warmup-steps", type=positive_int, default=128)
    trial.add_argument("--measure-steps", type=positive_int, default=2048)
    trial.add_argument("--action-cycle", type=positive_int, default=256)
    trial.add_argument("--prayer-limit", type=positive_int, default=5)
    trial.add_argument("--warmup-updates", type=positive_int, default=2)
    trial.add_argument("--measure-updates", type=positive_int, default=10)
    trial.add_argument("--checkpoint-path", default="/tmp/parity-training-checkpoint.bin")

    return parser


def main() -> int:
    args = build_parser().parse_args()
    if args.command == "core":
        return core_command(args)
    if args.command == "puffer-rollout":
        return puffer_parent_command(args, "rollout")
    if args.command == "puffer-training":
        return puffer_parent_command(args, "training")
    if args.command == "_puffer-trial":
        return puffer_trial_command(args)
    raise RuntimeError(f"unknown command: {args.command}")


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"ERROR: {type(exc).__name__}: {exc}", file=sys.stderr)
        raise
