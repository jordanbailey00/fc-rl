#ifndef FC_ACTION_INTERNAL_H
#define FC_ACTION_INTERNAL_H

#include "fc_types.h"

int fc_eat_action_valid(const FcState* state, int action);
int fc_drink_action_valid(const FcState* state, int action);

#endif
