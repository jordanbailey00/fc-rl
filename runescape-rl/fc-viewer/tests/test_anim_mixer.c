#include "fc_anim_loader.h"

#include <assert.h>
#include <string.h>

static AnimSequence sequence_with_transforms(
    uint16_t id, AnimSequenceFrame* frame, AnimTransform* transforms,
    int transform_count) {
    frame->delay = 1;
    frame->frame.framebase_id = 1;
    frame->frame.transform_count = (uint8_t)transform_count;
    frame->frame.transforms = transforms;
    return (AnimSequence){
        .seq_id = id,
        .frame_count = 1,
        .frames = frame,
    };
}

static void test_pose_action_mixer(void) {
    int16_t base_verts[6] = {0};
    int16_t mixed_verts[6] = {0};
    int group_zero[1] = {0};
    int group_one[1] = {1};
    int* groups[ANIM_MAX_LABELS] = {0};
    int group_counts[ANIM_MAX_LABELS] = {0};
    groups[0] = group_zero;
    groups[1] = group_one;
    group_counts[0] = 1;
    group_counts[1] = 1;
    AnimModelState state = {
        .verts = mixed_verts,
        .vert_count = 2,
        .groups = groups,
        .group_counts = group_counts,
    };

    uint8_t types[2] = {1, 1};
    uint8_t map_lengths[2] = {1, 1};
    uint8_t map_zero[1] = {0};
    uint8_t map_one[1] = {1};
    uint8_t* frame_maps[2] = {map_zero, map_one};
    AnimFrameBase framebase = {
        .base_id = 1,
        .slot_count = 2,
        .types = types,
        .map_lengths = map_lengths,
        .frame_maps = frame_maps,
    };
    AnimCache cache = {
        .bases = &framebase,
        .base_count = 1,
    };

    AnimTransform pose_transforms[2] = {
        {.slot_index = 0, .dx = 10},
        {.slot_index = 1, .dx = 20},
    };
    AnimTransform action_transforms[2] = {
        {.slot_index = 0, .dy = 100},
        {.slot_index = 1, .dy = 200},
    };
    AnimSequenceFrame pose_frame = {0};
    AnimSequenceFrame action_frame = {0};
    AnimSequence pose = sequence_with_transforms(
        1, &pose_frame, pose_transforms, 2);
    AnimSequence action = sequence_with_transforms(
        2, &action_frame, action_transforms, 2);

    assert(anim_mix_pose_action(
        &cache, &state, base_verts, &pose, 0, NULL, 0));
    assert(mixed_verts[0] == 10 && mixed_verts[1] == 0);
    assert(mixed_verts[3] == 20 && mixed_verts[4] == 0);

    assert(anim_mix_pose_action(
        &cache, &state, base_verts, &pose, 0, &action, 0));
    assert(mixed_verts[0] == 0 && mixed_verts[1] == 100);
    assert(mixed_verts[3] == 0 && mixed_verts[4] == 200);

    uint8_t pose_owned_slots[1] = {1};
    action.interleave_count = 1;
    action.interleave_order = pose_owned_slots;
    assert(anim_mix_pose_action(
        &cache, &state, base_verts, &pose, 0, &action, 0));
    assert(mixed_verts[0] == 0 && mixed_verts[1] == 100);
    assert(mixed_verts[3] == 20 && mixed_verts[4] == 0);

    memset(mixed_verts, 0, sizeof(mixed_verts));
    action_frame.frame.framebase_id = 99;
    assert(anim_mix_pose_action(
        &cache, &state, base_verts, &pose, 0, &action, 0));
    assert(mixed_verts[0] == 10 && mixed_verts[1] == 0);
    assert(mixed_verts[3] == 20 && mixed_verts[4] == 0);
}

int main(void) {
    test_pose_action_mixer();
    return 0;
}
