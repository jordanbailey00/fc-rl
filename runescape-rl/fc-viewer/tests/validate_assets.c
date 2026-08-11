#include "fc_assets.h"

#include <stdio.h>

static int check_asset(const char* path) {
    char resolved[FC_ASSET_PATH_MAX];
    if (!fc_asset_resolve_path(path, resolved, sizeof(resolved))) {
        fprintf(stderr, "missing asset: %s (looked for %s)\n", path, resolved);
        return 0;
    }
    printf("asset: %s -> %s\n", path, resolved);
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
        NULL
    };
    int ok = 1;

    printf("asset root: %s\n", fc_asset_root());
    printf("repo root:  %s\n", fc_repo_root());

    for (int i = 0; assets[i]; i++)
        ok = check_asset(assets[i]) && ok;

    ok = check_repo_file("fc-core/assets/fightcaves.collision") && ok;
    ok = check_repo_file("fc-core/assets/fightcaves.movement") && ok;
    ok = check_repo_file("fc-core/assets/fightcaves.los") && ok;
    return ok ? 0 : 1;
}
