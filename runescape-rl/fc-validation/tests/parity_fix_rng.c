#include "fc_api.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int run_prayer_case(int command) {
    FcState state;
    fc_init(&state);
    fc_reset(&state, UINT32_C(0x13579bdf));
    memset(state.npcs, 0, sizeof(state.npcs));
    state.npcs_remaining = 1;
    state.player.weapon_uses_ammo = 1;
    state.player.ammo_count = 0;
    state.player.prayer = PRAYER_PROTECT_MAGIC;
    state.player.current_prayer = state.player.max_prayer;
    state.player.prayer_drain_counter = 31;
    fc_rng_seed(&state, UINT32_C(0x2468ace1));
    uint32_t expected = state.rng_state;
    int actions[FC_NUM_ACTION_HEADS] = {0};
    actions[2] = command;
    fc_tick(&state, actions);
    if (state.rng_state != expected) {
        fprintf(stderr,
                "FAIL DET-004: prayer command %d consumed RNG: %08x -> %08x\n",
                command, expected, state.rng_state);
        fc_destroy(&state);
        return 1;
    }
    fc_destroy(&state);
    return 0;
}

int main(void) {
    static const int commands[] = {
        FC_PRAYER_NO_CHANGE,
        FC_PRAYER_FLICK_MAGIC,
        FC_PRAYER_FLICK_RANGE,
        FC_PRAYER_FLICK_MELEE,
    };
    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        if (run_prayer_case(commands[i])) return 1;
    }
    printf("PASS DET-004: no-change and flick prayer actions consume no RNG\n");
    return 0;
}
