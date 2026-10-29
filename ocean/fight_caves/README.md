# Fight Caves

A native C Fight Caves environment for PufferLib 5.0: all 63 waves, a playable
Raylib viewer, and checkpoint replay on GPU or CPU. Training, replay and manual
play share one simulation.

## Build

Fight Caves needs only PufferLib 5.0's normal native build dependencies, which
[PufferTank 5.0](https://github.com/PufferAI/PufferTank/tree/5.0) provides; there
is no Fight Caves-specific Python package, trainer or Docker image.

```bash
./build.sh fight_caves         # native trainer with Fight Caves compiled in: ./puffer
./build.sh fight_caves --cpu   # manual play, CPU replay and checkpoint checks: ./fight_caves
```

Both builds first install the pinned [assets](#assets); add `--debug` to the CPU
build for sanitizers. Run all commands from the repository root, where the assets
and `config/fight_caves.ini` are found. If the viewer reports a `llvmpipe`
(software) renderer at startup, run `unset __GLX_VENDOR_LIBRARY_NAME` first;
PufferTank's image sets that GLX override.

## Train

```bash
./puffer train
```

Settings come from `config/fight_caves.ini` on top of `config/default.ini`, and
any of them can be overridden with `--section.key=value`. Training is headless.
The `[env]` section holds three kinds of settings:

- **Capabilities** are what the agent can do beyond moving, attacking and the
  protection prayers. Each one adds action heads, observations and mask bits,
  so the build compiles them in from the INI and training stops with a message
  if the INI and the build disagree; rebuild after changing one.
  `consumables = 1` adds eat and drink heads and observes their cooldowns.
  `prayers = all` (default `protection`) adds a `prayer_book` head for the
  rest of the prayer book that RuneC v4, the OSRS engine these rules are
  ported from, implements: the skin, strength, reflex, eye and mystic tiers,
  Rapid Restore, Rapid Heal, Preserve, Chivalry, Deadeye, Mystic Vigour,
  Piety, Rigour and Augury. Each value toggles one prayer, as RuneC does: base
  Prayer and Defence requirements, Deadeye and Mystic Vigour replacing Eagle
  Eye and Mystic Might once available, conflicting prayers switched off,
  their drain added to the protection prayer's, every prayer off at zero
  points, their accuracy, damage and Defence percentages on the player's
  attacks and defence, and Rapid Heal's faster Hitpoints regeneration. The
  mask allows only toggles that take effect, and the observation adds which
  prayers are on and what the active ones drain. Every prayer counts as
  unlocked, as in a members world; RuneC's unlock varbits are not modelled.
  In the cave Rapid Restore and Preserve only drain, since nothing lowers or
  boosts the stats they act on. The prayers RuneC does not implement are in
  [Not yet in RuneC](#not-yet-in-runec).

  `gear_sets = 2` to `4` (default `1`) adds a `gear_set` head that switches
  between whole gear presets, `gear_set_1` to `gear_set_N`. Set 1 is worn at
  the start, and the items of the other sets that it does not wear start in
  the inventory ahead of the supplies, each once and ammunition as one stack.
  Choosing set k equips, in that tick before the attack, every item of set k
  that is not worn, the way clicking each item in the inventory equips it:
  the two-handed and shield rules apply, the attack timer and attacks in
  flight are untouched, and the attack target is dropped as after any equip,
  so the attack head picks it again (in the same tick if it is chosen with
  the switch). The switch is all or nothing and masked when set k is worn or
  one of its items cannot be equipped. The observation adds which set is
  worn and, for each set, its weapon's attack style, range and attack speed.
  A switch to a magic set also sets its autocast spell, when that can be set.

  `spellbook = standard` or `ancient` (default `none`) adds an `autocast`
  head that sets the autocast spell: 0 keeps it, k autocasts the book's spell
  k in spellbook order, the 20 Standard Strike to Surge spells (21 values) or
  the 16 Ancient Rush to Barrage spells (17 values). These are the combat
  spells RuneC lets a staff of the presets autocast; RuneC's manual one-off
  casts (the curses, Bind, Snare and Entangle) and the spells that need a
  staff no preset has (Iban Blast, Magic Dart, the god spells, Crumble Undead)
  are not here, nor the Arceuus and Lunar books. The value is the player's
  spellbook; without the capability it is the Standard book, RuneC's at the
  start. A spell is masked unless RuneC would accept it as the autocast spell
  now: the player alive, the Magic level, a staff that autocasts the book (the
  Kodai wand either book, the other staves Standard spells only) and the
  runes, the staff supplying its elements. The head applies after a gear
  switch and before the attack, keeping the target. The observation adds the
  autocast spell (one-hot) and, with the Ancient book, each visible NPC's
  remaining freeze, freeze immunity, poison damage and Attack drain.

  `special_attacks = 1` (default `0`) adds special attack energy and a
  `special_attack` head that arms the worn weapon's special for its next
  attack: 0 leaves it as it is, 1 toggles it, as RuneC's special attack
  button does. The energy follows RuneC: full at the start, then 10% back
  every 50 ticks while it is below full, counted after the hits on the
  player land. Arming is masked unless the worn weapon has a special RuneC
  implements and the energy covers its cost; disarming never is. The next
  attack that launches fires it: the attack rolls as usual, the special
  scales its damage, rounding down, and the cost is paid and the special
  disarms as it launches. If the weapon worn by then has no special or the
  energy is short, that attack does not launch, the special disarms and the
  attack target is dropped, as RuneC rejects the launch. Two preset weapons
  have one in RuneC, and both cost 50% and change only the damage: the Toxic
  blowpipe's hits deal 150% (`blowpipe_pure`; RuneC itself cannot fire its
  blowpipe yet) and the Armadyl crossbow's 100% (`acb_armadyl`), so its
  special spends energy for an ordinary shot. The head applies after a gear
  switch and the autocast spell, before the attack, as RuneC takes the switch
  and the toggle as input before its attack phase, which fires the special
  of the weapon worn then: switching from one weapon with a special to
  another and arming in one tick arms the new one's, and arming in the tick
  that switches to a weapon without one does nothing. The mask, made before
  the switch, offers arming only when the weapon worn then has a special, so
  arming for a weapon a switch wields takes the next tick. The observation
  adds the energy, whether the special is armed and what the worn weapon's
  special costs (0 without one).
- **Scenario** settings are what an episode starts with and need no rebuild:
  `gear_set_1` names a gear preset, the `*_level` keys set skill levels, and
  `initial_sharks` and `initial_prayer_doses` fill the inventory. With
  `gear_sets = N`, `gear_set_2` to `gear_set_N` name the other gear sets:
  each must be set and no higher one may be, and since `--env` overrides only
  replace keys the INI has, add their lines to it. Every gear set must be
  wearable at the configured levels, and the other sets' items and the
  supplies must fit in the 28 inventory slots. Every preset is wearable at the
  levels it was made for:

  | `gear_set_1` | Main gear | Attack | Strength | Defence | Ranged | Magic | Prayer | Hitpoints |
  | --- | --- | --- | --- | --- | --- | --- | --- | --- |
  | `tbow_masori` (default) | Twisted bow, dragon arrows, Masori (f), Venator ring | 1 | 1 | 99 | 99 | 1 | 99 | 99 |
  | `rcb_black_dhide` | Rune crossbow, adamant bolts, black d'hide, coif | 1 | 1 | 70 | 70 | 1 | 43 | 70 |
  | `rcb_pure` | Rune crossbow, adamant bolts, Robin hood hat, Book of law | 1 | 1 | 1 | 61 | 1 | 43 | 55 |
  | `msb_pure` | Magic shortbow (i), rune arrows, Robin hood hat | 1 | 1 | 1 | 70 | 1 | 43 | 60 |
  | `blowpipe_pure` | Toxic blowpipe with adamant darts, Robin hood hat, Necklace of anguish | 1 | 1 | 1 | 75 | 1 | 43 | 75 |
  | `acb_armadyl` | Armadyl crossbow, diamond dragon bolts (e), Armadyl armour | 1 | 1 | 75 | 80 | 1 | 70 | 80 |
  | `bowfa_crystal` | Bow of faerdhinen (c), crystal armour | 1 | 1 | 75 | 85 | 1 | 70 | 85 |
  | `tbow_masori_no_ring` | `tbow_masori` without the ring | 1 | 1 | 80 | 99 | 1 | 77 | 99 |
  | `smoke_staff_mystic` | Mystic smoke staff casting Water Blast, Mystic robes, Mage's book | 60 | 60 | 60 | 60 | 60 | 43 | 65 |
  | `kodai_virtus` | Kodai wand casting Water Surge, Virtus robes, Elidinis' ward | 80 | 80 | 80 | 80 | 94 | 80 | 90 |
  | `harmonised_ancestral` | Harmonised nightmare staff casting Water Surge, Ancestral robes, Elidinis' ward (f) | 99 | 99 | 99 | 99 | 99 | 99 | 99 |
  | `dragon_scimitar_torso` | Dragon scimitar on Chop (accurate slash), Fighter torso, Dragon defender, Helm of neitiznot | 60 | 60 | 60 | 60 | 60 | 43 | 65 |
  | `macuahuitl_blood_moon` | Dual macuahuitl on Pound (accurate crush), Blood moon armour | 70 | 75 | 70 | 70 | 70 | 70 | 80 |
  | `fang_bandos` | Osmumten's fang on Stab (accurate stab), Bandos armour, Avernic defender | 82 | 85 | 80 | 80 | 80 | 80 | 90 |
  | `scythe_torva` | Scythe of vitur on Reap (accurate slash), Torva armour, Infernal cape | 99 | 99 | 99 | 99 | 99 | 99 | 99 |

  `smoke_staff_mystic`, `kodai_virtus` and `harmonised_ancestral` are the
  low, high and maxed magic presets of the fc-rl 4.0 loadouts, with RuneC v4's
  item definitions. Worn at the start or
  switched to, a magic preset sets its spell as the autocast spell when RuneC
  would accept it (with `spellbook = ancient` its Standard spell cannot be;
  the autocast head sets an Ancient one). Each carries 50,000 of every rune
  the Standard and Ancient spells use, air, water, earth, fire, mind, chaos,
  death, blood, soul and wrath runes, ten inventory slots after any switch
  items and before the supplies. A cast spends its runes from those stacks as
  RuneC does, the staff supplying its elements (the Mystic smoke staff air and
  fire, the Kodai wand water) and combination runes, which no preset carries,
  paying two elements at once; out of runes the autocast spell clears and the
  staff bashes, like a ranged weapon out of ammunition. The medium Magic
  preset, built around the Twinflame staff, waits for RuneC's data for it,
  and `harmonised_ancestral` wears the Confliction gauntlets and Avernic
  treads (max) without bonuses (see [Not yet in RuneC](#not-yet-in-runec)).

  Magic follows RuneC: a staff with an autocast spell of the player's book
  casts at its target from up to 10 tiles in line of sight, every 5 ticks (4
  for the Harmonised nightmare staff's Standard spells), the hit landing
  2 + (d + 1) / 3 ticks later for a distance d to the target's centre. A
  Standard spell hits as hard as the strongest spell of its tier the Magic
  level reaches. Accuracy is (Magic, raised by the magic prayers' percentage,
  + 9) × (Magic attack bonus + 64) against (the NPC's Magic level + 9) × 64:
  the NPCs here have no magic defence bonus, which RuneC also rolls as 0 for
  every ordinary NPC. The max hit adds the Magic damage bonus and the
  prayers' percentage. A hit that lands also freezes (Ice), poisons (Smoke),
  lowers Attack (Shadow) or heals a quarter of its damage (Blood); the cave's
  NPCs do not regenerate, so a drain lasts, as RuneC treats an NPC without a
  regeneration interval. Without an autocast spell a staff bashes in melee.
  RuneC's manual casts, defensive autocast (which
  changes only experience), rune pouches, tomes and powered or charged staves
  are not ported, as no preset uses them.

  The last four are the low, medium, high and maxed melee presets of the
  fc-rl 4.0 loadouts, with RuneC v4's item definitions and requirements,
  where RuneC has them (see [Not yet in RuneC](#not-yet-in-runec)).

  Melee follows RuneC: a melee weapon attacks from open cardinal contact
  (reach 1), every 4 ticks (Dragon scimitar, Dual macuahuitl) or 5
  (Osmumten's fang, Scythe of vitur), the hit landing the tick after the
  attack. The weapon's type gives its attack styles and the preset names the
  one it attacks with: each attacks on its first, accurate style, as the
  fc-rl 4.0 presets did, with the attack type RuneC gives that style.
  Accuracy is (Attack, raised by the attack prayers' percentage, + the
  stance's invisible levels + 8) × (the attack type's bonus + 64) against (the
  NPC's Defence + 9) × 64: the NPCs here have no stab, slash or crush defence
  bonus, which RuneC also rolls as 0 for every ordinary NPC. The max hit is
  ((Strength, raised by the strength prayers' percentage, + the stance's
  invisible levels + 8) × (Strength bonus + 64) + 320) / 640. Accurate adds 3
  Attack levels, Aggressive 3 Strength, Defensive 3 Defence (to the player's
  defence rolls) and Controlled 1 of each. The unarmed punch and a staff's
  bash roll the same way on accurate crush and still land in the attack's
  tick. RuneC's 2-tile polearm reach is not ported, as no preset wields a
  polearm.

- **Rewards** and their shaping terms.

Training and replay print the capabilities, policy heads and scenario at
startup. `./fight_caves describe` prints them for the current config (it takes
the same `--env.key=value` overrides) or says what is wrong with it.

The environment reports episode averages for `zero_progress_ticks`,
`wave_reached`, `wrong_prayer_hits`, `reached_wave_63`, `jad_kill_rate`,
`prayer_uptime_range`, `prayer_uptime_melee`, `prayer_uptime_magic`,
`npc_healing_total`, `jad_healing_total` and `episode_length`. Puffer's `perf`
and `score` are both the Jad kill rate. Prayer uptimes are fractions of episode
ticks; healing totals count HP restored.

### Not yet in RuneC

The rules here are RuneC v4's. What RuneC does not implement yet is left out,
except three ranged rules the ranged presets had before this port followed
RuneC, which stay so that their training does not change. Revisit each when
RuneC implements it:

- **Prayers**
  - Protect Item, Retribution and Redemption: RuneC rejects them, pending
    player-death item retention and a verified trigger order.
  - Smite: RuneC applies it only through boss encounter hooks; the cave's
    NPCs have no Prayer to drain.
  - The Ruinous Powers book: RuneC leaves it out.
- **Magic**
  - Burst and Barrage area damage: RuneC's spells are single-target.
  - Poison and freeze immunities and elemental weaknesses: RuneC has no rules
    for them yet.
  - The Twinflame staff, Confliction gauntlets and Avernic treads (max):
    RuneC's item data has no bonuses or weapon data for them, so the medium
    magic preset waits, the Twinflame staff fights like an empty hand,
    `harmonised_ancestral` wears the gauntlets and treads without bonuses and
    `scythe_torva` the treads.
  - The Twinflame staff's, Confliction gauntlets' and Virtus robes' effects:
    no RuneC rules.
- **Melee**
  - The Scythe of vitur's extra hits by target size, Osmumten's fang's
    accuracy and damage rules and the Dual macuahuitl's second hit: RuneC has
    no weapon passives.
  - The Blood moon set's faster attacks and any Bandos, Torva, Ultor ring or
    Amulet of rancour effect: RuneC has no set or item effects.
  - Requirements for Osmumten's fang, the Dual macuahuitl, Blood moon and
    Torva armour, the Amulet of rancour and the Ultor ring: RuneC's item data
    has none, so any level wears them.
- **Ranged items**
  - The Twisted bow's scaling by the target's Magic level: RuneC waits for
    verified NPC bonus inputs. The bow keeps the scaling it had here.
  - Crystal armour's accuracy and damage bonus to the Bow of faerdhinen:
    not in RuneC. Kept here as before.
  - The Toxic blowpipe's dart container: RuneC rejects blowpipe attacks until
    it has one. Here the blowpipe fires its loaded adamant darts as before.
  - Enchanted bolt effects, such as the diamond dragon bolts (e)'s: RuneC
    checks only that the ammunition fits the weapon.
  - Ava's assembler and accumulator saving ammunition: not in RuneC.
- **Special attacks**
  - The Magic shortbow (i), Dragon scimitar, Osmumten's fang and Dual
    macuahuitl specials: RuneC has only their catalog text
    (`content/specials`), no rules, so arming is masked with them. The other
    preset weapons have no special in RuneC.
  - The Toxic blowpipe special's higher accuracy and healing and the Armadyl
    crossbow special's higher accuracy and enchanted bolt chance, which that
    catalog text describes: RuneC's specials apply only their cost and damage
    percentage.
- **Item effects**
  - No worn-item passive or set effect of a preset's items is in RuneC. The
    item rules RuneC does apply to them are here: the Harmonised nightmare
    staff's faster Standard spells, the staves' runes and autocast books, and
    the Bow of faerdhinen (c) needing no ammunition. The other rows of RuneC's
    item effect table for the presets' items and supplies are the shark's and
    prayer potion's stat changes, for the consumables capability.

## Play

```bash
./fight_caves                  # add --start-wave 63 to begin at a later wave
```

Manual play needs no CUDA or checkpoint. Space starts and pauses, Right Arrow
steps one tick, R resets and Q quits. Left-click walks or attacks, right-click
opens menus, right-drag orbits the camera and the wheel zooms; 1/2/3 toggle the
protection prayers, and with `gear_sets` above 1, F1 to F4 switch to gear sets 1
to 4. The side panel has the combat, inventory, equipment and
prayer tabs, and with `spellbook` the spellbook tab, which lists the book's
spells and autocasts the one clicked; clicking the minimap walks there and its
orb toggles running. The combat tab shows the weapon's attack styles with the
one the attacks use; a staff's also shows the autocast spell, and clicking it
lists the spells to autocast. The pinned assets predate the magic and melee
presets: their items (but the Barrows gloves) and the runes show their names
instead of icons and are left off the player model, the spells show their
names, and the casts, spell graphics, melee stances and melee attacks the
bundle lacks are skipped, the human stance standing in.
With `prayers = all` the prayer tab holds every prayer the policy can use,
bright when a click would toggle it; Preserve, Chivalry, Deadeye, Mystic
Vigour, Piety, Rigour and Augury show their names, as the pinned assets have
no icons for them. With `special_attacks = 1` the special attack orb under the
run orb and the bar in the combat tab show the energy, green while the special
is armed, and clicking either arms or disarms it. The pinned assets lack the
Armadyl crossbow's special animation and both specials' graphics (RuneC's
sequence 427 and spot animations 301 and 1043), so a special looks like the
weapon's normal attack, which for the blowpipe is also RuneC's special one.
The console in the chat box lists targets, jumps to a wave,
sets the tick rate, toggles god mode and debug overlays, and shows player
state, observations, the action mask, rewards and the event log. The full key
list is at the top of `viewer.c`.

## Replay and check checkpoints

```bash
./puffer eval latest                  # or a checkpoint path; needs CUDA
./fight_caves eval CHECKPOINT.bin     # CPU replay in the same viewer
./fight_caves eval CHECKPOINT.bin --headless --base.eval_episodes=2
./fight_caves check CHECKPOINT.bin    # exact size and finite weights only
```

Replay disables the gameplay controls but keeps pause, step, camera and debug;
1/2/4/0 select 1x/2x/4x/10x and Up/Down change the speed. CPU replay needs an
explicit path, uses the native action masks and resets recurrent state between
episodes; its float32 inference can sample different trajectories than CUDA/BF16.
Both replay paths need the policy shape used in training: pass
`--policy.hidden_size=N --policy.num_layers=N` when it differs from the INI.

`check` confirms that a file's size matches the configured policy's float32
weights and that every weight is finite. The CUDA loader only rejects missing or
short files, so check unfamiliar files first. Neither proves where a checkpoint
came from or that it matches this game version; keep the source revision, config
and run log with your checkpoints.

## Assets

The first build downloads the pinned
[Fight Caves v3 bundles](https://github.com/jordanbailey00/fc-rl/releases/tag/fight-caves-assets-v3)
(about 6 MB) listed in `resources/fight_caves/asset_manifest.json`:

- `resources/fight_caves/runtime/`: collision, movement and line-of-sight maps.
- `resources/fight_caves/viewer/`: models, equipment parts, animations, terrain,
  textures, UI sprites and fonts, and the minimap.

Each archive's SHA-256 is checked before extraction and recorded in the installed
directory's `.archive-sha256` stamp. Later builds reuse a matching install offline
without re-reading its files, so reinstall after editing or deleting an installed
file. A failed download or checksum stops the build and keeps the existing
install. Setup needs only Python's standard library with tarfile extraction
filters (Python 3.12+, or 3.8.17/3.9.17/3.10.12/3.11.4 and later).

```bash
python3 ocean/fight_caves/tools.py setup                # what the build runs
python3 ocean/fight_caves/tools.py setup --verify-only  # check the stamps
python3 ocean/fight_caves/tools.py setup --force        # reinstall both bundles
```

Missing data stops with an error rather than falling back to an open map or
reduced graphics. `FC_COLLISION_PATH`, `FC_MOVEMENT_PATH`, `FC_LOS_PATH` and
`FC_ASSET_ROOT` (the viewer directory) override the installed locations. The
assets are exported from Old School RuneScape and are not covered by PufferLib's
software license; see `resources/fight_caves/ASSET_NOTICE.md`.

## Tests

```bash
python3 -m pytest -q tests/test_fight_caves_integration.py
FC_TEST_RENDER=1 python3 -m pytest -q tests/test_fight_caves_integration.py  # adds graphical checks
```

The suite builds a CPU executable with `./build.sh`, then checks the asset
installer, checkpoint handling and CPU replay, and runs the C simulation and
C/C++ linkage suites. Missing prerequisites fail the run; only the graphical
checks, which need `DISPLAY` or `xvfb-run`, are skipped without `FC_TEST_RENDER=1`.
`CC`/`CXX` select the compilers (default clang/clang++). No GPU training runs
in pytest.

### Clean-checkout acceptance

On Linux with CUDA and a display:

```bash
python3 -S tests/fight_caves_e2e.py clean-clone                 # committed HEAD
python3 -S tests/fight_caves_e2e.py clean-clone --working-tree  # plus uncommitted changes
```

This standard-library runner clones into a retained temporary directory, builds
both executables (downloading the assets and Raylib), runs the C suites including
NVCC linkage, opens the viewer, trains four batches and replays the checkpoints
on CPU and GPU. It also checks the errors for missing assets, a missing compiler
or display and bad checkpoints, and fails if any source file changes. Nothing is
silently skipped. Logs, screenshots, checkpoints and
`build/fight-caves-e2e/report.json` stay in the clone.

A dirty source is rejected unless `--working-tree` is given, which applies the
tracked diff and copies untracked, non-ignored files. `--source PATH_OR_URL --ref
REF` tests another commit. `--prepare-only` just prepares the clone; run
`python3 -S tests/fight_caves_e2e.py checkout` inside it, for example in
PufferTank.
