#include "fc_loadout_debug.h"
#include "fc_magic.h"
#include "fc_items.h"
#include "fc_osrs_text.h"
#include <string.h>

static const int PRESETS[3][4] = {
    {FC_LOADOUT_MAGIC_LOW, FC_LOADOUT_MAGIC_MEDIUM, FC_LOADOUT_MAGIC_HIGH, FC_LOADOUT_MAGIC_MAX},
    {FC_LOADOUT_RANGED_LOW, FC_LOADOUT_RANGED_MEDIUM, FC_LOADOUT_RANGED_HIGH, FC_LOADOUT_SOTA_TBOW},
    {FC_LOADOUT_MELEE_LOW, FC_LOADOUT_MELEE_MEDIUM, FC_LOADOUT_MELEE_HIGH, FC_LOADOUT_MELEE_MAX},
};
static const char *const TIERS[] = {"Low", "Medium", "High", "Maxed"};

const char *fc_loadout_debug_action(FcState *state, int playable, int action) {
    if (!state || !playable) return "Loadout setup is disabled during policy replay.";
    if (action < 0 || action >= FC_LOADOUT_DEBUG_COUNT) return "Invalid setup action.";
    if (state->terminal || state->player.current_hp <= 0) return "Reset before changing your loadout.";
    if (action <= FC_LOADOUT_DEBUG_BOOK_3)
        return fc_magic_result_message(fc_set_spellbook(state, action));
    if (action == FC_LOADOUT_DEBUG_LEVEL) {
        if (state->active_loadout < FC_LOADOUT_MELEE_LOW) state->player.magic_level = 99;
        return "";
    }
    if (action == FC_LOADOUT_DEBUG_CLEAR_INVENTORY) {
        memset(state->player.inventory, 0, sizeof(state->player.inventory));
        state->player.sharks_remaining = state->player.prayer_doses_remaining = 0;
        state->player.selected_food_slot = state->player.selected_potion_slot = -1;
        return "Inventory items deleted. Equipped items were kept.";
    }
    if (action == FC_LOADOUT_DEBUG_CLEAR_EQUIPMENT) {
        /* Delete only the worn items. Use the normal unequip transaction to
         * recalculate stats/autocast, with temporary space even if bags are full. */
        FcState copy = *state;
        memset(copy.player.inventory, 0, sizeof(copy.player.inventory));
        for (int slot = 0; slot < FC_EQUIPMENT_SLOTS; slot++) {
            if (!copy.player.equipment[slot].item_id) continue;
            FcItemResult result = fc_unequip_item(&copy, slot);
            if (result != FC_ITEM_OK) return fc_item_result_message(result);
        }
        memcpy(copy.player.inventory, state->player.inventory, sizeof(copy.player.inventory));
        state->player = copy.player;
        return "Equipped items deleted. Inventory items were kept.";
    }
    if (action == FC_LOADOUT_DEBUG_RUNES) {
        FcItemResult result = fc_inventory_add_runes(state);
        return result == FC_ITEM_OK ? "Rune stacks replenished." : fc_item_result_message(result);
    }
    FcItemResult result = fc_apply_loadout(state, action - FC_LOADOUT_DEBUG_LOADOUT_BASE);
    return result == FC_ITEM_OK ? "Loadout equipped and skill levels applied."
                               : fc_item_result_message(result);
}

static Rectangle control(Rectangle body, int index) {
    float width = (body.width - 56) / 3;
    return (Rectangle){body.x + 16 + (index % 3) * (width + 12),
                       body.y + 24 + (index / 3) * 42, width, 30};
}

static Rectangle choice_row(Rectangle body, int dropdown, int choice) {
    Rectangle r = control(body, dropdown - 1);
    r.y += r.height + choice * 17;
    r.height = 17;
    return r;
}

int fc_loadout_debug_hit(Rectangle body, Vector2 mouse, FcLoadoutDebugUi *ui) {
    if (ui->dropdown) {
        int dropdown = ui->dropdown;
        ui->dropdown = 0;
        for (int i = 0; i < (dropdown == 1 ? 3 : 4); i++) {
            if (!CheckCollisionPointRec(mouse, choice_row(body, dropdown, i))) continue;
            if (dropdown == 3) return i;
            if (dropdown == 2) return FC_LOADOUT_DEBUG_LOADOUT_BASE + PRESETS[ui->kit][i];
            ui->kit = i;
            break;
        }
        return -1;
    }
    int action = -1;
    for (int i = 0; i < 6; i++) {
        if (!CheckCollisionPointRec(mouse, control(body,i))) continue;
        if (i < 2 || (i == 2 && ui->kit == FC_LOADOUT_KIT_MAGIC)) ui->dropdown = i + 1;
        if (i == 3) action = FC_LOADOUT_DEBUG_RUNES;
        if (i >= 4) action = i == 4 ? FC_LOADOUT_DEBUG_CLEAR_INVENTORY : FC_LOADOUT_DEBUG_CLEAR_EQUIPMENT;
    }
    if ((action == FC_LOADOUT_DEBUG_CLEAR_INVENTORY || action == FC_LOADOUT_DEBUG_CLEAR_EQUIPMENT)
        && ui->confirm != action) {
        ui->confirm = action;
        return -1;
    }
    ui->confirm = 0;
    return action;
}

static const char *const KIT_NAMES[] = {"Magic", "Ranged", "Melee"};

void fc_loadout_debug_draw(const FcState *state, int playable,
                          const FcLoadoutDebugUi *ui, Rectangle body) {
    Color text = playable ? (Color){255,225,160,255} : GRAY;
    Vector2 mouse = GetMousePosition();
    fc_osrs_draw_text("Combat type", (int)control(body,0).x, (int)body.y+6,12,text);
    fc_osrs_draw_text("Equip preset", (int)control(body,1).x, (int)body.y+6,12,text);
    fc_osrs_draw_text("Spellbook", (int)control(body,2).x, (int)body.y+6,12,text);
    const char *tier = "Choose tier";
    for (int i = 0; i < 4; i++) if (PRESETS[ui->kit][i] == state->active_loadout) tier = TIERS[i];
    const char *labels[] = {KIT_NAMES[ui->kit], tier,
        ui->kit == FC_LOADOUT_KIT_MAGIC ? fc_spellbook_name(state->player.spellbook) : "Not applicable",
        "Generate runes",
        ui->confirm == FC_LOADOUT_DEBUG_CLEAR_INVENTORY ? "Confirm deletion?" : "Delete inventory",
        ui->confirm == FC_LOADOUT_DEBUG_CLEAR_EQUIPMENT ? "Confirm deletion?" : "Delete equipped"};
    for (int i = 0; i < 6; i++) {
        Rectangle r = control(body,i);
        int enabled = playable && !(i == 2 && ui->kit != FC_LOADOUT_KIT_MAGIC);
        int hovered = enabled && !ui->dropdown && CheckCollisionPointRec(mouse,r);
        DrawRectangleRec(r, hovered ? (Color){73,62,47,255} : (Color){49,42,33,255});
        DrawRectangleLinesEx(r,1,(Color){115,99,73,255});
        DrawLine((int)r.x+1,(int)r.y+1,(int)(r.x+r.width)-2,(int)r.y+1,(Color){135,116,84,255});
        fc_osrs_draw_text(labels[i], (int)(r.x+(r.width-fc_osrs_measure_text(labels[i],12))/2),
                         (int)r.y+9,12,enabled ? text : GRAY);
        if (i < 2 || (i == 2 && ui->kit == FC_LOADOUT_KIT_MAGIC)) {
            float x=r.x+r.width-10, y=r.y+r.height/2;
            DrawTriangle((Vector2){x-4,y-2},(Vector2){x,y+3},(Vector2){x+4,y-2},text);
        }
    }
    if (ui->dropdown) {
        for (int i = 0; i < (ui->dropdown == 1 ? 3 : 4); i++) {
            Rectangle r = choice_row(body,ui->dropdown,i);
            DrawRectangleRec(r,CheckCollisionPointRec(mouse,r) ? (Color){80,67,48,255} : (Color){35,30,24,255});
            DrawRectangleLinesEx(r,1,(Color){115,99,73,255});
            int selected = ui->dropdown == 1 ? i == ui->kit : ui->dropdown == 2 ?
                PRESETS[ui->kit][i] == state->active_loadout : i == state->player.spellbook;
            fc_osrs_draw_text(ui->dropdown == 1 ? KIT_NAMES[i] :
                             ui->dropdown == 2 ? TIERS[i] : fc_spellbook_name(i),
                             (int)r.x+9,(int)r.y+3,12,selected ? YELLOW : text);
        }
    }
}
