#include "fc_projectile_visual.h"

#include <assert.h>
#include <math.h>

static int near(float actual, float expected) {
    return fabsf(actual - expected) < 0.0001f;
}

static void test_client_profile_timing(void) {
    FcProjectileTiming timing = {0};
    assert(fc_projectile_timing_from_client_cycles(
        41.0f, 71.0f, 5.0f / 3.0f, &timing));
    assert(near(timing.launch_delay, 0.82f));
    assert(near(timing.flight_duration, 0.62f));
    assert(near(timing.total_duration, 1.44f));
}

static void test_reference_profile_end_cycles(void) {
    const int distance = 4;
    assert(near(fc_projectile_profile_end_cycle(
        41.0f, 5.0f, 5.0f, distance), 66.0f));
    assert(near(fc_projectile_profile_end_cycle(
        32.0f, 0.0f, 5.0f, distance), 52.0f));
    assert(near(fc_projectile_profile_end_cycle(
        28.0f, 8.0f, 8.0f, distance), 68.0f));
    assert(near(fc_projectile_profile_end_cycle(
        41.0f, 0.0f, 5.0f, distance), 61.0f));
}

static void test_profile_timing_scales_with_tps(void) {
    FcProjectileTiming normal = {0};
    FcProjectileTiming fast = {0};
    assert(fc_projectile_timing_from_client_cycles(
        32.0f, 57.0f, 5.0f / 3.0f, &normal));
    assert(fc_projectile_timing_from_client_cycles(
        32.0f, 57.0f, 50.0f / 3.0f, &fast));
    assert(near(fast.launch_delay, normal.launch_delay / 10.0f));
    assert(near(fast.flight_duration, normal.flight_duration / 10.0f));
    assert(near(fast.total_duration, normal.total_duration / 10.0f));
}

static void test_effect_plays_at_most_once(void) {
    float tps = 5.0f / 3.0f;
    assert(near(fc_projectile_effect_duration_seconds(
        60.0f, 90.0f, tps), 1.2f));
    assert(near(fc_projectile_effect_duration_seconds(
        120.0f, 90.0f, tps), 1.8f));
    assert(near(fc_projectile_effect_duration_seconds(
        0.0f, 90.0f, tps), 0.6f));
}

static void test_arrives_at_target(void) {
    FcProjectilePath path = {
        .source_x = 1.0f,
        .source_y = 2.0f,
        .source_z = 3.0f,
        .target_x = 9.0f,
        .target_y = 4.0f,
        .target_z = -5.0f,
        .duration = 2.0f,
        .angle = 15.0f,
        .progress = 11.0f / 128.0f,
    };
    FcProjectileSample sample = {0};
    assert(fc_projectile_path_sample(&path, path.duration, &sample));
    assert(near(sample.x, path.target_x));
    assert(near(sample.y, path.target_y));
    assert(near(sample.z, path.target_z));
}

static void test_tracks_current_target(void) {
    FcProjectilePath path = {
        .source_x = 0.0f,
        .source_y = 1.0f,
        .source_z = 0.0f,
        .target_x = 8.0f,
        .target_y = 1.0f,
        .target_z = 0.0f,
        .duration = 2.0f,
        .angle = 0.0f,
        .progress = 0.0f,
    };
    FcProjectileSample original = {0};
    FcProjectileSample moved = {0};
    assert(fc_projectile_path_sample(&path, 1.0f, &original));
    path.target_x = 12.0f;
    assert(fc_projectile_path_sample(&path, 1.0f, &moved));
    assert(near(original.x, 4.0f));
    assert(near(moved.x, 6.0f));
}

static void test_progress_offsets_launch(void) {
    FcProjectilePath path = {
        .source_x = 2.0f,
        .source_y = 1.0f,
        .source_z = 2.0f,
        .target_x = 6.0f,
        .target_y = 1.0f,
        .target_z = 2.0f,
        .duration = 1.0f,
        .angle = 0.0f,
        .progress = 0.5f,
    };
    FcProjectileSample sample = {0};
    assert(fc_projectile_path_sample(&path, 0.0f, &sample));
    assert(near(sample.x, 2.5f));
    assert(near(sample.y, 1.0f));
    assert(near(sample.z, 2.0f));
}

int main(void) {
    test_client_profile_timing();
    test_reference_profile_end_cycles();
    test_profile_timing_scales_with_tps();
    test_effect_plays_at_most_once();
    test_arrives_at_target();
    test_tracks_current_target();
    test_progress_offsets_launch();
    return 0;
}
