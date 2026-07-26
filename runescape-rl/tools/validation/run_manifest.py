#!/usr/bin/env python3
"""Write a Fight Caves training run manifest.

The manifest is intentionally outside fc-core: it documents the training setup
without changing simulator behavior.
"""

from __future__ import annotations

import argparse
import ast
import configparser
from datetime import datetime, timezone
import hashlib
import json
import os
import platform
import shutil
import subprocess
import sys
from typing import Any


def sha256_file(path: str) -> str | None:
    if not path or not os.path.isfile(path):
        return None
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def parse_value(value: str) -> Any:
    try:
        return ast.literal_eval(value)
    except (SyntaxError, ValueError):
        return value


def read_config(path: str) -> dict[str, dict[str, Any]]:
    parser = configparser.ConfigParser()
    parser.read(path)
    return {
        section: {key: parse_value(value) for key, value in parser[section].items()}
        for section in parser.sections()
    }


def merge_configs(*configs: dict[str, dict[str, Any]]) -> dict[str, dict[str, Any]]:
    merged: dict[str, dict[str, Any]] = {}
    for config in configs:
        for section, values in config.items():
            merged.setdefault(section, {}).update(values)
    return merged


def read_key_value_file(path: str) -> dict[str, str]:
    if not path or not os.path.isfile(path):
        return {}
    values: dict[str, str] = {}
    with open(path, "r", encoding="utf-8") as f:
        for raw in f:
            line = raw.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, value = line.split("=", 1)
            values[key] = value
    return values


def git(repo_root: str, *args: str) -> str | None:
    try:
        proc = subprocess.run(
            ["git", "-C", repo_root, *args],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
        )
    except OSError:
        return None
    if proc.returncode != 0:
        return None
    return proc.stdout.strip()


def python_version(python_bin: str) -> str | None:
    if not python_bin:
        return None
    try:
        proc = subprocess.run(
            [python_bin, "-c", "import sys; print(sys.version.replace('\\n', ' '))"],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
        )
    except OSError:
        return None
    return proc.stdout.strip() if proc.returncode == 0 else None


def maybe_pufferlib_version(puffer_dir: str) -> str | None:
    init_path = os.path.join(puffer_dir, "pufferlib", "__init__.py")
    if not os.path.isfile(init_path):
        return None
    with open(init_path, "r", encoding="utf-8") as f:
        for line in f:
            if line.startswith("__version__"):
                _, value = line.split("=", 1)
                return parse_value(value.strip())
    return None


def selected_env() -> dict[str, str]:
    names = [
        "CONFIG_PATH",
        "FC_ACTIVE_LOADOUT",
        "FC_RUN_MANIFEST_PATH",
        "FORCE_BACKEND_REBUILD",
        "LOAD_MODEL_PATH",
        "NVCC_ARCH",
        "PUFFER_DIR",
        "PYTHON_BIN",
        "WANDB_PROJECT",
        "WANDB_TAG",
    ]
    return {name: os.environ[name] for name in names if name in os.environ}


def command_from_remainder(remainder: list[str]) -> list[str]:
    if remainder and remainder[0] == "--":
        return remainder[1:]
    return remainder


def build_manifest(args: argparse.Namespace) -> dict[str, Any]:
    default_config_path = os.path.join(args.puffer_dir, "config", "default.ini")
    default_config = read_config(default_config_path) if os.path.isfile(default_config_path) else {}
    config = read_config(args.config_path)
    synced_config = read_config(args.synced_config_path) if args.synced_config_path else {}
    override_config = synced_config or config
    effective_config = merge_configs(default_config, override_config)
    env_cfg = effective_config.get("env", {})
    run_cfg = effective_config.get("run", {})
    train_cfg = effective_config.get("train", {})
    vec_cfg = effective_config.get("vec", {})
    base_cfg = effective_config.get("base", {})
    backend_stamp = read_key_value_file(args.backend_stamp)
    git_status = git(args.repo_root, "status", "--short")

    resolved = {
        "env_name": base_cfg.get("env_name", "fight_caves"),
        "active_loadout": args.active_loadout,
        "initial_sharks": env_cfg.get("initial_sharks"),
        "initial_prayer_doses": env_cfg.get("initial_prayer_doses"),
        "total_agents": vec_cfg.get("total_agents"),
        "num_buffers": vec_cfg.get("num_buffers"),
        "num_threads": vec_cfg.get("num_threads"),
        "total_timesteps": train_cfg.get("total_timesteps"),
        "train_seed": train_cfg.get("seed"),
        "base_seed": base_cfg.get("seed"),
        "horizon": train_cfg.get("horizon"),
        "minibatch_size": train_cfg.get("minibatch_size"),
    }

    return {
        "schema_version": 1,
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "mode": args.mode,
        "command": command_from_remainder(args.command),
        "paths": {
            "repo_root": os.path.abspath(args.repo_root),
            "runescape_dir": os.path.abspath(args.runescape_dir),
            "puffer_dir": os.path.abspath(args.puffer_dir),
            "config_path": os.path.abspath(args.config_path),
            "synced_config_path": os.path.abspath(args.synced_config_path),
            "default_config_path": os.path.abspath(default_config_path),
            "backend_stamp": os.path.abspath(args.backend_stamp),
        },
        "git": {
            "branch": git(args.repo_root, "branch", "--show-current"),
            "commit": git(args.repo_root, "rev-parse", "HEAD"),
            "commit_short": git(args.repo_root, "rev-parse", "--short", "HEAD"),
            "dirty": bool(git_status),
            "status_short": git_status.splitlines() if git_status else [],
        },
        "config": {
            "default_sha256": sha256_file(default_config_path),
            "source_sha256": sha256_file(args.config_path),
            "synced_sha256": sha256_file(args.synced_config_path),
            "default_sections": default_config,
            "override_sections": override_config,
            "effective_sections": effective_config,
        },
        "resolved": resolved,
        "trainer_contract": {
            "observation_version": run_cfg.get("observation_version"),
            "action_version": run_cfg.get("action_version"),
            "reward_version": run_cfg.get("reward_version"),
            "reward_clip_enabled": bool(run_cfg.get("reward_clip_enabled", True)),
            "reward_clip_min": run_cfg.get("reward_clip_min", -1.0),
            "reward_clip_max": run_cfg.get("reward_clip_max", 1.0),
            "reward_clip_source": "pufferlib_4/src/pufferlib.cu rollout reward clamp",
            "action_masks_enforced_by_trainer": True,
            "action_masks_note": (
                "PufferLib natively applies the environment mask during rollout "
                "sampling and PPO log-probability/entropy recomputation."
            ),
        },
        "backend": {
            "active_loadout": args.active_loadout,
            "stamp": backend_stamp,
            "stamp_sha256": sha256_file(args.backend_stamp),
        },
        "runtime": {
            "python_bin": args.python_bin,
            "python_version": python_version(args.python_bin),
            "pufferlib_version": maybe_pufferlib_version(args.puffer_dir),
            "platform": platform.platform(),
            "hostname": platform.node(),
            "cwd": os.getcwd(),
            "env": selected_env(),
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Write a Fight Caves run manifest")
    parser.add_argument("--repo-root", required=True)
    parser.add_argument("--runescape-dir", required=True)
    parser.add_argument("--puffer-dir", required=True)
    parser.add_argument("--config-path", required=True)
    parser.add_argument("--synced-config-path", required=True)
    parser.add_argument("--backend-stamp", required=True)
    parser.add_argument("--active-loadout", required=True)
    parser.add_argument("--python-bin", required=True)
    parser.add_argument("--mode", required=True)
    parser.add_argument("--output-path", required=True)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()

    manifest = build_manifest(args)
    output_path = os.path.abspath(args.output_path)
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2, sort_keys=True)
        f.write("\n")

    latest_path = os.path.join(os.path.dirname(output_path), "latest.json")
    if os.path.abspath(latest_path) != output_path:
        shutil.copyfile(output_path, latest_path)
    print(output_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
