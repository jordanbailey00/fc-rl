#include "fc_api.h"

int main(void) {
    FcState state;
    fc_init(&state);
    fc_reset(&state, 1U);
    fc_destroy(&state);
    return 0;
}
