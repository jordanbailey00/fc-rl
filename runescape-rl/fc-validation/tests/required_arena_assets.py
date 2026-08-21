#!/usr/bin/env python3

import os
from pathlib import Path
import subprocess
import sys
import tempfile


ASSETS = {
    "FC_COLLISION_PATH": "fightcaves.collision",
    "FC_MOVEMENT_PATH": "fightcaves.movement",
    "FC_LOS_PATH": "fightcaves.los",
}


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: required_arena_assets.py <probe> <asset-dir>", file=sys.stderr)
        return 2

    probe = Path(sys.argv[1]).resolve()
    asset_dir = Path(sys.argv[2]).resolve()

    with tempfile.TemporaryDirectory(prefix="fc-missing-arena-assets-") as workdir:
        missing_path = Path(workdir) / "does-not-exist"
        for missing_var, expected_asset in ASSETS.items():
            env = os.environ.copy()
            for variable, filename in ASSETS.items():
                env[variable] = str(asset_dir / filename)
            env[missing_var] = str(missing_path)

            result = subprocess.run(
                [probe],
                cwd=workdir,
                env=env,
                capture_output=True,
                text=True,
                check=False,
            )
            expected = (
                f"fatal: required Fight Caves arena asset '{expected_asset}'"
            )
            if result.returncode == 0 or expected not in result.stderr:
                print(
                    f"FAIL: missing {expected_asset} did not fail clearly\n"
                    f"returncode={result.returncode}\n"
                    f"stdout={result.stdout}\n"
                    f"stderr={result.stderr}",
                    file=sys.stderr,
                )
                return 1

    print("PASS: collision, movement, and LOS assets are all required")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
