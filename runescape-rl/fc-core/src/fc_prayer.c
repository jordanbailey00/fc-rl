#include "fc_api.h"
#include "fc_prayer.h"
#include <stddef.h>

/*
 * fc_prayer.c — Prayer activation, drain, and potion restore.
 *
 * OSRS prayer drain (from PrayerDrain.kt):
 *   Each tick: prayerDrainCounter += totalDrainEffect (sum of active prayer drains)
 *   prayerDrainResistance = 60 + (prayerBonus * 2)
 *   While counter > resistance: drain 1 prayer point, counter -= resistance
 *
 *   Protection prayers all cost drain=12 per tick (from prayers.toml).
 *   Prayer points are in tenths (430 = 43 prayer points).
 *   Each "drain 1 point" = subtract 10 tenths.
 *
 * Prayer potion restore:
 *   floor(prayer_level * 0.25) + 7 points per dose.
 *   For level 43: floor(10.75) + 7 = 17 points → 170 in tenths.
 */

#define PRAYER_OVERHEAD_DRAIN_RATE 12

int fc_prayer_drain_tick(FcPlayer* p, int prayer_at_tick_start,
                         const FcPrayerTransition* transition) {
    int prayer_before = p->current_prayer;
    (void)transition;

    /* Perfect 1-tick flicks should be free: only accrue drain when some prayer
     * was active both before and after this tick's action processing. */
    if (prayer_at_tick_start == PRAYER_NONE || p->prayer == PRAYER_NONE) return 0;
    if (p->current_prayer <= 0) {
        /* Auto-deactivate if prayer points depleted */
        p->prayer = PRAYER_NONE;
        p->prayer_drain_counter = 0;
        return 0;
    }

    /* Counter-based drain matching OSRS PrayerDrain.kt exactly:
     * Accumulate drain rate each tick, drain 1 point when counter exceeds resistance. */
    int drain_rate = PRAYER_OVERHEAD_DRAIN_RATE;  /* all 3 protect prayers = 12 */
    int resistance = 60 + 2 * p->prayer_bonus;

    p->prayer_drain_counter += drain_rate;

    while (p->prayer_drain_counter > resistance) {
        p->current_prayer -= 10;  /* drain 1 prayer point (10 tenths) */
        p->prayer_drain_counter -= resistance;

        if (p->current_prayer <= 0) {
            p->current_prayer = 0;
            p->prayer = PRAYER_NONE;
            p->prayer_drain_counter = 0;
            return prayer_before;
        }
    }

    return prayer_before - p->current_prayer;
}

FcPrayerTransition fc_prayer_apply_action(FcPlayer* p, int prayer_action) {
    FcPrayerTransition result = {0};
    result.prior_prayer = p->prayer;
    result.requested_final_prayer = p->prayer;

    switch (prayer_action) {
        case FC_PRAYER_NO_CHANGE:
            break;
        case FC_PRAYER_OFF:
            result.requested_final_prayer = PRAYER_NONE;
            result.off_requested = (p->prayer != PRAYER_NONE);
            p->prayer = PRAYER_NONE;
            break;
        case FC_PRAYER_MAGIC:
            result.requested_final_prayer = PRAYER_PROTECT_MAGIC;
            result.on_requested = (p->prayer != PRAYER_PROTECT_MAGIC);
            p->prayer = (p->current_prayer > 0) ? PRAYER_PROTECT_MAGIC : PRAYER_NONE;
            break;
        case FC_PRAYER_RANGE:
            result.requested_final_prayer = PRAYER_PROTECT_RANGE;
            result.on_requested = (p->prayer != PRAYER_PROTECT_RANGE);
            p->prayer = (p->current_prayer > 0) ? PRAYER_PROTECT_RANGE : PRAYER_NONE;
            break;
        case FC_PRAYER_MELEE:
            result.requested_final_prayer = PRAYER_PROTECT_MELEE;
            result.on_requested = (p->prayer != PRAYER_PROTECT_MELEE);
            p->prayer = (p->current_prayer > 0) ? PRAYER_PROTECT_MELEE : PRAYER_NONE;
            break;
        default:
            break;
    }

    result.actual_final_prayer = p->prayer;
    result.off_performed = result.off_requested;
    result.on_succeeded = result.on_requested &&
        result.actual_final_prayer == result.requested_final_prayer;
    result.final_state_changed =
        result.actual_final_prayer != result.prior_prayer;
    return result;
}

int fc_prayer_apply_loss_tenths(FcPlayer* p, int requested_loss_tenths) {
    if (p == NULL || requested_loss_tenths <= 0) return 0;
    int prayer_before = p->current_prayer;
    int loss = requested_loss_tenths;
    if (loss > prayer_before) loss = prayer_before;
    p->current_prayer -= loss;
    return prayer_before - p->current_prayer;
}

int fc_prayer_potion_restore(int prayer_level) {
    /* floor(level * 0.25) + 7 points → in tenths */
    return (prayer_level / 4 + 7) * 10;
}
