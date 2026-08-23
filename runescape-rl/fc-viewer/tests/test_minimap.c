#include "fc_minimap.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define TEST_PI 3.14159265358979323846f

static void fill_tile(Color* pixels, int tile_x, int tile_y, Color color) {
    int left = ((int)FC_MINIMAP_SCENE_BORDER_TILES + tile_x) * 4;
    int top = ((int)FC_MINIMAP_SCENE_BORDER_TILES + FC_ARENA_HEIGHT - 1 -
               tile_y) * 4;
    for (int y = top; y < top + 4; y++) {
        for (int x = left; x < left + 4; x++) {
            pixels[x + y * FC_MINIMAP_SCENE_SIZE] = color;
        }
    }
}

int main(void) {
    Color* source = calloc(
        (size_t)FC_MINIMAP_SCENE_SIZE * FC_MINIMAP_SCENE_SIZE,
        sizeof(*source));
    assert(source);
    fill_tile(source, 2, 3, (Color){120, 80, 40, 255});
    fill_tile(source, 3, 3, (Color){20, 180, 40, 255});
    fill_tile(source, 2, 4, (Color){30, 60, 210, 255});

    FcMinimapScene scene = {0};
    assert(!fc_minimap_scene_load_pixels(&scene, source, 96, 96));
    assert(fc_minimap_scene_load_pixels(
        &scene, source, FC_MINIMAP_SCENE_SIZE, FC_MINIMAP_SCENE_SIZE));
    assert(scene.ready && scene.pixels && scene.pixels != source);
    free(source);

    Color output[FC_MINIMAP_DISPLAY_SIZE * FC_MINIMAP_DISPLAY_SIZE];
    fc_minimap_render(&scene, 2.5f, 3.5f, 0.0f, output);
    Color center = output[76 + 76 * FC_MINIMAP_DISPLAY_SIZE];
    Color east = output[80 + 76 * FC_MINIMAP_DISPLAY_SIZE];
    assert(center.r == 120 && center.g == 80 && center.b == 40);
    assert(east.r == 20 && east.g == 180 && east.b == 40);

    /* The player remains centered while the cache scene scrolls beneath it. */
    fc_minimap_render(&scene, 3.5f, 3.5f, 0.0f, output);
    center = output[76 + 76 * FC_MINIMAP_DISPLAY_SIZE];
    assert(center.r == 20 && center.g == 180 && center.b == 40);

    fc_minimap_render(&scene, 2.5f, 3.5f, TEST_PI * 0.5f, output);
    Color north_at_right = output[80 + 76 * FC_MINIMAP_DISPLAY_SIZE];
    assert(north_at_right.r == 30 && north_at_right.g == 60 &&
           north_at_right.b == 210);

    Vector2 north = fc_minimap_rotate_offset(0.0f, 1.0f,
                                              TEST_PI * 0.5f);
    assert(fabsf(north.x - 1.0f) < 0.0001f);
    assert(fabsf(north.y) < 0.0001f);

    int tile_x = -1;
    int tile_y = -1;
    assert(fc_minimap_click_to_tile(80.0f, 76.0f, 2.5f, 3.5f,
                                    TEST_PI * 0.5f, &tile_x, &tile_y));
    assert(tile_x == 2 && tile_y == 4);
    assert(!fc_minimap_click_to_tile(0.0f, 0.0f, 2.5f, 3.5f, 0.0f,
                                     &tile_x, &tile_y));

    fc_minimap_scene_free(&scene);
    assert(!scene.ready && !scene.pixels);
    puts("minimap tests passed");
    return 0;
}
