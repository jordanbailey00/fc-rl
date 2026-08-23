#include "fc_click_feedback.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void make_open_state(FcState* state) {
    memset(state, 0, sizeof(*state));
    for (int x = 0; x < FC_ARENA_WIDTH; x++) {
        for (int y = 0; y < FC_ARENA_HEIGHT; y++) {
            state->walkable[x][y] = 1;
        }
    }
    state->player.x = 1;
    state->player.y = 1;
}

int main(void) {
    FcState state;
    FcClickFeedback feedback;
    const int* route_x = NULL;
    const int* route_y = NULL;
    int start = -1;
    int len = -1;

    make_open_state(&state);
    fc_click_feedback_reset(&feedback);
    fc_click_feedback_select_move(&feedback, &state, 5, 3, 120.0f, 80.0f);

    assert(feedback.destination_active);
    assert(feedback.destination_x == 5 && feedback.destination_y == 3);
    assert(feedback.preview_pending);
    assert(feedback.preview_route_len > 0);
    assert(feedback.preview_route_x[feedback.preview_route_len - 1] == 5);
    assert(feedback.preview_route_y[feedback.preview_route_len - 1] == 3);
    assert(feedback.cross_kind == FC_CLICK_CROSS_MOVE);
    assert(fc_click_feedback_cross_frame(&feedback) == 0);
    assert(fc_click_feedback_route(&feedback, &state, &route_x, &route_y,
                                   &start, &len));
    assert(route_x == feedback.preview_route_x);
    assert(route_y == feedback.preview_route_y);
    assert(start == 0 && len == feedback.preview_route_len);

    fc_click_feedback_update(&feedback, 0.11f);
    assert(fc_click_feedback_cross_frame(&feedback) == 1);
    fc_click_feedback_update(&feedback, 0.30f);
    assert(feedback.cross_kind == FC_CLICK_CROSS_NONE);

    state.player.route_x[0] = 2;
    state.player.route_y[0] = 2;
    state.player.route_x[1] = 3;
    state.player.route_y[1] = 3;
    state.player.route_len = 2;
    state.player.route_idx = 1;
    fc_click_feedback_accept_move_tick(&feedback, &state);
    assert(!feedback.preview_pending);
    assert(feedback.destination_active);
    assert(fc_click_feedback_route(&feedback, &state, &route_x, &route_y,
                                   &start, &len));
    assert(route_x == state.player.route_x && route_y == state.player.route_y);
    assert(start == 1 && len == 2);

    state.player.route_idx = state.player.route_len;
    fc_click_feedback_sync(&feedback, &state);
    assert(!feedback.destination_active);

    fc_click_feedback_select_move(&feedback, &state, 7, 7, 10.0f, 20.0f);
    fc_click_feedback_select_interaction(&feedback, 30.0f, 40.0f);
    assert(!feedback.destination_active);
    assert(!feedback.preview_pending);
    assert(feedback.preview_route_len == 0);
    assert(feedback.cross_kind == FC_CLICK_CROSS_INTERACTION);
    assert(feedback.cross_screen_x == 30.0f && feedback.cross_screen_y == 40.0f);

    puts("click feedback tests passed");
    return 0;
}
