#!/usr/bin/env python3
"""Export an OSRS-style local-scene minimap raster from cache map data."""

from __future__ import annotations

import argparse
import math
from pathlib import Path

from PIL import Image

from export_terrain import (
    FloorDef,
    RegionTerrain,
    apply_light,
    decode_floor_definitions_modern,
    hsl16_to_rgb,
    hsl_encode,
    load_texture_average_colors_modern,
    parse_terrain_full,
    visual_source_plane,
)
from rc_cache import (
    CONFIG_OBJECT,
    INDEX_CONFIGS,
    INDEX_SPRITES,
    RcCacheStore,
    decode_location_definition,
    find_group_id,
    iter_location_placements,
    read_map_region_file,
)

SCENE_SIZE = 512
TILE_PIXELS = 4
REGION_TILES = 64
SCENE_BORDER_TILES = (SCENE_SIZE // TILE_PIXELS - REGION_TILES) // 2

# Client3 World3D minimap masks. Terrain overlay paths are stored zero-based;
# the scene adds one before selecting one of these 4x4 masks.
OVERLAY_SHAPES = (
    (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
    (1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
    (1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1),
    (1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0),
    (0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1),
    (0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
    (1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1),
    (1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0),
    (0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0),
    (1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1),
    (1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0),
    (0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1),
    (0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1),
)

OVERLAY_ROTATIONS = (
    (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15),
    (12, 8, 4, 0, 13, 9, 5, 1, 14, 10, 6, 2, 15, 11, 7, 3),
    (15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0),
    (3, 7, 11, 15, 2, 6, 10, 14, 1, 5, 9, 13, 0, 4, 8, 12),
)

WALL_COLOR = (238, 238, 238, 255)
DOOR_COLOR = (238, 0, 0, 255)


def _palette_rgb(hsl16: int, brightness: float = 0.8) -> tuple[int, int, int, int]:
    """Resolve packed HSL through the deterministic form of the client palette."""
    red, green, blue = hsl16_to_rgb(hsl16)

    def gamma(channel: int) -> int:
        return max(0, min(255, int(math.pow(channel / 256.0, brightness) * 256.0)))

    return gamma(red), gamma(green), gamma(blue), 255


def _underlay_color(
    terrain: RegionTerrain,
    plane: int,
    tile_x: int,
    tile_y: int,
    underlays: dict[int, FloorDef],
) -> tuple[int, int, int, int]:
    if terrain.underlay_ids[plane][tile_x][tile_y] <= 0:
        return 0, 0, 0, 255

    hue = saturation = lightness = multiplier = count = 0
    for sample_x in range(max(0, tile_x - 5), min(REGION_TILES, tile_x + 6)):
        for sample_y in range(max(0, tile_y - 5), min(REGION_TILES, tile_y + 6)):
            underlay_id = terrain.underlay_ids[plane][sample_x][sample_y]
            definition = underlays.get(underlay_id - 1) if underlay_id > 0 else None
            if definition is None:
                continue
            hue += definition.blend_hue
            saturation += definition.saturation
            lightness += definition.luminance
            multiplier += definition.blend_hue_multiplier
            count += 1

    if count == 0 or multiplier == 0:
        return 0, 0, 0, 255
    packed = hsl_encode(
        (hue * 256) // multiplier,
        saturation // count,
        lightness // count,
    )
    return _palette_rgb(apply_light(packed, 96))


def _overlay_color(
    overlay_id: int,
    overlays: dict[int, FloorDef],
    texture_colors: dict[int, int],
) -> tuple[int, int, int, int]:
    definition = overlays.get(overlay_id - 1)
    if definition is None or definition.rgb == 0xFF00FF:
        return 0, 0, 0, 255

    if definition.secondary_rgb != -1:
        packed = hsl_encode(
            definition.secondary_hue,
            definition.secondary_saturation,
            definition.secondary_lightness,
        )
    elif definition.texture >= 0:
        packed = texture_colors.get(definition.texture, -1)
        if packed < 0:
            return 0, 0, 0, 255
    else:
        packed = hsl_encode(
            definition.hue,
            definition.saturation,
            definition.lightness,
        )
    return _palette_rgb(apply_light(packed, 96))


def _tile_origin(tile_x: int, tile_y: int) -> tuple[int, int]:
    pixel_x = (SCENE_BORDER_TILES + tile_x) * TILE_PIXELS
    pixel_y = (SCENE_BORDER_TILES + REGION_TILES - 1 - tile_y) * TILE_PIXELS
    return pixel_x, pixel_y


def _draw_floor_tile(
    pixels,
    terrain: RegionTerrain,
    plane: int,
    tile_x: int,
    tile_y: int,
    underlays: dict[int, FloorDef],
    overlays: dict[int, FloorDef],
    texture_colors: dict[int, int],
) -> None:
    underlay = _underlay_color(terrain, plane, tile_x, tile_y, underlays)
    overlay_id = terrain.overlay_ids[plane][tile_x][tile_y] & 0xFFFF
    pixel_x, pixel_y = _tile_origin(tile_x, tile_y)
    if overlay_id == 0:
        for dy in range(TILE_PIXELS):
            for dx in range(TILE_PIXELS):
                pixels[pixel_x + dx, pixel_y + dy] = underlay
        return

    overlay = _overlay_color(overlay_id, overlays, texture_colors)
    scene_shape = terrain.shapes[plane][tile_x][tile_y] + 1
    rotation = terrain.rotations[plane][tile_x][tile_y] & 3
    if not 0 <= scene_shape < len(OVERLAY_SHAPES):
        raise ValueError(
            f"unsupported overlay shape {scene_shape} at {tile_x},{tile_y}"
        )
    mask = OVERLAY_SHAPES[scene_shape]
    rotated = OVERLAY_ROTATIONS[rotation]
    for index in range(TILE_PIXELS * TILE_PIXELS):
        dx = index % TILE_PIXELS
        dy = index // TILE_PIXELS
        pixels[pixel_x + dx, pixel_y + dy] = (
            overlay if mask[rotated[index]] else underlay
        )


def _visible_floor_planes(
    terrain: RegionTerrain,
    tile_x: int,
    tile_y: int,
    target_plane: int,
) -> list[int]:
    source_plane = visual_source_plane(terrain, tile_x, tile_y, target_plane)
    result = []
    if terrain.settings[source_plane][tile_x][tile_y] & 0x18 == 0:
        result.append(source_plane)
    upper = target_plane + 1
    if upper < 4 and terrain.settings[upper][tile_x][tile_y] & 0x8:
        result.append(upper)
    return result


def _placement_visible(terrain: RegionTerrain, placement, target_plane: int) -> bool:
    tile_x = placement.local_x
    tile_y = placement.local_y
    display_plane = placement.plane
    if display_plane > 0 and terrain.settings[1][tile_x][tile_y] & 0x2:
        display_plane -= 1
    if display_plane == target_plane:
        return terrain.settings[target_plane][tile_x][tile_y] & 0x18 == 0
    return (
        placement.plane == target_plane + 1
        and terrain.settings[target_plane + 1][tile_x][tile_y] & 0x8 != 0
    )


def _draw_wall(pixels, placement, color: tuple[int, int, int, int]) -> None:
    left, top = _tile_origin(placement.local_x, placement.local_y)
    rotation = placement.rotation
    shape = placement.shape

    def horizontal(y: int) -> None:
        for x in range(TILE_PIXELS):
            pixels[left + x, top + y] = color

    def vertical(x: int) -> None:
        for y in range(TILE_PIXELS):
            pixels[left + x, top + y] = color

    if shape in (0, 2):
        if rotation == 0:
            vertical(0)
        elif rotation == 1:
            horizontal(0)
        elif rotation == 2:
            vertical(TILE_PIXELS - 1)
        else:
            horizontal(TILE_PIXELS - 1)
    if shape == 2:
        if rotation == 0:
            horizontal(0)
        elif rotation == 1:
            vertical(TILE_PIXELS - 1)
        elif rotation == 2:
            horizontal(TILE_PIXELS - 1)
        else:
            vertical(0)
    elif shape == 3:
        corners = ((0, 0), (3, 0), (3, 3), (0, 3))
        dx, dy = corners[rotation]
        pixels[left + dx, top + dy] = color
    elif shape == 9:
        for offset in range(TILE_PIXELS):
            y = TILE_PIXELS - 1 - offset if rotation in (0, 2) else offset
            pixels[left + offset, top + y] = color


def _draw_map_scene(image: Image.Image, placement, definition, sprite) -> None:
    sprite_image = Image.frombytes("RGBA", (sprite.width, sprite.height), sprite.pixels)
    width_tiles = definition.width
    length_tiles = definition.length
    left = (SCENE_BORDER_TILES + placement.local_x) * TILE_PIXELS
    top = (
        SCENE_BORDER_TILES + REGION_TILES - placement.local_y - length_tiles
    ) * TILE_PIXELS
    left += (width_tiles * TILE_PIXELS - sprite.width) // 2
    top += (length_tiles * TILE_PIXELS - sprite.height) // 2
    image.alpha_composite(sprite_image, (left, top))


def export_minimap(
    cache_dir: Path,
    region_x: int,
    region_y: int,
    output: Path,
    target_plane: int = 0,
) -> None:
    """Export one centered 64x64 mapsquare in the client's 512px scene format."""
    store = RcCacheStore(cache_dir)
    terrain_data = read_map_region_file(store, region_x, region_y, "terrain")
    location_data = read_map_region_file(store, region_x, region_y, "locations")
    if terrain_data is None:
        raise SystemExit(f"missing terrain for region {region_x},{region_y}")
    if location_data is None:
        raise SystemExit(f"missing locations for region {region_x},{region_y}")

    terrain = parse_terrain_full(terrain_data, region_x * 64, region_y * 64)
    underlays, overlays = decode_floor_definitions_modern(store)
    texture_colors = load_texture_average_colors_modern(store)

    image = Image.new("RGBA", (SCENE_SIZE, SCENE_SIZE), (0, 0, 0, 255))
    pixels = image.load()
    for tile_y in range(REGION_TILES):
        for tile_x in range(REGION_TILES):
            for plane in _visible_floor_planes(
                terrain, tile_x, tile_y, target_plane
            ):
                _draw_floor_tile(
                    pixels,
                    terrain,
                    plane,
                    tile_x,
                    tile_y,
                    underlays,
                    overlays,
                    texture_colors,
                )

    placements = [
        placement
        for placement in iter_location_placements(
            location_data, region_x, region_y
        )
        if _placement_visible(terrain, placement, target_plane)
    ]
    object_files = store.read_group(INDEX_CONFIGS, CONFIG_OBJECT)
    definitions = {}
    for object_id in sorted({placement.object_id for placement in placements}):
        data = object_files.get(object_id)
        if data is None:
            raise SystemExit(f"missing object definition {object_id}")
        definition = decode_location_definition(object_id, data)
        if not definition.complete:
            raise SystemExit(
                f"object definition {object_id} has unsupported opcode "
                f"{definition.unknown_opcode}"
            )
        definitions[object_id] = definition

    sprite_manifest = store.read_index_manifest(INDEX_SPRITES)
    map_scene_group = find_group_id(sprite_manifest, "mapscene")
    if map_scene_group is None:
        raise SystemExit("cache sprite group 'mapscene' is missing")
    from rc_cache import load_sprite_group

    map_scenes = load_sprite_group(store, map_scene_group)
    wall_count = 0
    map_scene_count = 0
    for placement in placements:
        definition = definitions[placement.object_id]
        uses_map_scene = (
            definition.map_scene_id >= 0
            and placement.shape in (0, 1, 2, 3, 9, 10, 11, 22)
        )
        if uses_map_scene:
            if definition.map_scene_id >= len(map_scenes):
                raise SystemExit(
                    f"object {placement.object_id} references missing map-scene "
                    f"sprite {definition.map_scene_id}"
                )
            _draw_map_scene(
                image,
                placement,
                definition,
                map_scenes[definition.map_scene_id],
            )
            map_scene_count += 1
        elif placement.shape in (0, 2, 3, 9):
            is_door = definition.wall_or_door > 0 or (
                definition.wall_or_door < 0 and any(definition.actions)
            )
            _draw_wall(pixels, placement, DOOR_COLOR if is_door else WALL_COLOR)
            wall_count += 1

    output.parent.mkdir(parents=True, exist_ok=True)
    image.convert("RGB").save(output, format="PNG", optimize=False)
    print(
        f"minimap region {region_x},{region_y}: {SCENE_SIZE}x{SCENE_SIZE}, "
        f"{len(placements)} locations, {wall_count} walls, "
        f"{map_scene_count} map scenes"
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cache", type=Path, required=True)
    parser.add_argument("--region-x", type=int, required=True)
    parser.add_argument("--region-y", type=int, required=True)
    parser.add_argument("--plane", type=int, default=0, choices=range(4))
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    export_minimap(
        args.cache.resolve(),
        args.region_x,
        args.region_y,
        args.output.resolve(),
        args.plane,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
