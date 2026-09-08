#ifndef FC_PLAYER_APPEARANCE_H
#define FC_PLAYER_APPEARANCE_H
#include "fc_models.h"
#include "fc_types.h"

typedef struct {
    ModelSet *parts;
    ModelSet *model;
    struct { uint32_t item_id, hide_mask; } records[64];
    int record_count;
    int worn_ids[FC_EQUIPMENT_SLOTS];
} FcPlayerAppearance;

int fc_player_appearance_load(FcPlayerAppearance *appearance);
/* 1 = rebuilt, 0 = unchanged, -1 = missing/invalid asset or allocation failure. */
int fc_player_appearance_sync(FcPlayerAppearance *appearance,
                               const FcPlayer *player, uint32_t model_id);
void fc_player_appearance_free(FcPlayerAppearance *appearance);
#endif
