#!/usr/bin/env python3
"""The equipment state-hash migration must not relax policy compatibility."""
import copy
import json
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "validation"))
import contract_preflight as gate


def main():
    current = gate.load_compiled_contract(sys.argv[1])
    gate.validate_compiled_contract(current)
    preflight = {"contract": current, "contract_identity": gate.contract_identity(current)}
    with tempfile.TemporaryDirectory(prefix="fc-equipment-contract-") as directory:
        root = Path(directory)
        marker_path = root / "contract.json"

        def marker(contract):
            data = {
                "checkpoint_contract_schema_version": 2,
                "artifact_type": "fight_caves_checkpoint_directory",
                "contract": contract,
                "contract_identity": gate.contract_identity(contract),
            }
            marker_path.write_text(json.dumps(data))
            return data

        legacy = dict(current, state_hash_version=4)
        assert current["state_hash_version"] == 7
        magic_legacy = dict(current, state_hash_version=5)
        data = marker(magic_legacy)
        assert gate.validate_checkpoint_marker(marker_path, preflight) == data
        data = marker(legacy)
        assert gate.validate_checkpoint_marker(marker_path, preflight) == data
        checkpoint = root / "weights.bin"
        checkpoint.write_bytes(b"test-only weights")
        resolution = gate.resolve_checkpoint("explicit", root, preflight, checkpoint)
        assert resolution["sidecar_contract_identity"] == gate.contract_identity(legacy)
        resolution_path = root / "resolution.json"
        resolution_path.write_text(json.dumps(resolution))
        assert gate.load_checkpoint_resolution(resolution_path, preflight) == resolution

        for field, value in (
            ("state_hash_version", 3),
            ("puffer_obs_size", 319),
            ("puffer_action_dims", [17, 9, 7]),
            ("observation_version", "different"),
            ("action_version", "different"),
            ("reward_version", "different"),
            ("prayer_timing_version", "different"),
        ):
            invalid = copy.deepcopy(legacy)
            invalid[field] = value
            marker(invalid)
            try:
                gate.validate_checkpoint_marker(marker_path, preflight)
            except gate.ContractError:
                pass
            else:
                raise AssertionError(f"accepted incompatible {field}")
        data = marker(legacy)
        data["contract_identity"] = preflight["contract_identity"]
        marker_path.write_text(json.dumps(data))
        try:
            gate.validate_checkpoint_marker(marker_path, preflight)
        except gate.ContractError:
            pass
        else:
            raise AssertionError("accepted corrupt sidecar self-identity")
    print("equipment checkpoint migration: compatible v4 weights only; strict schema and identity checks passed")


if __name__ == "__main__":
    main()
