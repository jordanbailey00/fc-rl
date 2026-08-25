#!/usr/bin/env python3
"""Reproducible loadout, extended-soak, and sanitizer release gates."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import time
from typing import Any


LOADOUTS = (
    "FC_LOADOUT_BLACK_DHIDE_RCB",
    "FC_LOADOUT_SOTA_TBOW",
    "FC_LOADOUT_LOW_DEF_RCB",
    "FC_LOADOUT_RCB_PURE",
    "FC_LOADOUT_MSBI_PURE",
    "FC_LOADOUT_BLOWPIPE_PURE",
    "FC_LOADOUT_ACB_ARMADYL",
    "FC_LOADOUT_BOWFA_CRYSTAL",
    "FC_LOADOUT_TBOW_MASORI",
)

SANITIZER_ENV = {
    "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1:strict_string_checks=1",
    "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1",
}

PYTHON_NATIVE_SANITIZER_TESTS = (
    "parity_contract_002_evaluator_dimensions",
    "parity_contract_003_evaluator_buffers",
    "parity_contract_004_compiled_preflight",
    "parity_contract_004_preflight_rejections",
    "parity_contract_004_manifest_schema2",
    "parity_contract_004_active_configs",
    "parity_contract_004_evaluator_explicit_random",
    "parity_contract_004_evaluator_hard_failures",
    "parity_contract_004_manifest_warm_modes",
)


class GateFailure(RuntimeError):
    pass


def write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(
        json.dumps(payload, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    temporary.replace(path)


class Recorder:
    def __init__(self, raw_dir: Path, environment: dict[str, str] | None = None):
        self.raw_dir = raw_dir
        self.raw_dir.mkdir(parents=True, exist_ok=True)
        self.environment = environment or {}
        self.records: list[dict[str, Any]] = []

    def run(
        self,
        label: str,
        command: list[str],
        environment: dict[str, str] | None = None,
    ) -> None:
        index = len(self.records) + 1
        log_path = self.raw_dir / f"{index:03d}-{label}.log"
        env = os.environ.copy()
        env.update(self.environment)
        if environment:
            env.update(environment)
        started = time.monotonic()
        completed = subprocess.run(
            command,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            env=env,
        )
        elapsed = time.monotonic() - started
        log_path.write_text(completed.stdout, encoding="utf-8")
        record = {
            "label": label,
            "command": command,
            "exit_code": completed.returncode,
            "elapsed_seconds": elapsed,
            "raw_log": str(log_path.resolve()),
        }
        if environment:
            record["environment_overrides"] = dict(environment)
        self.records.append(record)
        print(
            f"[{index:03d}] {label}: exit={completed.returncode} "
            f"elapsed={elapsed:.2f}s"
        )
        if completed.returncode != 0:
            tail = "\n".join(completed.stdout.splitlines()[-30:])
            raise GateFailure(
                f"{label} failed with exit {completed.returncode}; "
                f"log={log_path}\n{tail}"
            )


def command_output(command: list[str]) -> str | None:
    try:
        return subprocess.run(
            command,
            check=True,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        ).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return None


def metadata() -> dict[str, Any]:
    cpu_model = None
    try:
        for line in Path("/proc/cpuinfo").read_text(encoding="utf-8").splitlines():
            if line.startswith("model name"):
                cpu_model = line.split(":", 1)[1].strip()
                break
    except OSError:
        pass
    return {
        "platform": platform.platform(),
        "hostname": platform.node(),
        "cpu_model": cpu_model,
        "logical_cpus": os.cpu_count(),
        "compiler": command_output(["cc", "--version"]),
        "cmake": command_output(["cmake", "--version"]),
        "python": sys.version,
    }


def sanitizer_python_environment() -> dict[str, str]:
    libasan = command_output(["cc", "-print-file-name=libasan.so"])
    if not libasan or libasan == "libasan.so" or not Path(libasan).is_file():
        raise GateFailure(
            "could not locate the active compiler's libasan for Python/native "
            "sanitizer tests"
        )
    inherited = os.environ.get("LD_PRELOAD", "")
    preload = libasan if not inherited else f"{libasan}:{inherited}"
    return {
        "LD_PRELOAD": preload,
        "ASAN_OPTIONS": (
            "detect_leaks=0:halt_on_error=1:strict_string_checks=1"
        ),
    }


def binary(build_dir: Path, name: str) -> Path:
    path = build_dir / "fc-validation" / name
    if not path.is_file():
        raise GateFailure(f"required target was not built: {path}")
    return path


def configure_and_build(
    recorder: Recorder,
    source: Path,
    build_dir: Path,
    loadout: str,
    jobs: int,
    sanitizers: bool,
    all_targets: bool = False,
) -> None:
    command = [
        "cmake", "-S", str(source), "-B", str(build_dir),
        "-DCMAKE_BUILD_TYPE=Release",
        f"-DFC_ACTIVE_LOADOUT={loadout}",
    ]
    if sanitizers:
        command.extend(("-DFC_ENABLE_ASAN=ON", "-DFC_ENABLE_UBSAN=ON"))
    recorder.run(f"configure-{loadout}", command)
    build_command = ["cmake", "--build", str(build_dir), f"-j{jobs}"]
    if not all_targets:
        build_command.extend((
            "--target", "parity_fix_core", "parity_fix_replay_linked",
            "parity_fix_soak",
        ))
    recorder.run(f"build-{loadout}", build_command)


def run_loadout_checks(
    recorder: Recorder,
    source: Path,
    build_dir: Path,
    loadout: str,
    seed_count: int,
    tick_budget: int,
    seed_start: int,
    environment: dict[str, str] | None = None,
) -> None:
    core = binary(build_dir, "parity_fix_core")
    linked = binary(build_dir, "parity_fix_replay_linked")
    soak = binary(build_dir, "parity_fix_soak")
    replay_artifact = build_dir / "fc-validation" / "loadout-replay.json"
    failure_trace = build_dir / "fc-validation" / "loadout-soak-failure.json"

    recorder.run(
        f"load004-{loadout}", [str(core), "load_004"], environment
    )
    recorder.run(
        f"replay-{loadout}",
        [
            sys.executable,
            str(source / "fc-validation" / "tests" / "parity_fix_replay.py"),
            "same_version", str(linked), str(replay_artifact),
        ],
        environment,
    )
    recorder.run(
        f"soak-{loadout}",
        [
            str(soak), "--seeds", str(seed_count), "--ticks", str(tick_budget),
            "--seed-start", str(seed_start),
            "--failure-trace", str(failure_trace),
        ],
        environment,
    )


def loadout_matrix(args: argparse.Namespace, recorder: Recorder) -> None:
    for index, loadout in enumerate(LOADOUTS):
        build_dir = args.build_root / loadout.lower()
        configure_and_build(
            recorder, args.source, build_dir, loadout, args.jobs, False
        )
        run_loadout_checks(
            recorder, args.source, build_dir, loadout,
            seed_count=1, tick_budget=500, seed_start=1001 + index * 10000,
        )


def extended_soak(args: argparse.Namespace, recorder: Recorder) -> None:
    base = args.total_seeds // len(LOADOUTS)
    remainder = args.total_seeds % len(LOADOUTS)
    assigned = 0
    for index, loadout in enumerate(LOADOUTS):
        build_dir = args.build_root / loadout.lower()
        if not binary(build_dir, "parity_fix_soak").is_file():
            raise GateFailure(
                f"loadout matrix build is missing for {loadout}: {build_dir}"
            )
        count = base + (1 if index < remainder else 0)
        assigned += count
        soak = binary(build_dir, "parity_fix_soak")
        failure_trace = (
            build_dir / "fc-validation" / "extended-soak-failure.json"
        )
        recorder.run(
            f"extended-soak-{loadout}",
            [
                str(soak), "--seeds", str(count),
                "--ticks", str(args.tick_budget),
                "--seed-start", str(50001 + index * 100000),
                "--failure-trace", str(failure_trace),
            ],
        )
    if assigned != args.total_seeds:
        raise GateFailure(
            f"extended soak assigned {assigned} seeds, expected {args.total_seeds}"
        )


def sanitizer_gate(args: argparse.Namespace, recorder: Recorder) -> None:
    full_build = args.build_root / "full"
    configure_and_build(
        recorder, args.source, full_build, "FC_LOADOUT_SOTA_TBOW",
        args.jobs, True, all_targets=True,
    )
    python_test_regex = "^(" + "|".join(PYTHON_NATIVE_SANITIZER_TESTS) + ")$"
    recorder.run(
        "sanitizer-leak-ctest",
        [
            "ctest", "--test-dir", str(full_build), "--output-on-failure",
            "-E", python_test_regex,
        ],
    )
    recorder.run(
        "sanitizer-python-native-ctest",
        [
            "ctest", "--test-dir", str(full_build), "--output-on-failure",
            "-R", python_test_regex,
        ],
        sanitizer_python_environment(),
    )

    for index, loadout in enumerate(LOADOUTS):
        build_dir = args.build_root / loadout.lower()
        configure_and_build(
            recorder, args.source, build_dir, loadout, args.jobs, True
        )
        run_loadout_checks(
            recorder, args.source, build_dir, loadout,
            seed_count=args.seeds_per_loadout,
            tick_budget=args.tick_budget,
            seed_start=900001 + index * 10000,
        )


def positive_int(text: str) -> int:
    value = int(text)
    if value <= 0:
        raise argparse.ArgumentTypeError("value must be positive")
    return value


def common(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--build-root", type=Path, required=True)
    parser.add_argument("--artifact", type=Path, required=True)
    parser.add_argument("--raw-dir", type=Path)
    parser.add_argument("--jobs", type=positive_int, default=2)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="gate", required=True)

    matrix = subparsers.add_parser("loadout-matrix")
    common(matrix)

    soak = subparsers.add_parser("extended-soak")
    common(soak)
    soak.add_argument("--total-seeds", type=positive_int, default=256)
    soak.add_argument("--tick-budget", type=positive_int, default=200000)

    sanitizer = subparsers.add_parser("sanitizers")
    common(sanitizer)
    sanitizer.add_argument("--seeds-per-loadout", type=positive_int, default=2)
    sanitizer.add_argument("--tick-budget", type=positive_int, default=5000)
    return parser


def main() -> int:
    args = build_parser().parse_args()
    args.source = args.source.resolve()
    args.build_root = args.build_root.resolve()
    args.artifact = args.artifact.resolve()
    raw_dir = (
        args.raw_dir.resolve() if args.raw_dir
        else args.artifact.with_suffix("")
    )
    environment = SANITIZER_ENV if args.gate == "sanitizers" else {}
    recorder = Recorder(raw_dir, environment)
    started = time.monotonic()
    payload: dict[str, Any] = {
        "schema": 1,
        "gate": args.gate,
        "status": "running",
        "source": str(args.source),
        "build_root": str(args.build_root),
        "metadata": metadata(),
        "environment": environment,
    }
    try:
        if args.gate == "loadout-matrix":
            loadout_matrix(args, recorder)
        elif args.gate == "extended-soak":
            extended_soak(args, recorder)
        else:
            sanitizer_gate(args, recorder)
        payload["status"] = "passed"
    except GateFailure as exc:
        payload["status"] = "failed"
        payload["error"] = str(exc)
    payload["elapsed_seconds"] = time.monotonic() - started
    payload["commands"] = recorder.records
    if args.gate == "extended-soak":
        payload["total_seeds"] = args.total_seeds
        payload["tick_budget"] = args.tick_budget
    if args.gate == "sanitizers":
        payload["seeds_per_loadout"] = args.seeds_per_loadout
        payload["tick_budget"] = args.tick_budget
    payload["loadouts"] = list(LOADOUTS)
    write_json(args.artifact, payload)
    if payload["status"] != "passed":
        print(f"FAIL: {payload['error']}", file=sys.stderr)
        return 1
    print(
        f"PASS {args.gate}: commands={len(recorder.records)} "
        f"elapsed={payload['elapsed_seconds']:.2f}s artifact={args.artifact}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
