# Optional combat Magic

Implemented in v38, September 2026. This extends the existing equipment and
combat systems; it does not select a new training task or change the live INI.

## Playable testing

From `runescape-rl` run `./build/fc-viewer/fc_viewer`.

1. Pause with Space while preparing. Open the bottom console's **Loadout** tab.
2. Choose **Magic** in the combat-type dropdown, then Low, Medium, High, or
   Maxed in **Equip preset**. This equips the set, applies its levels, and
   supplies non-consuming rune stacks. See [loadouts](loadouts.md) for gear,
   concrete levels, resource rules, and backend training selection.
3. The preset starts with a suitable Standard Water autocast. Choose another
   spellbook if needed; **Generate runes** replenishes the inventory stacks.
4. Preset selection replaces worn gear; it preserves existing inventory and
   fails atomically if there is not enough room for runes. Ordinary inventory
   and equipment clicks still obey level, capacity, and two-handed rules.
5. For a manual cast, select a spell in the spellbook and then an NPC. For
   autocasting, open the **Combat** tab, click **Autocast**, and select a spell.
   The selector lists only spells compatible with the current book and weapon;
   normal rune/level checks still apply. Powered staves instead show **Built-in**
   and cast when an NPC is attacked; they do not autocast spellbook spells.
6. Resume. Existing god mode and wave/NPC/TPS controls remain available.

For an initial check, choose High Magic and switch Kodai to Ancient Magic to
compare Ice Barrage (freeze/area damage) with Blood Barrage (area damage/heal).
Then test Standard Water spells with Twinflame and the Harmonised staff.
Check animations, missiles/splashes, impacts, healthbars, rune counts,
and gear appearance. **Autocast off** in the combat selector cancels the selected
spell; choosing **Bash** also returns a regular staff to melee. Switching
spellbooks clears spell selection and the combat target. Reset returns to the
compiled loadout. Legacy playable presets retain the Magic-99 testing override;
new tiered presets, training, and replay use their prescribed levels. UI setup
is disabled throughout replay.

Lunar is selectable but has no direct combat spells in this port. Its panel
states that explicitly instead of displaying nonfunctional combat choices.

### Loadout item management

- **Ranged** and **Melee** expose their four tiered presets from `FC_LOADOUTS`.
  Ranged Maxed is the unchanged SOTA Masori/Twisted bow gear. Selection equips
  rather than adding a second outfit to inventory; ammo/charges are unlimited.
- **Delete inventory** deletes inventory items only and clears consumable counts
  and selected slots. **Delete equipped** deletes worn items only, using normal
  unequip transactions to recalculate bonuses and autocast compatibility. It
  works even with a full inventory; inventory items are preserved.
- Both delete buttons require a second click on **Confirm deletion?**. Left-clicking
  elsewhere cancels confirmation. These are debug deletions, not ground-item drops;
  deleted test gear can be generated again. Regular Equipment-tab Remove still
  means unequip into inventory and still requires space.

The viewer-only `fc_loadout_debug.c/.h` dashboard calls the core preset API.
Its earlier hand-written gear-grant lists were superseded by the shared table.
Icons were copied from RuneC's exported item sprites and worn models/animations
exported from v38's local cache. No live INI or policy layout was changed.

## Runtime ownership and supported mechanics

- `fc-core/src/fc_magic.c` owns the 57 combat-spell definitions, requirements,
  rune transactions, accuracy/damage, manual casting, autocasting, and effects.
  There are 35 Standard, 16 Ancient, and six Arceuus definitions. Crumble Undead
  and the three Demonbane spells reject Fight Caves NPCs, which are neither
  undead nor demons; these are not silently converted into generic damage.
- Spells use the existing target approach, collision/LOS, attack cooldown,
  pending-hit queues, seeded RNG, damage/death/reward processing, and render
  events. Manual casts request one attack; autocasts repeat on cooldown.
  Failed validation does not spend runes or charges. Movement does not gain
  a second, viewer-owned casting implementation.
- Normal/combination runes, infinite elemental staff runes, charged tomes,
  weapon charges/depletion, and supported rune-saving passives are included.
  Gear stats are recalculated by the existing equip/unequip transactions.
- Equipment Magic accuracy and damage, elemental tier scaling, Fight Caves
  water weaknesses, chaos gauntlets, smoke staff, Virtus, Kodai, tomes, and
  Shadow's built-in multipliers are represented. `magic_damage_permille` in
  `FcItemDef` uses 30 for +3%, not 30%. Tome damage multiplies the rolled hit;
  water-tome curse accuracy/effectiveness are distinct from water damage.
- Ice/binding immobilization, freeze immunity, blood healing capped to actual
  damage, smoke poison, Shadow/cursing stat reductions and restoration,
  burst/barrage secondary targets, Sang healing, and toxic-trident venom are
  resolved by the core. Launch-dependent bonuses are captured before charges
  expire or equipment changes. Jad converts venom to poison.
- `fc-viewer/src/fc_loadout_debug.c` owns the manual setup buttons and is absent
  from training. The shared core preset/rune APIs are also used by an explicitly
  selected Magic training preset at reset. No per-step refill or viewer callback
  is used. Default ranged reset does not select Magic or generate runes.
- `fc_magic_visual.c` is presentation metadata, not a second spell calculator.
  Actor/projectile rendering consumes authoritative launch events and reuses
  the existing animation mixer and impact/hitsplat/death synchronization.
  The spellbook now reads the core spell catalogue instead of the former
  hard-coded, nonfunctional Standard-spell UI table.

The callable interface is `fc_magic.h`: `fc_set_spellbook`, `fc_set_autocast`,
`fc_cast_spell`, validation and read-only catalogue/calculation functions.
`fc_inventory_add` provides explicit item insertion for setup; casting itself
never generates runes. A Magic training preset initializes autocast so the
existing target head can attack. Adding policy-controlled spellbook/spell
switching still requires an explicit action/observation contract change.

## Scope and reference choices

This is combat Magic for the Fight Caves, not the complete Magic skill. Utility
spells, teleports, Lunar support effects, thralls, Charge/Mark of Darkness,
spell unlock quests, offensive Prayers, Magic potions, special attacks, rune
pouches, XP progression, and normal recharge/banking interfaces are not added.
The preset selector supplies curated full sets from the supported catalogue. Magic uses its default
stance; defensive/long-range stance selection is not implemented. Dawnbringer
cannot fire its Theatre-of-Blood-only attack here. Wilderness-only weapon
multipliers do not apply inside the Fight Caves.

RuneC_v4's `rc-content/combat/magic.c`, combat visuals, and item metadata were
references, not runtime dependencies. Formula/gear cross-checks used the
OSRS Wiki DPS calculator snapshot `d8e13a61` (August 2026) and Wiki mechanics
pages. This includes its July 2026 Sang update (1/5 healing proc and +8 damage
on that proc), rather than older cached 1/6 descriptions. Tome semantics were
cross-checked against the [water tome effects](https://oldschool.runescape.wiki/w/Tome_of_water)
and [Magic damage](https://oldschool.runescape.wiki/w/Magic_damage) references.
The implementation is a pinned snapshot, not a guarantee that future OSRS
balance changes will be picked up automatically.

## Assets and compatibility

All gameplay/UI runtime logic is C and all required exported assets live under
`fc-viewer/assets`. No runtime call accesses RuneC or another reference tree.
Spell icons and additional wear models, spot effects, missiles, and animations
were exported from the existing local cache. Selected missing item-icon PNGs
were copied from RuneC's exported item sprites. No Java/Kotlin runtime or source
was imported. Developer export tooling remains Python, as before.

`tools/build_fc_assets.py --magic-only --out-dir <staging-dir> --replace`
rebuilds these cache-derived assets without re-exporting the arena. Visual C
profiles define required spot/animation IDs; `magic_icons.json` maps stable
spell IDs to cache sprite IDs. New equipment data requires its exported worn
parts and inventory icon as well as an `FcItemDef` row.

The live INI, starting ranged gear/stats, rewards, 320-float policy input, and
three action heads `[17, 9, 8]` are unchanged. Magic is inactive on default
reset. Default ammo/charges are now unlimited, as requested for tiered loadouts.
State-hash version is **7**, including resource flags and Confliction state.
Weight-only checkpoints from v4/v5/v6 remain usable when
every other policy-contract field matches; state/action trace hashes are not
declared interchangeable.

## Validation

`fc-validation/tests/magic.c` covers all spell definitions, valid/invalid
targets, rune/charge/queue transactions, equipment restrictions, elemental and
gear modifiers, launch snapshots, healing/poison/venom/freezing/stat effects,
multi-target costs, live autocast cadence, and reset isolation.
`fc-viewer/tests/test_magic_viewer.c` verifies every debug action is inert in
replay, every tiered set can be equipped, grants are atomic, removal
preserves unrelated items/stats/timers, dropdown and confirmation hit testing
works, autocast filtering follows weapon capabilities, and spell assets exist.
Its optional `--graphics` check opens
a hidden window, composes supported equipment appearances, and renders the
spellbook/dashboard to `/tmp/fc-magic-ui-validation.png`, plus the combat tab
and spell selector to `/tmp/fc-autocast-combat.png` and `/tmp/fc-autocast-picker.png`.
Each new tiered outfit and its dashboard state is also captured at
`/tmp/fc-loadout-kit-9.png` through `-19.png`.

### Historical pre-tiered validation

The following results predate the unlimited-resource/tiered-preset changes.

The final CMake build and all **170 CTest tests** passed. The Magic core test
also passed AddressSanitizer/UndefinedBehaviorSanitizer (leak detection disabled).
The CUDA `sm_120` training extension rebuilt successfully. The hidden-window
equipment/UI test passed; its dashboard/spellbook screenshot was inspected.

The pre/post default ranged seed-73 preservation workload uses 10,000 warmup
and 1,000,000 measured steps. Both checksums are `ff4813ed2ec9d6b2`,
with 3,830 resets (2,789 terminal). This is a deterministic preservation check,
not a new 100M/750M training experiment. Manual visual validation of combat
remains a separate check from asset loading and headless mechanics tests.

The subsequent Loadout-tab extension passed the same 170 tests and the
hidden-window graphics test; all three complete-kit screenshots were inspected.
The seed-73 million-step workload retained checksum `ff4813ed2ec9d6b2` and
the same reset counts. No training job was run for this UI/setup change.
