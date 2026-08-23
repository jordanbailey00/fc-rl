#include "fc_assets.h"

#include <stdio.h>
#include <string.h>

static int check_asset(const char* path) {
    char resolved[FC_ASSET_PATH_MAX];
    if (!fc_asset_resolve_path(path, resolved, sizeof(resolved))) {
        fprintf(stderr, "missing asset: %s (looked for %s)\n", path, resolved);
        return 0;
    }
    printf("asset: %s -> %s\n", path, resolved);
    return 1;
}

static unsigned int read_png_u32(const unsigned char* bytes) {
    return ((unsigned int)bytes[0] << 24) |
           ((unsigned int)bytes[1] << 16) |
           ((unsigned int)bytes[2] << 8) |
           (unsigned int)bytes[3];
}

static int check_png_size(const char* path, unsigned int expected_width,
                          unsigned int expected_height) {
    static const unsigned char png_signature[8] = {
        0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'
    };
    char resolved[FC_ASSET_PATH_MAX];
    unsigned char header[24];
    if (!fc_asset_resolve_path(path, resolved, sizeof(resolved))) {
        fprintf(stderr, "missing PNG asset: %s\n", path);
        return 0;
    }

    FILE* file = fopen(resolved, "rb");
    if (!file) {
        fprintf(stderr, "unable to open PNG asset: %s\n", resolved);
        return 0;
    }
    size_t bytes_read = fread(header, 1, sizeof(header), file);
    fclose(file);

    if (bytes_read != sizeof(header) ||
        memcmp(header, png_signature, sizeof(png_signature)) != 0 ||
        memcmp(header + 12, "IHDR", 4) != 0) {
        fprintf(stderr, "invalid PNG asset: %s\n", resolved);
        return 0;
    }

    unsigned int width = read_png_u32(header + 16);
    unsigned int height = read_png_u32(header + 20);
    if (width != expected_width || height != expected_height) {
        fprintf(stderr,
                "unexpected PNG size: %s is %ux%u, expected %ux%u\n",
                resolved, width, height, expected_width, expected_height);
        return 0;
    }
    return 1;
}

static int check_repo_file(const char* path) {
    char resolved[FC_ASSET_PATH_MAX];
    if (!fc_repo_resolve_path(path, resolved, sizeof(resolved))) {
        fprintf(stderr, "missing repo file: %s (looked for %s)\n", path, resolved);
        return 0;
    }
    printf("repo:  %s -> %s\n", path, resolved);
    return 1;
}

int main(void) {
    const char* assets[] = {
        "fightcaves.terrain",
        "fightcaves.objects",
        "fightcaves.atlas",
        "fc_npcs.models",
        "fc_player.models",
        "fc_projectiles.models",
        "fc_spotanims.bin",
        "fc_all.anims",
        "sprites/shark.png",
        "sprites/prayer_potion.png",
        "sprites/protect_melee_on.png",
        "sprites/protect_missiles_on.png",
        "sprites/protect_magic_on.png",
        "sprites/tab_inventory.png",
        "sprites/tab_combat.png",
        "sprites/tab_prayer.png",
        "data/sprites/ui/hitsplat_zero.png",
        "data/sprites/ui/hitsplat_damage.png",
        "data/sprites/ui/hitsplat_heal.png",
        "data/sprites/ui/hitsplat_prayer_drain.png",
        "data/sprites/ui/healthbar_full_30.png",
        "data/sprites/ui/healthbar_empty_30.png",
        NULL
    };
    int ok = 1;

    printf("asset root: %s\n", fc_asset_root());
    printf("repo root:  %s\n", fc_repo_root());

    for (int i = 0; assets[i]; i++)
        ok = check_asset(assets[i]) && ok;

    ok = check_png_size("data/sprites/ui/hitsplat_zero.png", 25, 25) && ok;
    ok = check_png_size("data/sprites/ui/hitsplat_damage.png", 25, 25) && ok;
    ok = check_png_size("data/sprites/ui/hitsplat_heal.png", 25, 25) && ok;
    ok = check_png_size("data/sprites/ui/hitsplat_prayer_drain.png", 25, 25) && ok;
    ok = check_png_size("data/sprites/ui/healthbar_full_30.png", 30, 5) && ok;
    ok = check_png_size("data/sprites/ui/healthbar_empty_30.png", 30, 5) && ok;

    ok = check_repo_file("fc-core/assets/fightcaves.collision") && ok;
    ok = check_repo_file("fc-core/assets/fightcaves.movement") && ok;
    ok = check_repo_file("fc-core/assets/fightcaves.los") && ok;
    return ok ? 0 : 1;
}
