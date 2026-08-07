#ifndef FC_DEBUG_H
#define FC_DEBUG_H

#include "fc_types.h"

/*
 * fc_debug.h — Debug/trace hooks for determinism verification and diagnostics.
 *
 * Compile with -DFC_DEBUG to enable debug logging. When disabled (default),
 * all debug calls compile to no-ops with zero runtime cost.
 *
 * These hooks are sufficient to diagnose simulation drift, incorrect combat
 * resolution, and policy misbehavior without the viewer.
 */

/* ======================================================================== */
/* Debug event types                                                         */
/* ======================================================================== */

typedef enum {
    FC_DBG_TICK_START,      /* tick begins */
    FC_DBG_ACTION,          /* action processed */
    FC_DBG_MOVE,            /* player moved */
    FC_DBG_ATTACK_FIRE,     /* attack initiated (hit queued) */
    FC_DBG_HIT_RESOLVE,     /* pending hit resolves (damage applied) */
    FC_DBG_PRAYER_CHANGE,   /* prayer toggled */
    FC_DBG_CONSUME,         /* food/potion consumed */
    FC_DBG_NPC_SPAWN,       /* NPC spawned */
    FC_DBG_NPC_DEATH,       /* NPC died */
    FC_DBG_WAVE_CLEAR,      /* wave completed */
    FC_DBG_TERMINAL,        /* episode ended */
    FC_DBG_STATE_HASH       /* state hash computed */
} FcDebugEventType;

typedef struct {
    FcDebugEventType type;
    int tick;
    int i0, i1, i2, i3;    /* event-specific integer data */
} FcDebugEvent;

/* ======================================================================== */
/* Debug log (circular buffer)                                               */
/* ======================================================================== */

#define FC_DEBUG_LOG_SIZE 1024

typedef struct {
    FcDebugEvent events[FC_DEBUG_LOG_SIZE];
    int count;      /* total events logged (may exceed LOG_SIZE) */
    int write_idx;  /* next write position (wraps) */
} FcDebugLog;

/* ======================================================================== */
/* Conditional compilation                                                   */
/* ======================================================================== */

#ifdef FC_DEBUG

void fc_debug_init(FcDebugLog* log);
void fc_debug_log_event(FcDebugLog* log, FcDebugEventType type, int tick,
                        int i0, int i1, int i2, int i3);

/* Convenience macros */
#define FC_DBG_LOG(log, type, tick, a, b, c, d) \
    fc_debug_log_event((log), (type), (tick), (a), (b), (c), (d))

#else

#define FC_DBG_LOG(log, type, tick, a, b, c, d) ((void)0)

static inline void fc_debug_init(FcDebugLog* log) { (void)log; }

#endif /* FC_DEBUG */

/* ======================================================================== */
/* Action trace (for seed + action replay)                                   */
/* ======================================================================== */

/*
 * Records per-tick action buffers for replay compatibility.
 * Replay metadata records the seed and state-hash version. The payload stores
 * actions as int32[num_ticks][FC_NUM_ACTION_HEADS] plus per-tick hashes.
 *
 * The trace is a simple growable buffer. Call fc_trace_record after each step.
 * To replay: fc_reset(state, trace.seed), then for each tick:
 *   fc_step(state, trace.actions[tick])
 * and verify fc_state_hash matches the recorded hash.
 */

#define FC_TRACE_INITIAL_CAP 4096

typedef struct {
    uint32_t seed;
    uint32_t state_hash_version;
    int* actions;       /* flat: actions[tick * FC_NUM_ACTION_HEADS + head] */
    uint32_t* hashes;   /* per-tick state hash (optional, for verification) */
    int num_ticks;
    int capacity;       /* in ticks */
} FcActionTrace;

void fc_trace_init(FcActionTrace* trace);
void fc_trace_destroy(FcActionTrace* trace);
void fc_trace_reset(FcActionTrace* trace, uint32_t seed);
void fc_trace_record(FcActionTrace* trace, const int actions[],
                     int num_heads, uint32_t state_hash);

#endif /* FC_DEBUG_H */
