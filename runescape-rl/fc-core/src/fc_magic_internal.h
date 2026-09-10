#ifndef FC_MAGIC_INTERNAL_H
#define FC_MAGIC_INTERNAL_H
#include "fc_magic.h"
FcMagicResult fc_magic_launch(FcState *state, FcNpc *target, int distance);
void fc_magic_resolve(FcState *state, FcNpc *target,
                       const FcPendingHit *hit, int actual_damage);
void fc_magic_tick_npc(FcState *state, FcNpc *npc);
#endif
