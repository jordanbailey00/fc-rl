#!/usr/bin/env python3
"""Fight Caves build helpers.

setup installs the asset bundles pinned in resources/fight_caves/asset_manifest.json.
cflags prints the compiler flags for the capabilities set in the [env] section
of config/fight_caves.ini; build.sh passes them to every Fight Caves compile.

Usage: python3 ocean/fight_caves/tools.py setup [--force | --verify-only]
       python3 ocean/fight_caves/tools.py cflags [--config PATH]
"""

from __future__ import annotations

import argparse
import configparser
import hashlib
from http.client import HTTPException
import json
from pathlib import Path
import shutil
import sys
import tarfile
import tempfile
from urllib.request import Request, urlopen

REPO_ROOT = Path(__file__).resolve().parents[2]
RESOURCE_ROOT = REPO_ROOT / "resources" / "fight_caves"
MANIFEST = RESOURCE_ROOT / "asset_manifest.json"
# Written into each installed bundle directory: the SHA-256 of its archive.
STAMP = ".archive-sha256"
# Capabilities by [env] key: each value's name and the number compiled in as
# FC_CAP_<KEY>; a missing key takes the first. binding.c's
# FC_BUILT_CAPABILITIES has the same names, and puf_init checks the INI it
# trains with against the build.
CAPABILITIES = {"consumables": {"0": 0, "1": 1},
                "prayers": {"protection": 0, "all": 1},
                "gear_sets": {"1": 1, "2": 2, "3": 3, "4": 4},
                "spellbook": {"none": 0, "standard": 1, "ancient": 2},
                "special_attacks": {"0": 0, "1": 1}}


class AssetError(RuntimeError):
    pass


class ConfigError(RuntimeError):
    pass


def capability_flags(config: Path) -> list[str]:
    """Reads [env] the way PufferLib does: config/default.ini, then config."""
    if not config.is_file():
        raise ConfigError(f"{config} not found")
    parser = configparser.ConfigParser(inline_comment_prefixes=("#", ";"),
                                       interpolation=None, strict=False)
    parser.read([REPO_ROOT / "config" / "default.ini", config])
    env = parser["env"] if parser.has_section("env") else {}
    flags = []
    for key, choices in CAPABILITIES.items():
        raw = env.get(key, next(iter(choices))).strip()
        name = capability_name(raw)
        if name not in choices:
            raise ConfigError(f"{key} = {raw}: must be {' or '.join(choices)}")
        flags.append(f"-DFC_CAP_{key.upper()}={choices[name]}")
    return flags


def capability_name(raw: str) -> str:
    """Numbers the way PufferLib parses them, so 1, 1.0 and true agree."""
    text = raw.lower()
    if text in ("true", "false"):
        return "1" if text == "true" else "0"
    try:
        number = float(text.replace("_", ""))
    except ValueError:
        return text
    return str(int(number)) if number.is_integer() else text


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        while chunk := handle.read(1024 * 1024):
            digest.update(chunk)
    return digest.hexdigest()


def is_installed(bundle: dict) -> bool:
    stamp = RESOURCE_ROOT / bundle["install_prefix"] / STAMP
    return stamp.is_file() and stamp.read_text().strip() == bundle["sha256"]


def download(url: str, destination: Path) -> None:
    print(f"Downloading {url}")
    request = Request(url, headers={"User-Agent": "PufferLib-Fight-Caves-assets/1"})
    try:
        with urlopen(request, timeout=60) as response, destination.open("wb") as out:
            shutil.copyfileobj(response, out)
    except (OSError, HTTPException) as exc:
        raise AssetError(f"download failed for {url}: {exc}") from exc


def install_bundle(bundle: dict) -> None:
    """Download and verify the archive, extract it, then swap the stamped
    directory in. A failure before the swap leaves the current install alone."""
    archive_name, prefix = bundle["archive"], bundle["install_prefix"]
    # Stage beside the install so the swap is a rename on one filesystem.
    with tempfile.TemporaryDirectory(prefix=".download-", dir=RESOURCE_ROOT) as temp_name:
        temp = Path(temp_name)
        archive = temp / archive_name
        download(bundle["url"], archive)
        if sha256_file(archive) != bundle["sha256"]:
            raise AssetError(f"checksum mismatch for downloaded {archive_name}")
        try:
            with tarfile.open(archive, "r:gz") as source:
                # The data filter rejects absolute paths, paths that leave the
                # destination, links out of it and special files.
                source.extractall(temp / "extracted", filter="data")
        except (OSError, tarfile.TarError) as exc:
            raise AssetError(f"could not extract {archive_name}: {exc}") from exc
        staged = temp / "extracted" / prefix
        if not staged.is_dir():
            raise AssetError(f"{archive_name} has no {prefix}/ directory")
        (staged / STAMP).write_text(bundle["sha256"] + "\n")
        destination = RESOURCE_ROOT / prefix
        if destination.exists():
            destination.rename(temp / "previous")
        staged.rename(destination)
    print(f"Installed Fight Caves {prefix} assets from {archive_name}.")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Install the pinned Fight Caves asset bundles, or print the "
                    "compiler flags for the configured capabilities."
    )
    parser.add_argument("command", choices=["setup", "cflags"])
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--force", action="store_true",
                      help="setup: download and reinstall every bundle")
    mode.add_argument("--verify-only", action="store_true",
                      help="setup: check the installed stamps without downloading")
    parser.add_argument("--config", type=Path,
                        default=REPO_ROOT / "config" / "fight_caves.ini",
                        help="cflags: the INI whose [env] capabilities to compile in")
    args = parser.parse_args(argv)
    if args.command == "cflags":
        try:
            print(" ".join(capability_flags(args.config)))
        except (ConfigError, configparser.Error) as exc:
            print(f"Fight Caves config error: {exc}", file=sys.stderr)
            return 1
        return 0
    try:
        for bundle in json.loads(MANIFEST.read_text(encoding="utf-8"))["bundles"]:
            prefix, archive_name = bundle["install_prefix"], bundle["archive"]
            if is_installed(bundle) and not args.force:
                print(f"Fight Caves {prefix} assets are installed from {archive_name}.")
            elif args.verify_only:
                raise AssetError(f"{prefix}/ is missing or was not installed from "
                                 f"{archive_name}; run "
                                 "python3 ocean/fight_caves/tools.py setup")
            else:
                install_bundle(bundle)
    except (AssetError, OSError) as exc:
        print(f"Fight Caves asset setup failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
