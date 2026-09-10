# Tiered combat loadouts

Implemented in v38, September 2026. Equipment follows the supplied
`osrs_combat_gear_loadouts_2026.md` primary slot tables, not its optional
boss-specific alternatives. Maxed Ranged deliberately retains the existing
SOTA Masori (f)/Twisted bow preset, including its original levels and bonuses.
The document gives level ranges, so the concrete levels below are the chosen
training defaults. Hitpoints/Prayer here are whole levels, not internal tenths.

| Style/tier | Main equipment | Atk | Str | Def | Rng | Prayer | Magic | HP |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Melee Low | Neitiznot, torso, dragon legs/scimitar/defender | 60 | 60 | 60 | 60 | 43 | 60 | 65 |
| Melee Medium | Blood moon, dual macuahuitl | 70 | 75 | 70 | 70 | 70 | 70 | 80 |
| Melee High | Faceguard, Bandos, Fang, Avernic defender | 82 | 85 | 80 | 80 | 80 | 80 | 90 |
| Melee Maxed | Torva, Scythe, Infernal cape, rancour, Ultor | 99 | 99 | 99 | 99 | 99 | 99 | 99 |
| Ranged Low | Red d'hide, rune crossbow, Odium, diamond (e) | 60 | 60 | 60 | 65 | 43 | 60 | 65 |
| Ranged Medium | Blessed d'hide, Armadyl crossbow, ruby dragon (e) | 70 | 75 | 70 | 75 | 70 | 75 | 80 |
| Ranged High | Crystal, Bowfa (c), Pegasians | 80 | 80 | 80 | 85 | 80 | 80 | 90 |
| Ranged Maxed/default | Existing Masori (f)/Tbow, dragon arrows | 1 | 1 | 99 | 99 | 99 | 1 | 99 |
| Magic Low | Mystic, smoke staff, Mage's book | 60 | 60 | 60 | 60 | 43 | 60 | 65 |
| Magic Medium | Blue moon, Twinflame, Mage's book | 70 | 75 | 70 | 70 | 70 | 75 | 80 |
| Magic High | Virtus, Kodai, Elidinis' ward | 80 | 80 | 80 | 80 | 80 | 94 | 90 |
| Magic Maxed | Ancestral, Harmonised staff, ward (f), Confliction | 99 | 99 | 99 | 99 | 99 | 99 | 99 |

The complete slot lists are authoritative in `fc-core/src/fc_loadouts.c`.
Equipment bonuses/requirements come from `fc_items.c`; the existing equip and
unequip paths recalculate them. Existing Agility-99 run-energy mechanics remain.

## Playable viewer

From `runescape-rl`:

```bash
./build/fc-viewer/fc_viewer
```

Open the bottom **Loadout** tab, choose **Combat type**, then **Equip preset**.
Selecting a tier immediately replaces worn equipment, including weapon, correct
offhand (or none for two-handed weapons), accessories, and ammo. The replaced
debug gear is discarded rather than inserted into inventory. Existing inventory
items are kept; switching does not reset the cave, RNG, cooldown, run energy, or
attacks already in flight. Current HP/Prayer clamp to the new maximum but do
not refill. Ordinary inventory clicks still equip/unequip normally.

Magic presets select Standard autocast: Water Blast for Low, Water Wave for
Medium, and Water Surge for High/Maxed. These match the Fight Caves' water
weakness where present and require no extra viewer setup to attack. Choose a
different book/spell using the existing spellbook and Combat-tab controls.
All four presets use conventional spellcasting weapons, not powered staves.

All presets have unlimited ammo/charges, including default Tbow arrows. Magic
presets also receive 21 normal/combination rune stacks of 10,000 each. Casts
validate those stacks but never decrement them. Removing a required rune blocks
spells unless another stack or the equipped staff supplies the same rune. For
example, remove Blood runes to block Water Wave; merely removing Water runes
does not block it when Twinflame supplies unlimited Water itself.

Magic needs enough inventory room for its rune stacks. A full inventory rejects
the entire switch without changing gear or levels; use **Delete inventory**
if needed. **Generate runes** replenishes stacks without duplicating them.
Delete buttons require confirmation and setup controls are disabled in replay.
The legacy playable reset still grants Magic 99 for manual testing; selecting
any new preset applies its actual listed levels instead.

## Select a training preset

The existing compile-time selector is reused; no new INI setting or policy head
is necessary. From `runescape-rl`, for example:

```bash
FC_ACTIVE_LOADOUT=FC_LOADOUT_MAGIC_MEDIUM ./train.sh train --tag magic_medium
```

`train.sh` detects a selector change, rebuilds, and records the active preset in
the compiled/checkpoint contract. It uses the unchanged main INI and enables
W&B by default. The ordinary attack-target head now uses the selected weapon
and preset autocast; it does not need a viewer grant. This is a new training
task, not a promise that a previously trained ranged policy can use it well.

Names are `FC_LOADOUT_MELEE_{LOW,MEDIUM,HIGH,MAX}`,
`FC_LOADOUT_RANGED_{LOW,MEDIUM,HIGH}`, and
`FC_LOADOUT_MAGIC_{LOW,MEDIUM,HIGH,MAX}`. Ranged Maxed/default is
`FC_LOADOUT_SOTA_TBOW`. The older nine IDs remain stable; eleven new IDs follow
them. Do not pass the braces literally: choose one complete identifier.
Omitting `FC_ACTIVE_LOADOUT` restores the default on the next training launch.

Magic rune stacks survive the adapter's initial-supplies reset. If adding food
or potions later, keep their occupied slots plus the 21 rune slots within 28;
invalid initial inventory configurations fail with a clear error.

Build the viewer with the same selector to replay that preset's checkpoint:

```bash
cmake -S . -B build -DFC_ACTIVE_LOADOUT=FC_LOADOUT_MAGIC_MEDIUM
cmake --build build -j 8
```

Use `-DFC_ACTIVE_LOADOUT=` to restore the normal viewer. Checkpoints from a
different preset are intentionally rejected. The current delivered viewer is
still built with the default SOTA preset.
When launching `eval_viewer.py`, also set `FC_ACTIVE_LOADOUT` to the same
identifier; its compiled-backend preflight checks that selection as well.

## Mechanics, ownership, and compatibility

Core combat includes accurate-stance melee, Fang's outside-ToA double accuracy
roll and restricted damage range, dual-macuahuitl conditional second hits and
Blood moon haste, and Scythe footprint/arc multi-hits. Enchanted bolts use
diamond armour piercing and ruby HP damage/self-cost without diary boosts.
Existing Crystal/Tbow modifiers remain. Magic adds Twinflame adaptation, six-tick
casts and the delayed 40% second hit; Harmonised Standard autocast uses four
ticks; Confliction tracks a miss against the same target/spell before rerolling.
References include the Wiki pages for [Twinflame](https://oldschool.runescape.wiki/w/Twinflame_staff),
[Fang](https://oldschool.runescape.wiki/w/Osmumten%27s_fang),
[Scythe](https://oldschool.runescape.wiki/w/Scythe_of_vitur),
[Dual macuahuitl](https://oldschool.runescape.wiki/w/Dual_macuahuitl), and
[Confliction](https://oldschool.runescape.wiki/w/Confliction_gauntlets).
Other melee stances, special attacks, quest progression, and the document's
optional alternative weapons are not added by these presets.

`fc_apply_loadout` owns atomic gear/level changes; `fc_items_init` owns reset
equipment/resources. The viewer calls these APIs; its old hard-coded gear-grant
lists were removed. Runtime remains C with local exported assets. Asset export
correctly distinguishes the hat slot from head/jaw hide slots, avoiding missing
faces under open hats. No runtime dependencies on reference repositories exist.

Player parts use the textured MDL3 format with local `fc_player.atlas` and
`fc_player.tanim` companions. The existing animated-atlas component scrolls the
cache textures at their recorded direction/speed, and the appearance composer
preserves/rebases texture triangles so UVs follow animated capes. Cape colors
use RuneC's unlit wearable-export convention; other equipment retains its
existing shading. Wearable face priorities remain metadata, not baked geometry
offsets. Rebuild these companions together with
`python3 tools/build_fc_assets.py --magic-only --replace` using the local cache.
Missing player materials fail startup rather than reverting to color-only capes.

INI hyperparameters, rewards, observations (286+34), and policy heads [17,9,8]
are unchanged. State-hash v7 includes resource flags and Confliction state;
old state hashes are not interchangeable. Compatible weight-only v4/v5/v6
checkpoints can replay under current mechanics, not reproduce old state traces.

## Validation

`loadouts.c` covers every new preset's wearable requirements and levels, atomic
failure, resources, Twinflame, melee multi-hits, Fang, enchanted bolts, and
Confliction. Its 20,000-tick default finite/infinite comparison requires identical
state hashes after normalizing only resource flags and remaining ammo.
`loadout_training.c` exercises real adapter resets and attacks with the compiled
preset; it passes separately compiled for all eleven new presets, as well as
the default. Medium Magic was additionally checked with ASan/UBSan.
`test_magic_viewer --graphics` validates all worn items and captures each new
outfit at `/tmp/fc-loadout-kit-9.png` through `-19.png` for visual review.
No new long PPO training run was launched for this implementation.

Final checks: all 172 CTest tests pass; loadout, equipment, Magic, and adapter
tests pass ASan/UBSan; the default CUDA `sm_120` backend rebuild passes. The
hidden-window graphics check passes and all eleven outfit screenshots were
inspected. Interactive combat and equipment switching should still be visually
validated in the playable viewer.

The subsequent cape-material fix also passes all 172 tests and the explicit
`test_equipment_appearance` / `test_magic_viewer --graphics` checks. Build the
former target explicitly and run from `build/`; it writes `equipment-appearance.png`
and `cape-appearance-{0,1}.png` there. The cape sheets show rear idle/walk/attack
poses at two texture times, with assertions for texture-triangle rebasing,
animated UV bounds, cache scroll speeds, and unchanged simulation hashes.
All 169 exported parts retain their base geometry, skin labels, face indices,
and priorities; all 163 non-cape parts retain their vertex colors.
ASan/UBSan graphics validation passes (driver leak detection disabled). Startup
checks with either player material companion missing fail explicitly. The first
graphics invocation from `/tmp` correctly rejected missing arena maps; the
successful checks ran from the documented checkout/build directory instead.

Follow-up correction: those initial graphics tests inherited Raylib's default
backface culling, while the live viewer explicitly disabled it for the player.
This let cape lining/reverse faces draw over the outside, reproducing broken
lava patches on Fire/Infernal and white streaks on the regular Saradomin cape.
The live player draw and both graphics tests now share a one-sided draw function,
matching RuneC's equipped-model pass; NPC/scenery rendering and assets are not
changed by this follow-up. The cape test includes Magic Low and compares the
live draw pixel-for-pixel against an explicit one-sided reference at two texture
times, starting from disabled culling. Its `cape-two-sided-{0,1}.png` sheets
reproduce the old defect; `cape-reference-{0,1}.png` match the corrected sheets.
All 172 CTest tests and both explicit graphics checks pass after this correction.
