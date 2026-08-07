#include "fc_debug.h"
#include "fc_api.h"
#include "fc_contracts.h"
#include <stdlib.h>
#include <string.h>

/* ======================================================================== */
/* Debug log (conditional compilation)                                       */
/* ======================================================================== */

#ifdef FC_DEBUG

void fc_debug_init(FcDebugLog* log) {
    memset(log, 0, sizeof(FcDebugLog));
}

void fc_debug_log_event(FcDebugLog* log, FcDebugEventType type, int tick,
                        int i0, int i1, int i2, int i3) {
    FcDebugEvent* e = &log->events[log->write_idx];
    e->type = type;
    e->tick = tick;
    e->i0 = i0;
    e->i1 = i1;
    e->i2 = i2;
    e->i3 = i3;

    log->write_idx = (log->write_idx + 1) % FC_DEBUG_LOG_SIZE;
    log->count++;
}

#endif /* FC_DEBUG */

/* ======================================================================== */
/* Action trace (always compiled — replay is first-class)                    */
/* ======================================================================== */

void fc_trace_init(FcActionTrace* trace) {
    memset(trace, 0, sizeof(FcActionTrace));
    trace->capacity = FC_TRACE_INITIAL_CAP;
    trace->actions = (int*)malloc(sizeof(int) * trace->capacity * FC_NUM_ACTION_HEADS);
    trace->hashes = (uint32_t*)malloc(sizeof(uint32_t) * trace->capacity);
    trace->num_ticks = 0;
    trace->seed = 0;
    trace->state_hash_version = FC_STATE_HASH_VERSION;
}

void fc_trace_destroy(FcActionTrace* trace) {
    free(trace->actions);
    free(trace->hashes);
    memset(trace, 0, sizeof(FcActionTrace));
}

void fc_trace_reset(FcActionTrace* trace, uint32_t seed) {
    trace->seed = seed;
    trace->state_hash_version = FC_STATE_HASH_VERSION;
    trace->num_ticks = 0;
}

void fc_trace_record(FcActionTrace* trace, const int actions[],
                     int num_heads, uint32_t state_hash) {
    /* Grow if needed */
    if (trace->num_ticks >= trace->capacity) {
        int new_cap = trace->capacity * 2;
        trace->actions = (int*)realloc(trace->actions,
                                       sizeof(int) * new_cap * num_heads);
        trace->hashes = (uint32_t*)realloc(trace->hashes,
                                           sizeof(uint32_t) * new_cap);
        trace->capacity = new_cap;
    }

    int offset = trace->num_ticks * num_heads;
    for (int i = 0; i < num_heads; i++) {
        trace->actions[offset + i] = actions[i];
    }
    trace->hashes[trace->num_ticks] = state_hash;
    trace->num_ticks++;
}
