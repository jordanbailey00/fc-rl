#include "fc_api.h"
#include "fc_items.h"
#include "fc_magic.h"
#include "fc_actor_animation.h"
#include "fc_loadout_debug.h"
#include "fc_magic_visual.h"
#include "fc_player_appearance.h"
#include "fc_model_animation.h"
#include "fc_spotanims.h"
#include "fc_osrs_text.h"
#include "fc_assets.h"
#include "ui.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"magic viewer:%d: %s\n",__LINE__,#x); return 1; } } while(0)

int main(int argc,char **argv) {
    FcState s;
    fc_init(&s); fc_reset(&s,73);
    uint32_t before=fc_state_hash(&s);
    for(int action=0;action<FC_LOADOUT_DEBUG_COUNT;action++) {
        fc_loadout_debug_action(&s,0,action);
        CHECK(fc_state_hash(&s)==before);
    }
    /* Failed magic preset insertion is atomic; worn gear is not lost. */
    before=fc_state_hash(&s);
    fc_loadout_debug_action(&s,1,FC_LOADOUT_DEBUG_LOADOUT_BASE+FC_LOADOUT_MAGIC_LOW);
    CHECK(before==fc_state_hash(&s));
    for(int id=FC_LOADOUT_MELEE_LOW;id<FC_NUM_LOADOUTS;id++) {
        fc_reset(&s,73);
        fc_set_initial_supplies(&s,0,0);
        s.tick=20; s.player.attack_timer=3; s.player.run_energy=1234;
        uint32_t rng=s.rng_state;
        fc_loadout_debug_action(&s,1,FC_LOADOUT_DEBUG_LOADOUT_BASE+id);
        CHECK(s.active_loadout==id && s.player.magic_level==FC_LOADOUTS[id].magic_lvl);
        CHECK(s.player.attack_level==FC_LOADOUTS[id].attack_lvl);
        CHECK(s.tick==20 && s.player.attack_timer==3 && s.player.run_energy==1234 && s.rng_state==rng);
        CHECK(s.player.equipment[FC_EQUIP_SLOT_WEAPON].item_id);
        if(FC_LOADOUTS[id].autocast_spell) {
            CHECK(fc_magic_check(&s,s.player.autocast_spell,1)==FC_MAGIC_OK);
            for(int i=0;i<21;i++) CHECK(s.player.inventory[i].quantity==10000);
        }
        FcItemStack inventory[FC_INVENTORY_SLOTS];
        memcpy(inventory,s.player.inventory,sizeof(inventory));
        fc_loadout_debug_action(&s,1,FC_LOADOUT_DEBUG_CLEAR_EQUIPMENT);
        CHECK(s.player.weapon_kind==FC_WEAPON_UNARMED && !s.player.autocast_spell);
        CHECK(!memcmp(inventory,s.player.inventory,sizeof(inventory)));
        fc_loadout_debug_action(&s,1,FC_LOADOUT_DEBUG_CLEAR_INVENTORY);
        for(int i=0;i<FC_INVENTORY_SLOTS;i++) CHECK(!s.player.inventory[i].item_id);
    }
    s.terminal=1;
    before=fc_state_hash(&s);
    for(int action=0;action<FC_LOADOUT_DEBUG_COUNT;action++) fc_loadout_debug_action(&s,1,action);
    CHECK(before==fc_state_hash(&s));
    fc_reset(&s,73);

    Rectangle body={0,0,506,126};
    FcLoadoutDebugUi debug={0};
    CHECK(fc_loadout_debug_hit(body,(Vector2){185,35},&debug)==-1 && debug.dropdown==2);
    CHECK(fc_loadout_debug_hit(body,(Vector2){185,78},&debug)==FC_LOADOUT_DEBUG_LOADOUT_BASE+FC_LOADOUT_MAGIC_MEDIUM);
    CHECK(fc_loadout_debug_hit(body,(Vector2){350,35},&debug)==-1 && debug.dropdown==3);
    CHECK(fc_loadout_debug_hit(body,(Vector2){350,78},&debug)==FC_BOOK_ANCIENT);
    CHECK(fc_loadout_debug_hit(body,(Vector2){25,35},&debug)==-1 && debug.dropdown==1);
    CHECK(fc_loadout_debug_hit(body,(Vector2){25,78},&debug)==-1 && debug.kit==FC_LOADOUT_KIT_RANGED);
    CHECK(fc_loadout_debug_hit(body,(Vector2){350,35},&debug)==-1 && !debug.dropdown);
    CHECK(fc_loadout_debug_hit(body,(Vector2){185,78},&debug)==-1 && debug.confirm);
    CHECK(fc_loadout_debug_hit(body,(Vector2){185,78},&debug)==FC_LOADOUT_DEBUG_CLEAR_INVENTORY);
    CHECK(fc_loadout_debug_hit(body,(Vector2){25,78},&debug)==FC_LOADOUT_DEBUG_RUNES);

    RuneCUiState selection={0};
    selection.autocast_picker=1;
    const int capabilities[]={0,FC_AUTOCAST_STANDARD|FC_AUTOCAST_ANCIENT,
        FC_AUTOCAST_STANDARD|FC_AUTOCAST_ARCEUUS};
    for(int c=0;c<3;c++) for(int book=0;book<FC_BOOK_COUNT;book++) {
        selection.autocast_capabilities=capabilities[c];
        selection.spellbook=book;
        int slot=0;
        for(int i=0;i<fc_spell_count();i++) {
            const FcSpellDef *spell=fc_spell_at(i);
            if(spell->book==book && (spell->autocast&capabilities[c]))
                CHECK(runec_ui_spell_id(&selection,slot++)==spell->id);
        }
        CHECK(runec_ui_spell_id(&selection,slot)==0);
    }
    selection.autocast_picker=0;
    selection.spellbook=FC_BOOK_STANDARD;
    CHECK(runec_ui_spell_id(&selection,34)!=0 && runec_ui_spell_id(&selection,35)==0);
    for(int i=0;i<fc_item_count();i++) {
        const FcItemDef *item=fc_item_at(i);
        if(item->weapon_kind==FC_WEAPON_POWERED_STAFF && item->powered_divisor && item->id!=22516) {
            if(!fc_magic_visual_profile(0,item->id)) fprintf(stderr,"missing powered profile: %d %s\n",item->id,item->name);
            CHECK(fc_magic_visual_profile(0,item->id));
        }
    }

    AnimCache *cache=anim_cache_load("fc_all.anims");
    CHECK(cache);
    CHECK(anim_get_sequence(cache,fc_player_visual_profile(-3)->attack_anim));
    CHECK(fc_player_visual_profile(-3)->attack_anim==401 && !fc_player_visual_profile(-3)->projectile_travel_spot);
    SpotAnimSet *spots=spotanims_load("fc_spotanims.bin");
    CHECK(spots);
    for(int i=0;i<fc_spell_count();i++) {
        const FcSpellDef *spell=fc_spell_at(i);
        const FcPlayerVisualProfile *visual=fc_magic_visual_profile(spell->id,0);
        CHECK(visual && anim_get_sequence(cache,visual->attack_anim));
        CHECK(!visual->projectile_launch_spot || spotanim_find(spots,visual->projectile_launch_spot));
        CHECK(!visual->projectile_travel_spot || spotanim_find(spots,visual->projectile_travel_spot));
        CHECK(!visual->projectile_impact_spot || spotanim_find(spots,visual->projectile_impact_spot));
        char path[100]; snprintf(path,sizeof(path),"data/sprites/ui/spell_%d.png",spell->id);
        CHECK(fc_asset_exists(path));
    }
    CHECK(spotanim_find(spots,85));
    if(argc>1 && strcmp(argv[1],"--graphics")==0) {
        SetTraceLogLevel(LOG_WARNING);
        SetConfigFlags(FLAG_WINDOW_HIDDEN);
        InitWindow(1200,800,"Magic validation");
        CHECK(IsWindowReady() && fc_osrs_text_init());
        FcPlayerAppearance appearance;
        CHECK(fc_player_appearance_load(&appearance));
        /* Compose every item, including charged/depleted variants. */
        int mystic_hat_found=0;
        for(int i=0;i<appearance.record_count;i++) {
            if(appearance.records[i].item_id!=4089) continue;
            /* A hat must not erase the player's head/jaw identity kits. */
            CHECK((appearance.records[i].hide_mask&3)==0);
            mystic_hat_found=1;
        }
        CHECK(mystic_hat_found);
        for(int i=0;i<fc_item_count();i++) {
            const FcItemDef *item=fc_item_at(i);
            if(item->slot<0) continue;
            memset(s.player.equipment,0,sizeof(s.player.equipment));
            s.player.equipment[item->slot]=(FcItemStack){item->id,1,item->charge_capacity};
            CHECK(fc_player_appearance_sync(&appearance,&s.player,FC_PLAYER_MODEL_BASE)>=0);
        }
        RuneCUiState ui;
        runec_ui_init(&ui);
        ui.active_tab=RUNEC_UI_TAB_SPELLBOOK;
        ui.spellbook=FC_BOOK_ANCIENT;
        s.player.spellbook=FC_BOOK_ANCIENT;
        memset(ui.spell_enabled,1,sizeof(ui.spell_enabled));
        RenderTexture2D target=LoadRenderTexture(1200,800);
        BeginTextureMode(target);
        ClearBackground((Color){45,45,45,255});
        runec_ui_draw(&ui,1200,800);
        debug=(FcLoadoutDebugUi){0};
        fc_loadout_debug_draw(&s,1,&debug,(Rectangle){7,642,506,126});
        EndTextureMode();
        Image image=LoadImageFromTexture(target.texture);
        ImageFlipVertical(&image);
        CHECK(ExportImage(image,"/tmp/fc-magic-ui-validation.png"));
        UnloadImage(image);
        ui.active_tab=RUNEC_UI_TAB_COMBAT;
        ui.magic_weapon=1;
        ui.autocast_capabilities=FC_AUTOCAST_STANDARD|FC_AUTOCAST_ANCIENT;
        ui.autocast_spell=4;
        runec_ui_set_combat_weapon_name(&ui,"Kodai wand");
        runec_ui_set_combat_style_profile(&ui,22);
        for(int picker=0;picker<2;picker++) {
            ui.autocast_picker=picker;
            BeginTextureMode(target);
            ClearBackground((Color){45,45,45,255});
            runec_ui_draw(&ui,1200,800);
            debug.dropdown=1;
            fc_loadout_debug_draw(&s,1,&debug,(Rectangle){7,642,506,126});
            EndTextureMode();
            image=LoadImageFromTexture(target.texture);
            ImageFlipVertical(&image);
            CHECK(ExportImage(image,picker ? "/tmp/fc-autocast-picker.png" : "/tmp/fc-autocast-combat.png"));
            UnloadImage(image);
        }
        /* Full-kit composition and control layout, not just individual pieces. */
        for (int id=FC_LOADOUT_MELEE_LOW;id<FC_NUM_LOADOUTS;id++) {
            fc_reset(&s,73);
            fc_set_initial_supplies(&s,0,0);
            CHECK(fc_apply_loadout(&s,id)==FC_ITEM_OK);
            CHECK(fc_player_appearance_sync(&appearance,&s.player,FC_PLAYER_MODEL_BASE)>=0);
            ModelEntry *entry=appearance.model->entries;
            AnimModelState *pose=NULL;
            uint16_t sequence=0;
            int frame=0;
            float timer=0;
            const FcPlayerVisualProfile *profile=fc_player_visual_profile(fc_player_equipment_visual_profile(&s.player));
            fc_model_animation_update(entry,cache,&pose,&sequence,&frame,&timer,profile->idle_anim,0,0);
            BeginTextureMode(target);
            ClearBackground((Color){45,45,45,255});
            Camera3D camera={.position={0,1.6f,-7},.target={0,1.0f,0},.up={0,1,0},.fovy=3.2f,.projection=CAMERA_ORTHOGRAPHIC};
            BeginMode3D(camera);
            DrawModelEx(entry->model,(Vector3){0,0,0},(Vector3){0,1,0},180,(Vector3){1,1,1},WHITE);
            EndMode3D();
            debug=(FcLoadoutDebugUi){.kit=id>=FC_LOADOUT_MAGIC_LOW ? 0 : id>=FC_LOADOUT_RANGED_LOW ? 1 : 2};
            fc_loadout_debug_draw(&s,1,&debug,(Rectangle){7,642,506,126});
            EndTextureMode();
            image=LoadImageFromTexture(target.texture);
            ImageFlipVertical(&image);
            char path[80]; snprintf(path,sizeof(path),"/tmp/fc-loadout-kit-%d.png",id);
            CHECK(ExportImage(image,path));
            UnloadImage(image);
            anim_model_state_free(pose);
        }
        UnloadRenderTexture(target);
        runec_ui_shutdown(&ui);
        fc_player_appearance_free(&appearance);
        fc_osrs_text_shutdown();
        CloseWindow();
    }
    spotanims_free(spots);
    anim_cache_free(cache);
    puts("magic viewer: debug isolation, inventory atomicity and all spell assets passed");
    return 0;
}
