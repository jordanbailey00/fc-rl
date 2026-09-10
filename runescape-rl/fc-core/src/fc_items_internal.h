#ifndef FC_ITEMS_INTERNAL_H
#define FC_ITEMS_INTERNAL_H
#include "fc_items.h"
void fc_items_init(FcPlayer *player, const FcLoadout *loadout);
void fc_items_consume(FcPlayer *player, int potion);
void fc_items_spend_ammo(FcPlayer *player);
void fc_items_recalculate(FcPlayer *player);
#endif
