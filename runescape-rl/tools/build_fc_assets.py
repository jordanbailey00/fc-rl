#!/usr/bin/env python3
"""Build the Fight Caves runtime asset snapshot from the repo-local cache."""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import re
import shutil
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
PIPELINE_DIR = REPO_ROOT / "tools" / "cache_pipeline"
DEFAULT_CACHE_DIR = (
    PIPELINE_DIR / "source" / "current_fightcaves_demo" / "data" / "cache"
)
DEFAULT_KEYS_PATH = (
    PIPELINE_DIR / "source" / "current_fightcaves_demo" / "data" / "keys.json"
)
DEFAULT_OUT_DIR = REPO_ROOT / "build" / "fc_assets_generated"
LOADOUT_HEADER = REPO_ROOT / "fc-core" / "include" / "fc_player_init.h"

FIGHT_CAVES_REGIONS = [(37, 79)]
FIGHT_CAVES_NPC_IDS = [3116, 3118, 3120, 3121, 3123, 3125, 3127, 3128]
FIGHT_CAVES_SPOTANIM_IDS = [
    15,   # rune arrow travel
    24,   # rune arrow launch
    27,   # standard crossbow bolt travel
    157,  # Jad magic impact
    230,  # adamant dart travel
    236,  # adamant dart launch
    439,  # Jad magic launch
    440,  # Jad ranged launch
    443,  # Tok-Xil ranged travel
    444,  # Tok-Xil ranged impact
    445,  # Ket-Zek magic travel
    446,  # Ket-Zek magic impact
    448,  # Jad magic travel
    451,  # Jad ranged impact
    1116, # dragon arrow launch
    1120, # dragon arrow travel
    1468, # diamond dragon bolt (e) travel
    1922, # corrupted Bowfa travel
    1923, # corrupted Bowfa launch
]
FIGHT_CAVES_ANIM_IDS = {
    426, 808, 819, 820, 821, 822, 823, 824, 829, 836,
    4591, 4226, 4228, 4230, 5061, 7552,
    2618, 2619, 2620, 2621,
    2623, 2624, 2625, 2627,
    2628, 2630, 2631, 2632,
    2634, 2636, 2637, 2638,
    2642, 2643, 2644, 2646,
    2650, 2651, 2652, 2654, 2655, 2656,
}


def _strip_c_comments(text: str) -> str:
    return re.sub(r"/\*.*?\*/|//.*?$", "", text, flags=re.S | re.M)


def _parse_c_int(text: str) -> int:
    token = text.strip().rstrip(",")
    token = re.sub(r"([0-9A-Fa-fx]+)[uUlL]+$", r"\1", token)
    return int(token, 0)


def _parse_loadout_symbols(text: str) -> dict[str, int]:
    symbols: dict[str, int] = {}
    base = re.search(r"#define\s+FC_PLAYER_MODEL_BASE\s+([^\s]+)", text)
    if base:
        symbols["FC_PLAYER_MODEL_BASE"] = _parse_c_int(base.group(1))

    enum = re.search(r"typedef\s+enum\s*\{(?P<body>.*?)\}\s*FcLoadoutId\s*;",
                     text, flags=re.S)
    if enum:
        next_value = 0
        for raw in enum.group("body").split(","):
            line = raw.strip()
            if not line:
                continue
            if "=" in line:
                name, value = [part.strip() for part in line.split("=", 1)]
                next_value = _parse_c_int(value)
            else:
                name = line
            symbols[name] = next_value
            next_value += 1
    return symbols


def _eval_c_int_expr(expr: str, symbols: dict[str, int]) -> int:
    total = 0
    for part in expr.split("+"):
        token = part.strip().rstrip(",")
        if token in symbols:
            total += symbols[token]
        else:
            total += _parse_c_int(token)
    return total


def _extract_initializer_blocks(text: str, name: str) -> list[str]:
    start = text.find(name)
    if start < 0:
        raise SystemExit(f"could not find {name} in {LOADOUT_HEADER}")
    open_idx = text.find("{", start)
    if open_idx < 0:
        raise SystemExit(f"could not find {name} initializer in {LOADOUT_HEADER}")

    depth = 0
    end_idx = -1
    for i in range(open_idx, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                end_idx = i
                break
    if end_idx < 0:
        raise SystemExit(f"unterminated {name} initializer in {LOADOUT_HEADER}")

    content = text[open_idx + 1:end_idx]
    blocks: list[str] = []
    depth = 0
    block_start = -1
    for i, ch in enumerate(content):
        if ch == "{":
            if depth == 0:
                block_start = i
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0 and block_start >= 0:
                blocks.append(content[block_start:i + 1])
                block_start = -1
    return blocks


def load_player_loadouts_from_header() -> list[tuple[int, str, list[int]]]:
    raw = LOADOUT_HEADER.read_text()
    symbols = _parse_loadout_symbols(raw)
    text = _strip_c_comments(raw)
    loadouts: list[tuple[int, str, list[int]]] = []
    for block in _extract_initializer_blocks(text, "FC_LOADOUTS"):
        name = re.search(r"\.name\s*=\s*\"([^\"]+)\"", block)
        model_id = re.search(r"\.player_model_id\s*=\s*([^,\n]+)", block)
        item_ids = re.search(r"\.model_item_ids\s*=\s*\{(?P<ids>.*?)\}",
                             block, flags=re.S)
        item_count = re.search(r"\.model_item_count\s*=\s*(\d+)", block)
        if not (name and model_id and item_ids and item_count):
            raise SystemExit(f"incomplete loadout metadata in {LOADOUT_HEADER}:\n{block}")
        ids = [
            _parse_c_int(piece)
            for piece in item_ids.group("ids").split(",")
            if piece.strip()
        ]
        ids = ids[:int(item_count.group(1))]
        loadouts.append((
            _eval_c_int_expr(model_id.group(1), symbols),
            name.group(1),
            ids,
        ))
    return loadouts


FIGHT_CAVES_PLAYER_LOADOUTS = load_player_loadouts_from_header()


def is_under(path: Path, root: Path) -> bool:
    path = path.resolve()
    root = root.resolve()
    return path == root or root in path.parents


def require_repo_local(path: Path, label: str) -> Path:
    resolved = path.resolve()
    if not is_under(resolved, REPO_ROOT):
        raise SystemExit(f"{label} must be inside {REPO_ROOT}: {resolved}")
    return resolved


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def ensure_pipeline_imports() -> None:
    pipeline = str(PIPELINE_DIR)
    if pipeline not in sys.path:
        sys.path.insert(0, pipeline)


def apply_recolors(model, src_values: list[int], dst_values: list[int]) -> None:
    for src, dst in zip(src_values, dst_values):
        for i, color in enumerate(model.face_colors):
            if color == src:
                model.face_colors[i] = dst


def apply_retextures(model, src_values: list[int], dst_values: list[int]) -> None:
    if not model.face_textures:
        return
    for src, dst in zip(src_values, dst_values):
        for i, texture in enumerate(model.face_textures):
            if texture == src:
                model.face_textures[i] = dst


def scale_model(model, scale_xz: int, scale_y: int) -> None:
    if scale_xz == 128 and scale_y == 128:
        return
    for i in range(model.vertex_count):
        model.vertices_x[i] = int(round(model.vertices_x[i] * scale_xz / 128.0))
        model.vertices_z[i] = int(round(model.vertices_z[i] * scale_xz / 128.0))
        model.vertices_y[i] = int(round(model.vertices_y[i] * scale_y / 128.0))


def export_fc_npc_models(cache_dir: Path, output: Path) -> None:
    from rc_cache import (
        CONFIG_NPC,
        INDEX_CONFIGS,
        RcCacheStore,
        decode_npc_definition,
        load_model,
        merge_models,
        write_models_binary,
    )

    store = RcCacheStore(cache_dir)
    npc_files = store.read_group(INDEX_CONFIGS, CONFIG_NPC)
    models = []

    for npc_id in FIGHT_CAVES_NPC_IDS:
        data = npc_files.get(npc_id)
        if data is None:
            print(f"warning: NPC {npc_id} missing from cache")
            continue
        npc = decode_npc_definition(npc_id, data)
        parts = []
        for model_id in npc.models:
            model = load_model(store, model_id)
            if model is None:
                print(f"warning: NPC {npc_id} model {model_id} missing")
                continue
            apply_recolors(model, npc.recolor_from, npc.recolor_to)
            apply_retextures(model, npc.retexture_from, npc.retexture_to)
            parts.append(model)
        if not parts:
            print(f"warning: NPC {npc_id} has no decoded model parts")
            continue
        merged = parts[0] if len(parts) == 1 else merge_models(parts)
        scale_model(merged, npc.width_scale, npc.height_scale)
        merged.model_id = npc_id
        models.append(merged)
        print(
            f"NPC {npc_id} {npc.name!r}: {len(parts)} parts, "
            f"{merged.vertex_count} verts, {merged.face_count} faces"
        )

    write_models_binary(output, models)


def export_fc_player_model(cache_dir: Path, output: Path) -> None:
    import export_models as model_exporter
    from rc_cache import (
        CONFIG_ITEM,
        INDEX_CONFIGS,
        RcCacheStore,
        decode_item_definition,
        load_model,
        merge_models,
        write_models_binary,
    )

    reader = model_exporter.ModernCacheReader(cache_dir)
    store = RcCacheStore(cache_dir)
    item_files = store.read_group(INDEX_CONFIGS, CONFIG_ITEM)
    all_item_ids = sorted({item_id for _, _, ids in FIGHT_CAVES_PLAYER_LOADOUTS for item_id in ids})
    item_defs = {
        item_id: decode_item_definition(item_id, item_files[item_id])
        for item_id in all_item_ids
        if item_id in item_files
    }
    kit_defs = model_exporter.decode_identity_kits_modern(reader)

    player_models = []
    for synthetic_id, loadout_name, item_ids in FIGHT_CAVES_PLAYER_LOADOUTS:
        parts = []
        keep_body_parts = {1}  # jaw/beard; equipped FC gear supplies the rest.
        for body_part_id in sorted(keep_body_parts):
            kit_id = model_exporter.DEFAULT_MALE_KITS.get(body_part_id)
            kit = kit_defs.get(kit_id) if kit_id is not None else None
            if not kit:
                continue
            kit_parts = []
            for model_id in kit.body_models:
                model = load_model(store, model_id)
                if model:
                    kit_parts.append(model)
            if not kit_parts:
                continue
            body = kit_parts[0] if len(kit_parts) == 1 else merge_models(kit_parts)
            apply_recolors(body, kit.original_colors, kit.replacement_colors)
            parts.append(body)

        for item_id in item_ids:
            item = item_defs.get(item_id)
            if item is None:
                print(f"warning: player item {item_id} missing from item defs")
                continue
            item_parts = []
            for model_id in item.male_model_ids:
                if model_id < 0:
                    continue
                model = load_model(store, model_id)
                if model is None:
                    print(f"warning: player item {item_id} model {model_id} missing")
                    continue
                item_parts.append(model)
            if item_parts:
                parts.append(item_parts[0] if len(item_parts) == 1 else merge_models(item_parts))
                print(f"player {loadout_name} item {item_id} {item.name!r}: {len(item_parts)} parts")

        if not parts:
            raise SystemExit(f"no player model parts decoded for {loadout_name}")
        merged = parts[0] if len(parts) == 1 else merge_models(parts)
        merged.model_id = synthetic_id
        player_models.append(merged)
        print(f"player {loadout_name}: {merged.vertex_count} verts, {merged.face_count} faces")

    write_models_binary(output, player_models)


def export_fc_spotanims(cache_dir: Path, output: Path) -> None:
    import export_spotanims
    from rc_cache import RcCacheStore

    store = RcCacheStore(cache_dir)
    all_defs = export_spotanims.decode_spotanims_modern(store)
    selected = {
        gfx_id: all_defs[gfx_id]
        for gfx_id in FIGHT_CAVES_SPOTANIM_IDS
        if gfx_id in all_defs
    }
    missing = sorted(set(FIGHT_CAVES_SPOTANIM_IDS) - set(selected))
    if missing:
        print(f"warning: spotanims missing from cache: {missing}")
    export_spotanims.write_spotanims_binary(output, selected)


def export_fc_projectile_models(cache_dir: Path, spotanims_bin: Path, output: Path) -> None:
    import export_projectile_models
    from export_textures import build_atlas
    from rc_cache import (
        RcCacheStore,
        load_model,
        load_texture_average_colors,
        load_texture_sprites,
        write_models_binary,
    )

    selected = set(FIGHT_CAVES_SPOTANIM_IDS)
    model_ids = export_projectile_models.read_spotanim_model_ids(spotanims_bin, selected)
    spotanim_ids = export_projectile_models.read_spotanim_ids(spotanims_bin, selected)

    store = RcCacheStore(cache_dir)
    tex_colors = load_texture_average_colors(store)
    atlas = build_atlas(load_texture_sprites(store))
    models = []

    for model_id in model_ids:
        model = load_model(store, model_id)
        if model is None:
            print(f"warning: projectile source model {model_id} missing")
            continue
        models.append(model)
        print(f"projectile source model {model_id}: {model.vertex_count} verts")

    spot_defs = export_projectile_models.selected_spotanim_defs(store, spotanim_ids)
    for gfx_id in spotanim_ids:
        spot = spot_defs.get(gfx_id)
        if spot is None or spot.model_id < 0:
            continue
        model = load_model(store, spot.model_id)
        if model is None:
            print(f"warning: spotanim {gfx_id} model {spot.model_id} missing")
            continue
        model = copy.deepcopy(model)
        model.model_id = export_projectile_models.SPOTANIM_MODEL_BASE + gfx_id
        export_projectile_models.apply_spotanim_recolors(model, spot)
        models.append(model)
        print(f"spotanim {gfx_id}: model {spot.model_id} -> {model.model_id}")

    write_models_binary(output, models, tex_colors=tex_colors, atlas=atlas)


def read_object_animation_ids(oanim_path: Path) -> set[int]:
    import struct

    if not oanim_path.is_file():
        return set()
    data = oanim_path.read_bytes()
    if len(data) < 12:
        return set()
    magic, version, count = struct.unpack_from("<III", data, 0)
    if magic != 0x4D4E414F or version != 1:
        raise SystemExit(f"bad object animation file: {oanim_path}")
    row_fmt = "<IIiiiBBBBffff"
    row_size = struct.calcsize(row_fmt)
    pos = 12
    out: set[int] = set()
    for _ in range(count):
        if pos + row_size > len(data):
            raise SystemExit(f"truncated object animation file: {oanim_path}")
        row = struct.unpack_from(row_fmt, data, pos)
        pos += row_size
        animation_id = int(row[2])
        if animation_id >= 0:
            out.add(animation_id)
    return out


def export_fc_animations(
    cache_dir: Path,
    spotanims_bin: Path,
    object_anims_bin: Path,
    output: Path,
) -> None:
    import export_animations

    needed = set(export_animations.NEEDED_ANIMATIONS)
    needed |= FIGHT_CAVES_ANIM_IDS
    needed |= export_animations.read_spotanim_sequence_ids(spotanims_bin)
    needed |= read_object_animation_ids(object_anims_bin)
    export_animations.export_animations_from_modern_cache(cache_dir, output, needed)


def load_fc_collision_flags(cache_dir: Path):
    from export_collision_map import parse_terrain
    from export_collision_map_modern import (
        decode_modern_obj_defs_rc,
        parse_objects_modern,
    )
    from rc_cache import RcCacheStore, read_map_region_file

    rx, ry = FIGHT_CAVES_REGIONS[0]
    store = RcCacheStore(cache_dir)
    obj_defs = decode_modern_obj_defs_rc(store)
    terrain = read_map_region_file(store, rx, ry, "terrain")
    if terrain is None:
        raise SystemExit(f"missing terrain for region {rx},{ry}")
    flags, down_heights = parse_terrain(terrain)
    locations = read_map_region_file(store, rx, ry, "locations")
    if locations:
        parse_objects_modern(locations, flags, down_heights, obj_defs)
    return rx, ry, flags


def export_fc_collision(cache_dir: Path, output: Path) -> None:
    from export_collision_map import BLOCKED

    rx, ry, flags = load_fc_collision_flags(cache_dir)

    output.parent.mkdir(parents=True, exist_ok=True)
    walkable_count = 0
    with output.open("wb") as f:
        for y in range(64):
            for x in range(64):
                walkable = 0 if (flags[0][x][y] & BLOCKED) else 1
                walkable_count += walkable
                f.write(bytes([walkable]))
    print(f"collision region {rx},{ry}: {walkable_count} walkable tiles")


def export_fc_movement(cache_dir: Path, output: Path) -> None:
    from export_collision_map import (
        WALL_EAST,
        WALL_NORTH,
        WALL_NORTH_EAST,
        WALL_NORTH_WEST,
        WALL_SOUTH,
        WALL_SOUTH_EAST,
        WALL_SOUTH_WEST,
        WALL_WEST,
    )

    rx, ry, flags = load_fc_collision_flags(cache_dir)
    wall_mask = (
        WALL_NORTH_WEST | WALL_NORTH | WALL_NORTH_EAST | WALL_EAST |
        WALL_SOUTH_EAST | WALL_SOUTH | WALL_SOUTH_WEST | WALL_WEST
    )
    output.parent.mkdir(parents=True, exist_ok=True)
    walled_count = 0
    with output.open("wb") as f:
        for y in range(64):
            for x in range(64):
                movement = flags[0][x][y] & wall_mask
                walled_count += movement != 0
                f.write(bytes([movement]))
    print(f"movement region {rx},{ry}: {walled_count} tiles with directional walls")


def export_fc_los(cache_dir: Path, output: Path) -> None:
    from export_collision_map import (
        IMPENETRABLE_BLOCKED,
        IMPENETRABLE_WALL_EAST,
        IMPENETRABLE_WALL_NORTH,
        IMPENETRABLE_WALL_SOUTH,
        IMPENETRABLE_WALL_WEST,
    )

    rx, ry, flags = load_fc_collision_flags(cache_dir)
    output.parent.mkdir(parents=True, exist_ok=True)
    blocked_count = 0
    with output.open("wb") as f:
        for y in range(64):
            for x in range(64):
                source = flags[0][x][y]
                los = 0
                if source & IMPENETRABLE_WALL_NORTH:
                    los |= 1 << 0
                if source & IMPENETRABLE_WALL_EAST:
                    los |= 1 << 1
                if source & IMPENETRABLE_WALL_SOUTH:
                    los |= 1 << 2
                if source & IMPENETRABLE_WALL_WEST:
                    los |= 1 << 3
                if source & IMPENETRABLE_BLOCKED:
                    los |= 1 << 4
                blocked_count += los != 0
                f.write(bytes([los]))
    print(f"LOS region {rx},{ry}: {blocked_count} tiles with projectile flags")


def alias_fc_sprites(sprites_dir: Path) -> None:
    aliases = {
        "protect_magic_on.png": "prayeron_12.png",
        "protect_magic_off.png": "prayeroff_12.png",
        "protect_missiles_on.png": "prayeron_13.png",
        "protect_missiles_off.png": "prayeroff_13.png",
        "protect_melee_on.png": "prayeron_14.png",
        "protect_melee_off.png": "prayeroff_14.png",
        "tab_combat.png": "side_icon_combat.png",
        "tab_inventory.png": "side_icon_inventory.png",
        "tab_prayer.png": "side_icon_prayer.png",
    }
    for dst_name, src_name in aliases.items():
        src = sprites_dir / src_name
        dst = sprites_dir / dst_name
        if src.exists():
            shutil.copy2(src, dst)


def carry_forward_legacy_item_sprites(sprites_dir: Path) -> None:
    """Keep viewer-required item icons until a cache model icon renderer exists."""
    for name in ("shark.png", "prayer_potion.png"):
        dst = sprites_dir / name
        src = REPO_ROOT / "fc-viewer" / "assets" / "sprites" / name
        if not dst.exists() and src.exists():
            shutil.copy2(src, dst)


FC_VIEWER_COMPAT_SPRITES = {
    "prayer_potion.png",
    "protect_magic_off.png",
    "protect_magic_on.png",
    "protect_melee_off.png",
    "protect_melee_on.png",
    "protect_missiles_off.png",
    "protect_missiles_on.png",
    "shark.png",
    "tab_combat.png",
    "tab_inventory.png",
    "tab_prayer.png",
}


def prune_unreferenced_compat_sprites(sprites_dir: Path) -> None:
    """Keep only the legacy top-level sprites consumed by the viewer."""
    for path in sprites_dir.iterdir():
        if path.is_file() and path.name not in FC_VIEWER_COMPAT_SPRITES:
            path.unlink()


def export_fc_sprites(cache_dir: Path, output: Path) -> None:
    import export_sprites_modern

    output.mkdir(parents=True, exist_ok=True)
    rc = export_sprites_modern.main(["--cache", str(cache_dir), "--output", str(output)])
    if rc != 0:
        raise SystemExit(f"sprite export failed with exit code {rc}")
    alias_fc_sprites(output)
    carry_forward_legacy_item_sprites(output)
    prune_unreferenced_compat_sprites(output)


def build_manifest(staging_root: Path, cache_dir: Path) -> dict[str, object]:
    files = []
    generated_roots = [
        staging_root / "fc-viewer" / "assets",
        staging_root / "fc-core" / "assets",
    ]
    for root in generated_roots:
        if not root.exists():
            continue
        for path in sorted(p for p in root.rglob("*") if p.is_file()):
            rel = path.relative_to(staging_root).as_posix()
            runtime = REPO_ROOT / rel
            generated_sha = sha256_file(path)
            entry = {
                "path": rel,
                "size": path.stat().st_size,
                "sha256": generated_sha,
            }
            if runtime.exists():
                current_sha = sha256_file(runtime)
                entry["current_sha256"] = current_sha
                entry["compare"] = "same" if current_sha == generated_sha else "different"
            else:
                entry["compare"] = "missing_in_runtime"
            files.append(entry)
    return {
        "schema": 1,
        "repo_root": str(REPO_ROOT),
        "cache": str(cache_dir.relative_to(REPO_ROOT)),
        "regions": [f"{rx},{ry}" for rx, ry in FIGHT_CAVES_REGIONS],
        "npc_ids": FIGHT_CAVES_NPC_IDS,
        "spotanim_ids": FIGHT_CAVES_SPOTANIM_IDS,
        "player_loadouts": [
            {"model_id": model_id, "name": name, "item_ids": item_ids}
            for model_id, name, item_ids in FIGHT_CAVES_PLAYER_LOADOUTS
        ],
        "files": files,
    }


def replace_runtime_assets(staging_root: Path, manifest: dict[str, object]) -> None:
    for entry in manifest["files"]:
        rel = Path(entry["path"])
        src = staging_root / rel
        dst = REPO_ROOT / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)
        print(f"replaced {rel.as_posix()}")


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cache-dir", type=Path, default=DEFAULT_CACHE_DIR)
    parser.add_argument("--keys", type=Path, default=DEFAULT_KEYS_PATH)
    parser.add_argument("--out-dir", type=Path, default=DEFAULT_OUT_DIR)
    parser.add_argument(
        "--replace",
        action="store_true",
        help="copy generated files into fc-viewer/assets and fc-core/assets",
    )
    parser.add_argument(
        "--no-clean",
        action="store_true",
        help="do not clear the staging output directory before generation",
    )
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    cache_dir = require_repo_local(args.cache_dir, "cache directory")
    keys_path = require_repo_local(args.keys, "keys file")
    out_dir = require_repo_local(args.out_dir, "output directory")

    if not cache_dir.exists():
        raise SystemExit(f"cache directory not found: {cache_dir}")
    if not keys_path.exists():
        raise SystemExit(f"keys file not found: {keys_path}")
    if out_dir == REPO_ROOT:
        raise SystemExit("output directory cannot be the repo root")

    ensure_pipeline_imports()

    if out_dir.exists() and not args.no_clean:
        shutil.rmtree(out_dir)
    assets_dir = out_dir / "fc-viewer" / "assets"
    core_assets_dir = out_dir / "fc-core" / "assets"
    sprites_dir = assets_dir / "sprites"
    assets_dir.mkdir(parents=True, exist_ok=True)
    core_assets_dir.mkdir(parents=True, exist_ok=True)

    from export_objects import export_modern_objects
    from export_terrain import export_modern_terrain

    export_modern_terrain(
        cache_dir=cache_dir,
        regions=FIGHT_CAVES_REGIONS,
        output=assets_dir / "fightcaves.terrain",
        scene_plane=0,
        brightness=1.0,
    )
    export_modern_objects(
        cache_dir=cache_dir,
        regions=FIGHT_CAVES_REGIONS,
        output=assets_dir / "fightcaves.objects",
        scene_plane=0,
        rsmod_visual_levels=True,
    )
    export_fc_npc_models(cache_dir, assets_dir / "fc_npcs.models")
    export_fc_player_model(cache_dir, assets_dir / "fc_player.models")
    export_fc_spotanims(cache_dir, assets_dir / "fc_spotanims.bin")
    export_fc_projectile_models(
        cache_dir,
        assets_dir / "fc_spotanims.bin",
        assets_dir / "fc_projectiles.models",
    )
    export_fc_animations(
        cache_dir,
        assets_dir / "fc_spotanims.bin",
        assets_dir / "fightcaves.oanim",
        assets_dir / "fc_all.anims",
    )
    export_fc_sprites(cache_dir, sprites_dir)
    export_fc_collision(cache_dir, core_assets_dir / "fightcaves.collision")
    export_fc_movement(cache_dir, core_assets_dir / "fightcaves.movement")
    export_fc_los(cache_dir, core_assets_dir / "fightcaves.los")

    manifest = build_manifest(out_dir, cache_dir)
    manifest_path = out_dir / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"wrote manifest: {manifest_path}")

    counts = {}
    for entry in manifest["files"]:
        counts[entry["compare"]] = counts.get(entry["compare"], 0) + 1
    print("compare:", ", ".join(f"{k}={v}" for k, v in sorted(counts.items())))

    if args.replace:
        replace_runtime_assets(out_dir, manifest)
    else:
        print("runtime assets were not replaced; pass --replace after reviewing the manifest")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
