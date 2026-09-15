# Teaching an agent to finish Fight Caves

Fight Caves sounds like a straightforward assignment: survive 63 waves of enemies, defeat Jad, collect your fire cape. In practice, you've got movement, targeting, protection prayers and a steadily shrinking supply of resources to worry about. I wanted to build a version of that encounter an agent could learn through reinforcement learning, then see how far we could take it. That meant building the game simulation, giving the agent a way to play, and figuring out what to do when its idea of progress differed from mine.

We got to an agent that completed **9,570 of 10,800 full caves: 88.61%**, with a Twisted bow and no food or Prayer potions. The selected run trained for roughly 500 million steps, taking about seven minutes on an RTX 5070 Ti, excluding evaluation. Seven minutes is the cost of that training run. The months of simulator work, failed experiments and parameter searches that made those seven minutes useful are where most of this story lives.

We'll start with the game and work our way down to the ticks, observations and rewards the agent actually used. Then we'll look at the experiments: what improved, what broke, and how we told the difference. Along the way, an agent will spend nearly 200,000 ticks getting about halfway through the cave, healing will undo an impressive amount of damage, and a promising training setup will run into fresh random seeds. The numbers are useful; understanding what produced them is the fun part.

> **VIDEO 1 — Start with the result.** Place a 20–40-second Jad excerpt here, followed by a link to its uncut wave-1-to-completion episode. Capture the selected `0024` checkpoint in its compatible current runtime. Keep player HP, Prayer, active protection and Jad HP legible; identify episode seed/rotation and playback speed. Show a readable prayer response and the final kill. Suggested files: `writeup-assets/current-jad.mp4`, a poster frame, and `writeup-assets/full-cave.mp4`. Use uploaded GitHub video URLs or linked poster images when ready. The archival demo is not footage of this checkpoint.

## What we're asking the agent to do

Fight Caves is a combat encounter in Old School RuneScape. Its waves introduce enemies individually, then combine them so the player has to manage several threats at once. Protection prayers defend against particular attack styles. Positioning determines which enemies can reach you, which can attack from a distance, and whether you can shoot back.

Our simulator uses a 64 × 64 arena and 15 spawn rotations. A tick represents 0.6 seconds of game time. Here's how the main enemies enter the picture:

| First wave | Enemy | What it adds |
|---|---|---|
| 1 | Tz-Kih | Melee attacks and Prayer drain. |
| 3 | Tz-Kek | A large enemy that splits into two smaller ones when killed. |
| 7 | Tok-Xil | Ranged attacks, plus melee at close range. |
| 15 | Yt-MejKot | Melee pressure and healing. Damage doesn't necessarily stay dealt. |
| 31 | Ket-Zek | Magic attacks, contact melee and a large footprint to navigate around. |
| 63 | TzTok-Jad | A final protection-timing test, with healers restoring his HP. |

This gives us two connected learning problems. The agent needs to survive long enough to see Jad, and it needs useful experience fighting him once it gets there. A policy that dies on wave 20 gets very little practice at wave 63. Meanwhile, preserving HP and Prayer takes thousands of smaller decisions whose consequences may arrive much later.

Jad also gives us a useful example of how mechanics turn into strategy. In the current simulation, he has 250 HP and can summon four healers below 150 HP. Each healer can restore 5 HP every four ticks while in healing range. A resolved player hit, even a zero-damage hit, distracts that healer permanently: it stops healing and pursues the player. Changing targets can therefore remove a source of healing while creating another nearby threat.

> **FIGURE 1 — The arena and its progression.** Place an arena map here with actual collision/LOS boundaries and illustrative 1×1, 3×3 and 5×5 footprints. Under it, show the first-appearance waves from the table. Use current runtime maps and `WAVE_TABLE`; label the positions as examples. Export: `writeup-assets/arena-and-waves.svg`.

### What the agent brought into the cave

The current result uses strong equipment. That matters: damage output changes how long an enemy stays alive, and defence changes how many mistakes an episode can survive.

| Equipment or setting | Selected setup |
|---|---|
| Weapon and ammunition | Twisted bow; 50,000 starting Dragon arrows. |
| Armour | Masori mask (f), body (f) and chaps (f). |
| Cape and necklace | Ava's assembler; Necklace of anguish. |
| Gloves, boots and ring | Zaryte vambraces; Pegasian boots; Venator ring. |
| HP, Prayer, Defence and Ranged | 99 each. |
| Attack, Strength and Magic | 1 each in the preset. |
| Food and potions | Zero sharks; zero Prayer-potion doses. |
| Run-energy assumptions | Agility 99; weight 30 kg. |

That's the implemented `FC_LOADOUT_SOTA_TBOW` preset. The policy doesn't choose its gear. With no food or potions, it has to work with its starting HP and Prayer, plus natural HP regeneration. This is a specific simulated task; these results aren't measurements of an agent playing the live client.

> **IMAGE 2 — Starting equipment and resources.** Place a current-runtime equipment screenshot immediately after the table, with separate stats/inventory crops if needed for readability. Capture before the first action and show the empty food/potion supply. Export: `writeup-assets/starting-loadout.png`.

### First, we needed a game we could trust

The project started with a Kotlin/JVM prototype, then moved the simulation to C in March. Offline tools decoded cache data into terrain, objects, collision and visual assets. The C engine handled the game rules; a Raylib viewer let us play and inspect them.

Even getting the cave into the viewer had its surprises. One early object parser didn't recognize two zero-byte flag opcodes, 21 and 22. That caused it to discard 95.7% of the object definitions it read. Fixing the parser increased exported object instances from 205 to 4,382. An awful lot of missing scenery came down to two flags that didn't even have a payload.

For training, the useful separation was between advancing the game and drawing it. The engine owns movement, collision, line of sight, attacks, prayer, healing and wave transitions. The training adapter turns that state into observations and rewards. The viewer displays the same rules. We can run thousands of caves without drawing thousands of windows, then inspect a suspicious interaction without maintaining a second combat implementation.

The early benchmark records show why this was worth doing: C-side throughput at 64 environments increased from 335,800 to 3,353,676 environment steps per second over the initial optimization work. That measures the C simulation alone; network inference and learning add their own costs.

Speed alone wouldn't help if the game rules were wrong. Seeded resets, action replay and field-based state hashes let us check that the same inputs produced the same transitions. Viewer overlays exposed collision, NPC footprints, routes and pending attacks. Those tools became particularly useful once we had a policy willing to repeat an awkward interaction several million times.

> **FIGURE 3 — One simulation, several ways to inspect it.** Place a two-part figure here. A: offline cache exports → collision/arena data and visual assets; shared C state/tick → training adapter, viewer and replay/checker. Route visual assets to the viewer. B: annotate `runescape-rl/assets/readme/viewer-debug-los.png` with footprint, route, collision and LOS callouts; label it archival. Exports: `writeup-assets/engine.svg` and `writeup-assets/viewer-diagnostics.png`. Optional inset: 205→4,382 exported instances, explicitly an asset-parser correction rather than a training result.

## Giving the agent a way to play

A person can read a guide, recognize an enemy and arrive with a plan. Our agent begins with randomly initialized network weights. It learns a **policy**: a function that takes information about the current state and chooses actions.

We give that policy some substantial conveniences. It gets structured state instead of pixels, selects actions directly instead of moving a mouse, and collects experience from thousands of caves at once. Enemy recognition and input execution are much easier through this interface. Discovering which actions lead to a successful cave still takes work. A human can understand a sentence about Jad's prayer timing before ever meeting him; the agent has to learn from the experience its training produces.

### What it can see

Each decision receives 320 floating-point values:

| Block | Values | Information provided |
|---|---:|---|
| Player | 23 | HP, Prayer, position, attack timer, protection, target, resource loss and run energy. |
| Nearby NPCs | 8 × 31 = 248 | Type, HP, position, distance, LOS, attack timing, pending style, prayer deadlines, healing and target state. |
| Wave and progress | 15 | Wave, rotation, remaining enemies, required work and time since progress. |
| Legal actions | 34 | A bit for each movement, attack and prayer choice. |

If you're watching a health bar, the policy gets a number. If you're watching an attack animation, it gets attack-state features. Some of those features are more precise than a human-facing cue: pending-hit deadlines and healing-source flags come directly from the engine. Ordinary-NPC style predictions can also be available from distance without line of sight. This setup doesn't test whether an agent can infer all of that from video.

There are limits on its view. The simulation can hold 16 NPCs, but we expose the eight nearest, ordered by distance to their footprints and then spawn index. Empty slots are zero-filled. “NPC 2” means the enemy in that observation slot, so it can refer to a different enemy after the ordering changes. We resolve a selected slot against the pre-action state, before movement can reshuffle it.

### What it can do

The network chooses one value from each of three action groups, called **heads**:

| Head | Choices | Meaning |
|---|---:|---|
| Movement | 17 | Stay still, walk in eight directions, or run in eight directions. |
| Attack | 9 | Make no new selection, or select one of the eight observed NPCs. |
| Prayer | 8 | Keep, switch off, select one of three protections, or flick one of those protections. |

Selecting a target persists across ticks. Once the agent has picked an enemy, it can keep shooting without selecting that enemy again. Consequently, an attack-head value of zero doesn't mean “no attack happened.” Explicit movement without an explicit attack clears the target, and a shot that actually fires uses the current tile and consumes that tick's movement opportunity.

Prayer flicking is also an explicit action: the engine performs the off/on transition. The policy learns when to use it; the mouse-click sequence is already handled. The current task has no eating, drinking, gear-switching or spellbook action heads.

An **action mask** removes illegal choices before the policy samples an action. We also include those legality bits in the observation. This saves the learner from repeatedly attempting forbidden moves. Standing still and accomplishing nothing is perfectly legal, though. We'll meet that policy shortly.

### Why the order inside a tick matters

Suppose Jad launches a ranged attack at tick `T`. In the current simulation, protection locks at the boundary at `T+2`, before that tick's action. The action at `T+1` can still change the prayer used for the hit. The projectile lands later, with a minimum impact delay of three ticks.

That gives the policy a response opportunity after the tell. It also gives us a deadline when inspecting its behavior. Looking only at the prayer active when a hitsplat appears can blame the wrong action.

The engine makes that order explicit: snapshot protection; process the player's prayer, target, attack and allowed movement; update timers/resources; run NPC behavior; resolve pending hits; handle deaths, waves and healers; advance the tick and lock due prayer decisions. Ordinary NPC attacks use the start-of-tick prayer snapshot at launch. Jad's delayed non-melee lock is a separate rule; melee resolves immediately.

> **FIGURE 4 — From game state to a decision.** Place a human-view/structured-view diagram here. Map appearance, health bars and attack cues to their supplied features, then show 23 + 248 + 15 + 34 inputs → three 512-wide MinGRU layers → 17/9/8 action heads and a value output. Add the recurrent-state loop and a separate mask-to-sampler arrow. Mark exact timing as engine-provided. Beneath it, draw the Jad timeline: launch at T → response at T+1 → lock before action T+2 → impact. Exports: `writeup-assets/policy-interface.svg` and `writeup-assets/prayer-window.svg`.

### Turning experience into updates

After each step, the agent receives a numerical reward. During training, we use those rewards to estimate how well an action worked out compared with what the network expected. That difference is the **advantage**. Updates make actions with positive estimated advantages more likely and actions with negative ones less likely.

The learner uses Proximal Policy Optimization, or PPO, with replay and V-trace corrections. PPO clips its update objective to discourage overly large changes in action probabilities. Replay reuses collected experience; V-trace adjusts estimates when the policy being updated differs from the one that collected it. An entropy term encourages some action diversity during learning.

The network has three recurrent MinGRU layers, each 512 units wide, for 2,541,056 parameters. Recurrence gives it a state it can carry between decisions, so each observation doesn't have to be interpreted in isolation. The actor outputs action probabilities; the critic estimates future discounted reward. Muon performs the parameter updates.

We collect 256 steps from each of 4,096 agents per rollout: 1,048,576 agent steps. Training uses two buffers, 16 environment threads and 32,768-step minibatches. At that rollout size, the requested 500M-step run finished after 476 rollouts, at 499,122,176 steps. We'll return to the settings that made those updates work after looking at what went wrong along the way.

## The route from early runs to useful behavior

The project didn't improve in a straight line. We found strong policies, changed the task or corrected a mechanic, and sometimes had to work our way back. Keeping those conditions attached to the results matters more than arranging every score into an upward-sloping chart.

### Surviving was the first trap

The first recorded run, `xgsb170g`, trained for 500M steps. It reached mean wave 27.6 and earned mean return 554.3, with **zero completions in 10,174 episodes**. Return is the sum of rewards in an episode; a large return only tells us the agent collected what we chose to reward.

A later experiment raised the episode limit from 30,000 to 200,000 ticks and trained for 2B steps. Mean episode length reached 199,939 ticks. Mean wave stayed near 30. Completions stayed at zero, across 4,239 episodes.

We had made room for a very long unsuccessful cave. The summaries don't tell us whether that policy hid, kited or got stuck. They do tell us that extra survival time wasn't turning into completed waves. The cap and training budget both changed, so this is a useful failure record rather than a clean measurement of the cap's effect.

> **FIGURE 5 — A longer episode with almost the same progress.** Place paired summary panels here for `xgsb170g` and `ss966rf9`: mean wave 27.6/30.0, ticks 3,110/199,939, and wins 0/10,174 versus 0/4,239. Label 500M/2B budgets and the changed cap. Source: the run-history entries. Export: `writeup-assets/survival-without-completion.svg`.

### We had strong agents before we had the current task

By early May, a 200-trial search reported an 88.6% final completion rate for `a3mi6u2g`. That was an older setup with food, potions and a much larger collection of shaped rewards. The campaign report records 68.2 hours of search time. The winning run alone doesn't represent what it cost to find the recipe.

Removing supplies began as a diagnostic experiment in May. We wanted to see what happened when food and potions could no longer compensate for damage and Prayer loss. That became an important distinction in later results: a high score with 20 sharks and 32 potion doses answers a different question from a high score with neither.

June supplied another reminder that “same config” can be misleading. An audit found identical configuration files across two snapshots, but different healer rules and three changed collision tiles. The healer trigger had moved from half HP to 150 HP, and re-summoning required Jad to return to full HP. Gear selection was also compiled into the backend. Copying an INI couldn't reproduce all of that.

The subsequent nine-loadout comparison explicitly rebuilt the backend for each loadout. At 1,499,463,680 steps, the SOTA Twisted bow preset completed **90.98%** of 10,033 episodes; a weaker Twisted bow/Masori preset completed **75.81%** of 10,066. Both used the historical full-supply setup. The clean rebuild showed that the newer backend could still train a strong agent. The earlier audit identifies differences worth checking, but doesn't establish which one caused an individual failed rerun.

This is why the experiment record now includes code, assets, executable, loadout, configuration and checkpoint identity. Those details define the game the network actually learned.

> **OPTIONAL FIGURE 6 — How the task changed.** Place a compact annotated timeline here: March simulator work → April stalls → May strong full-supply policies and no-supply diagnostic → June loadout/reproduction audit → July reward/trainer work → current no-supply result. Keep results in separate labeled cards, without a shared performance trend line. For the June gear comparison, use final values from `ov5qfn36.json` and `ymj1j1mi.json`; label full supplies, fixed learner settings and 1.499B actual steps. The May 88.6% is from the campaign report; its raw log has not been recovered. Export: `writeup-assets/project-turning-points.svg`.

### What learning looks like in the current run

With that history in mind, here's the progression inside the selected current run. The native log stores four training-bin averages. Each row summarizes a stretch of training, rather than an independently evaluated checkpoint.

| Mean step location | Mean wave | Mean episode ticks | Reached Jad | Completed cave |
|---:|---:|---:|---:|---:|
| 63.44M | 21.29 | 1,214.46 | 0% | 0% |
| 187.70M | 33.90 | 1,985.52 | 0% | 0% |
| 312.48M | 52.40 | 3,761.38 | 14.64% | 0.19% |
| 437.26M | 62.23 | 5,014.18 | 86.24% | 63.49% |

Early on, episodes ended among the ordinary enemies. In the third bin, the policy reached Jad regularly enough to collect experience there, but almost never finished. By the fourth, most episodes reached him and completion had risen sharply. Access to the fight and competence at finishing it developed at different times.

The final evaluation reached 88.61% completion. Its mean episode lasted 4,930.82 ticks, or 49.31 minutes of simulated game time, averaged over successes and failures. That duration explains some of the appeal of running the simulation faster than real time.

The logs show where performance improved. To explain the specific tactics, we need the matching replay: which targets it picked, when it fired, and what happened to the healers. A rising wave curve can't tell us that the agent discovered a particular safespot.

> **FIGURE 7 — Learning to reach Jad, then finish.** Place aligned wave, episode-length and reach/completion panels here, using the four training bins. Show averaging explicitly. Add separate final-evaluation markers at 499.12M: 89.25% reach and 88.61% completion, n=10,800. For a denser curve, parse `confirm_0024_seed73.log`, deduplicate epoch displays and exclude final-evaluation repeats; label it the exact seed-73 confirmation with rounded console metrics. Export: `writeup-assets/current-learning.svg`.

> **VIDEO 2 — What those improvements look like.** Directly below Figure 7, place identified early, middle and final-checkpoint clips. Show target selection, actual attacks, movement and prayer. Include a final-policy Jad/healer sequence with healing events, healer distraction and Jad HP. Add a 6–12-tick state/action trace beside that clip. Use it to determine whether the policy tags, kills or outdamages healers before adding that behavioral claim. Exports: `writeup-assets/training-stages.mp4` and `writeup-assets/jad-decision-trace.svg`. Historical clips need their own compatible runtimes.

## When a reward curve tells the wrong story

An agent can get better at collecting reward while getting no closer to finishing the cave. It can also stall without collecting much reward at all. Both deserve investigation, but they call for different fixes.

We found it useful to read three things together: the task outcome, the individual reward components, and the behavior producing them. That turns “this run looks strange” into something we can check.

### Paying too much for protection

In `cfuyizo1`, we restored correct-prayer rewards alongside progress feedback. Surviving became more attractive. Unfortunately, continuing the fight didn't become attractive enough.

| Final per-episode measurement | Value |
|---|---:|
| Correct-danger-prayer reward | 86.87 |
| Progress reward | 2.92 |
| Total return after clipping | 63.97 |
| Time without a target | About 83.1% |
| Time without progress | About 73.8% |
| Completions | 0 / 10,043 |

The prayer component was roughly 30 times the progress component before the total was clipped. We had made defending a good way to collect reward even while the fight went nowhere. The agent had no reason to care that this wasn't the gameplay we had pictured; our reward was its feedback.

Later, we had a cleaner test of that incentive. Removing the remaining correct-danger-prayer reward, with the same 750M budget and training seed, improved final completion from **88.02% to 92.74%** (`i215ulj4` → `txqsiahp`). Its weight went from 0.005 to zero. Other shaping stayed active, so the lesson was specific: this extra payment wasn't helping that setup.

> **FIGURE 8 — What the agent was getting paid for.** Place prayer/progress reward bars here, 86.87 versus 2.92, with the no-target/no-progress fractions and zero wins beside them. Label components as measured before total clipping; don't stack them to equal the clipped return. Add a separate matched-removal panel: 88.0188%→92.7431%, n=10,216/10,018. Sources: `cfuyizo1`, `i215ulj4`, `txqsiahp`. Export: `writeup-assets/prayer-reward-diagnosis.svg`.

### Healing can erase a lot of apparent progress

Hard action masks helped us rule out one explanation for another unsuccessful run. In `l2l7lf6b`, all 2,382 logged history rows reported zero invalid movement, attack and prayer actions. Legal actions were getting through. Useful combat was still a problem.

During the 1.75–2.0B-step window, NPCs healed an average of 156,930 internal HP units per episode against 178,834 units of gross damage. Healing erased the equivalent of **87.75% of damage dealt**. Later, in the final 100M steps before the terminal flush, the policy spent 98.59% of ticks without a target and reached a mean wave of only 3.32.

Those are two different symptoms: damage being undone, followed by disengagement. To call the first a profitable reward exploit, we have to check what healing did to the reward in that run. High healing by itself doesn't establish a profit.

We tried charging 1.1 times as much for negative progress instead of 1.0. At the same 2,499,805,184 steps and seed 73, final mean wave increased from 3.75 to 25.27; mean cave progress rose from 0.0503 to 0.3953. Both runs still had zero completions. It helped that comparison, but didn't finish the job. The current reward uses multiplier 1.0.

> **FIGURE 9 — Damage, healing and disengagement.** Place same-window damage/healing bars here, either in labeled internal tenths of HP or both divided by ten. Separately show late-window attack-none 98.81% and no-target 98.59%; these overlap, so don't stack them. Add the final multiplier comparison, including zero wins in both runs. Source: the `l2l7lf6b` windows in `fc_revamp.md` and final JSON metrics for `l2l7lf6b`/`ruuq4231`. Export: `writeup-assets/healing-and-stalling.svg`. An authenticated historical loop replay would be a useful optional companion.

### Sometimes the game needs fixing

Before increasing a stall penalty, it helps to check whether the enemies can actually complete the interaction we're asking for.

A July movement/combat correction addressed cases where an NPC could be in nominal attack range but lack line of sight or valid melee contact, and still fail to keep chasing. With the reward, learner, vector and policy configurations held fixed, final mean wave increased from **40.82 to 49.17** (`mzqf7iml` → `7mxnrzua`, 1.499B steps each). Jad reaches went from zero to about nine in 10,021 episodes; completions remained zero. Correcting the interaction let the agent get farther, even though more learning work remained.

Geometry needed precise rules. The later parity work made melee depend on cardinal contact between rectangular footprints, checked movement edges in all eight directions for NPC sizes 1–5, and used sequential occupancy so an NPC could enter a tile vacated earlier that tick. A creature that looks “next to” the player on screen may still have an invalid diagonal contact or a wall between them. The [movement and parity record](runescape-rl/docs/archive/fc_cleanup_and_parity_history.md) documents those cases.

Combat formulas needed the same attention. Natural regeneration had been restoring one HP every 10 ticks; we corrected it to every 100 ticks. A follow-up achieved 82.51% completion, but also doubled the training budget to 1.5B, so it doesn't isolate the regeneration change. A cleaner comparison added the missing +60 Magic attack bonus to Ket-Zek and Jad's Magic attacks: at the same 750M budget and seed, completion moved from 92.74% to 91.49%. A slightly lower score was the right outcome to accept for a more accurate model.

This made the agent useful as a source of awkward test cases. The workflow was to locate a repeated failure in the metrics, inspect the state/action trace, reproduce it in a deterministic fixture, and then check the relevant rule. A collapse could come from rewards, geometry, observation timing or the learner. Turning a penalty knob before doing that work could hide the actual problem.

> **FIGURE 10 — A concrete mechanics correction.** Place before/after geometry sketches here using a recovered deterministic fixture for the July chase/contact issue. Mark footprints, blocked edges, attack validity and next movement. Beside them, plot mean wave 40.8176→49.1724, with n=10,070/10,021 and zero completions. Sources: `mzqf7iml.json`, `7mxnrzua.json`, the “Backend movement/positioning fix” entry in `fc_revamp.md` and matching historical code. Label a reconstructed diagram as a schematic if the fixture cannot be recovered. Export: `writeup-assets/movement-correction.svg`.

## Giving progress a useful meaning

We could reward only the final kill. That would describe the objective neatly, but an agent that rarely reaches Jad would spend most of its time collecting no positive feedback. This is **sparse** reward: the useful signal arrives rarely. **Dense** reward gives feedback along the way. We could pay for every apparently useful action, for example, though the prayer experiments showed how that could go wrong.

Removing damage rewards wasn't enough by itself. In `eifkgdut`, we emphasized kills and progress events and added a healing penalty. Final mean wave fell from about 28.05 in the preceding observation experiment to 21.68, with about 83.64% of episode time spent without a target. Other feedback remained active, so this wasn't a pure sparse-reward test. That particular replacement wasn't sustaining combat.

The design we kept gives dense feedback for **reducing the work still required to finish the cave**. That definition handles several awkward cases in one place.

### Count what remains

Imagine dealing 10 HP of damage to an enemy, then watching it heal those 10 HP back. You're back where you started. A reward based only on damage dealt can treat the next 10 HP as another accomplishment. A reward based on remaining work can charge back the progress that healing undid.

For ordinary waves, required work includes living enemies' HP. A large Tz-Kek also carries the HP of the two children it will spawn, so splitting it doesn't create unexpected new work. On the last wave, required work is Jad's HP. Killing his healers isn't required to finish; their healing matters because it restores Jad's HP.

Here's the calculation. `W` is the remaining work, measured in the simulator's internal tenths of HP. `S` is the work at the start of the wave. `q` measures progress through that wave, and `C` combines it with the waves already cleared:

```text
q = clamp(1 - W / S, 0, 1)
C = ((wave - 1) + q) / 63
progress reward = 0.001 × (C_now - C_previous) × 63 × S
```

The factors `63 × S` undo the normalization when we turn a change in cave progress back into work. Within a wave's unclamped range, the reward reduces to 0.001 times the net work removed. The wave-transition code resets the comparison baseline so spawning a fresh wave doesn't count as moving backward; completing the cave sets `C` to one.

Ten game HP is 100 internal units, so removing it earns **+0.1** progress reward. Restoring it costs **−0.1**, plus **−0.005** for one effective heal event. Those are component values before the trainer clips the combined reward. We can now give frequent feedback without treating the same restored HP as fresh progress every time it's damaged.

> **FIGURE 11 — The accounting behind the reward.** Place the 10-HP damage/heal example here: remaining work before damage, after damage and after healing, with +0.1/−0.1 and the separate −0.005 event cost. Include a large Tz-Kek branching into two already-counted children, and a Jad/healer inset showing why healer HP isn't mandatory work. Label this a code-derived example. Export: `writeup-assets/required-work.svg`.

### The rest of the reward budget

Progress is the main positive feedback, but resources and inactivity also matter. The selected configuration uses these terms:

| Term | Current rule |
|---|---|
| Net progress | The required-work calculation above, weighted by 0.001. |
| Completion / death | +1 / −1. |
| Damage taken | −0.25 × fraction of maximum HP lost that tick. |
| Prayer lost | −0.02 per game Prayer point. |
| Time | −0.0001 per tick. |
| Effective NPC healing | −0.005 per positive healing event. |
| No positive progress | Beyond 800/1,600/2,400 ticks, add −0.001/−0.005/−0.02 per tick; tiers accumulate. |
| No attack | Once the ready-without-attack counter exceeds 50, −0.005 × [1 + 0.05 × (wave−1)]. |
| Invalid action | −0.1 if an invalid choice occurs; masks normally prevent it. |

The no-attack counter increments when enemies remain and the attack timer is ready; an attack attempt resets it. Fifty ordinary cooldown ticks therefore don't mean the same thing as fifty counted opportunities to attack. The wave scaling raises the active penalty to −0.0205 at Jad. The no-progress tiers similarly distinguish a brief gap from sustained inactivity: their cumulative costs are −0.001, −0.006 and −0.026 per tick after each threshold is exceeded.

Gross damage, NPC kills, wave clears, separate Jad kills and correct prayers have zero direct reward weights in this setup. The environment sums the active components. The trainer then clamps each rollout reward to [−1, +1] before learning. Keeping those two stages separate is important when comparing a component breakdown with the return used for training.

### A good agent creates longer episodes

As the current policy improved, mean training episodes grew from about 1,214 to 5,014 ticks. With a fixed step budget, that means fewer fresh episodes and terminal outcomes. More of the collected experience comes from later waves, but only after the policy has learned to reach them.

Small costs also accumulate. A −0.0001 tick penalty adds up to about −0.12 over 1,200 ticks and −0.50 over 5,000. We didn't automatically shorten episodes, equalize their reward totals or scale those costs away. We had to check their totals against actual progress as the policy got farther.

Discounting adds another reason to provide intermediate feedback. The discount factor, gamma, reduces the weight of rewards farther in the future. At the selected gamma of about 0.999126, a reward 5,000 ticks away has a direct multiplier of only 0.0126. The critic can learn and bootstrap intermediate value estimates, but it still needs useful experience to learn from. Dense work-based rewards, parallel caves and recurrent rollouts gave the learner feedback along the route to Jad.

### Keep the observation useful, too

We applied the same discipline to observations. The core has a larger 475-value diagnostic layout; the policy uses its own 320-value view. The 20 raw reward-feature channels stay out of that input. Debugging information can be useful to us without becoming another feature the network has to interpret.

The selected setup zeros nine aggregate incoming-hit channels while retaining per-NPC attack style, timing and prayer deadlines. That removes a redundant summary without removing the individual threats. NPC distance and validity remain enabled. We can point to exactly what we excluded; establishing whether every retained feature is necessary would require separate ablations. Recurrence deserves that test as well.

## The training settings that moved the result

Once the task and feedback were in better shape, the training recipe still mattered enormously. A **sweep** is a collection of training runs with different settings. It gives us a way to search combinations, though changing several settings together doesn't tell us which one deserves all the credit.

### July: the same task, a very different result

The clearest example came from a 140-trial search. The baseline and selected run had identical environment, reward, observation, vector and model settings, and both used 749,731,840 steps. Final completion went from **21/10,054 (0.2089%) to 9,561/10,003 (95.5813%)**.

Several changes in the recipe help explain what we were searching:

| Setting | Baseline `8rg9wurg` | Selected `mmyxbyn4` | Why it matters |
|---|---:|---:|---|
| Learning rate | 0.000939323 | 0.002075675 | Controls the scale of parameter updates. |
| Entropy coefficient | 0.00644464 | 0.000625461 | Weights the incentive to keep action choices diverse. |
| Gamma / GAE lambda | 0.999518847 / 0.9995 | 0.999126114 / 0.9 | Control discounting and how advantage estimates combine rewards with value predictions. |
| Minibatch size | 4,096 | 32,768 | Changes how much experience contributes to an update. |
| Policy clip | 0.132360 | 0.05 | Changes when PPO's probability-ratio objective is clipped. |
| Replay ratio | 1.333590 | 2.055184 | Changes how much collected experience is reused. |

V-trace limits and replay-priority settings changed too; the exact values are in the appendix. The successful region favored lower entropy, GAE 0.9, horizon 256 and larger minibatches. These gave us directions for the next search, with the individual effects still to be separated through matched experiments.

This was a turning point for the simpler reward setup. It also used the older, faster HP regeneration rule; that belongs beside its 95.58% when we report the result.

> **FIGURE 12 — Searching the training recipe.** Place a July trial-index versus final-completion scatter here, with baseline and selected run marked. Recover the 140-run membership from `v3_simple_reward_sweep.md` and the JSON logs first; if incomplete, use labeled baseline/selected summary dots. Add a separate current-campaign panel after the current-search discussion below: 72.4608% baseline, 88.6111% selected, 90 attempts. Mark current trial `0008` as invalid, outside the valid-score plot. Keep each campaign's task/budget in its caption. Exports: `writeup-assets/july-search.svg` and `writeup-assets/current-search.svg`.

### August: look at how it got there

A later 130-trial search explored architecture, agent count and value/optimizer settings. All eight highest-final performers used three recurrent layers; seven used width 512 and six used 4,096 agents. That concentration helped guide the architecture choice.

We also started looking more carefully at the training history behind a high final score:

| Run | Final evaluation | Final-quarter training mean | Final-quarter training q10 | First recorded ≥90% |
|---|---:|---:|---:|---:|
| Selected `1nvvx5qu` | 94.79% | 94.98% | 93.20% | 316.7M steps |
| `pozhjer2` | 96.41% | 56.80% | 9.04% | 692.1M steps |

Here, q10 is the tenth percentile of the logged training measurements: a way to see the lower part of that window's performance.

The selected run learned high completion earlier and maintained it through the final quarter. The other learned successfully much later. A final-score leaderboard would miss that distinction. The [August analysis](sweep_top8.md) retains the full comparison.

> **FIGURE 13 — Similar endpoints, different learning histories.** Place final-quarter mean/q10 markers and distinct final-evaluation dots here, using the two runs above. Annotate first-90% steps and 4,096/8,192 agents. Label q10 as a training quantile, not evaluation uncertainty. Replace with dense curves only after retrieving the original histories. Export: `writeup-assets/stability-versus-endpoint.svg`.

### The current recipe

The latest search fixed the task, model, agent count, horizon, minibatch, gamma/GAE, V-trace settings and training seed, then varied eight optimizer/update parameters. Of 90 attempts, 89 produced finite checkpoints; one became numerically unstable. The selected run improved final completion from the prior **72.46% to 88.61%** at the same roughly 500M-step budget.

An earlier comparison also tested how we collected experience and updated the network. Asynchronous training allows those jobs to overlap; synchronous training coordinates collection and updates. We paired that choice with whether the recurrent network kept its state between 256-step horizons:

| Training mode | Keep recurrent state across horizons | Reset it each horizon |
|---|---:|---:|
| Asynchronous | 0% | 0.0391% |
| Synchronous | 63.53% | 72.4608% |

These were same-seed 500M tests. We kept synchronous training with horizon resets, clearing the carried network state at each horizon. Final evaluation carries state across horizons. This combination worked best in the tested recipe; we'd need more seeds to estimate how dependable that advantage is.

Some settings clearly helped as part of a combination, but the evidence doesn't support a tidy ranking of every knob from most to least important. For example, August's strong runs used a broad range of value-loss coefficients. That shows several workable combinations, not that the coefficient has no effect. Parameters held fixed in a sweep haven't been tested for irrelevance either.

The next useful step is smaller, matched experiments across fresh training seeds. The seed results below explain why that matters more than another decimal place on the best score.

## How consistently does it finish?

For the current final evaluation, we reloaded the selected checkpoint and started full caves at wave 1. Actions were sampled from the masked categorical distributions, with recurrent state carried across horizons. Evaluation requested at least 10,000 completed episodes; the batched run finished with 10,800.

| Outcome | Episodes | Share |
|---|---:|---:|
| Failed before Jad | Approximately 1,161 | 10.75% |
| Reached Jad but failed | Approximately 69 | 0.6389% |
| Completed the cave | 9,570 | 88.6111% |

The recorded Jad-reach rate was 89.25%, corresponding to about 9,639 arrivals. Of those, about **99.28% finished the cave**. The arrival/failure counts are reconstructed by rounding aggregate floating-point rates.

For these weights, most failures happened before Jad. That points us toward the earlier waves and the states the policy creates there. When this policy arrived at the final encounter, it usually finished.

> **FIGURE 14 — Where the selected checkpoint fails.** Place a 100% stacked outcome bar here: 1,161 pre-Jad failures, 69 Jad failures, 9,570 completions. Label reconstructed counts, n=10,800 and the conditional denominator of 9,639. Put the tiny Jad-failure label outside its segment. Export: `writeup-assets/final-outcomes.svg`. Per-wave/rotation breakdowns need episode records before they can be added.

### A reliable checkpoint and a reliable training recipe

Once we have a good set of weights, we can replay many caves to measure it. Starting training again asks a different question: will the same recipe find another good set of weights?

We tested three finalist recipes with training seeds 73, 101, 202 and 303:

| Recipe | Seed 73 | Seed 101 | Seed 202 | Seed 303 | Equal-weight mean |
|---|---:|---:|---:|---:|---:|
| 0024 | 88.61% | 38.51% | 74.56% | 81.07% | 70.69% |
| 0084 | 83.75% | 87.65% | 88.44% | 7.44% | 66.82% |
| 0028 | 80.23% | 44.29% | 79.24% | 3.91% | 51.92% |

All twelve runs produced finite weights. Seed 73 reproduced each recipe's original checkpoint, but it was also the seed used during selection. The three fresh seeds are the more useful test of how reliably the recipe learns. For the selected `0024` recipe, their mean completion was 64.72%.

Recipe `0084`, seed 303, makes the distinction particularly clear. It reached Jad in 81.19% of episodes and completed only 7.44%, about 9.16% conversion after arrival. Its recorded training dashboards never showed strong completion performance. It learned to get to the boss much more reliably than it learned to finish him.

So we have a checkpoint that wins repeatedly, and a training recipe with substantial variation. Comparing strong and weak seeds' HP and Prayer on arrival, prayer deadlines, targets and healer events is the next useful diagnosis. Another aggregate score won't tell us which decision differs.

> **FIGURE 15 — Show every training seed.** Place paired Jad-reach/completion markers for all twelve confirmations here, grouped by recipe and labeled by seed. Mark seed 73 as used for selection and highlight the `0084`/303 gap. Read final `env/jad_kill_rate`, `env/reached_wave_63` and `env/n` from the confirmation INIs. Keep runs separate rather than pooling episodes into one success rate. Export: `writeup-assets/training-seeds.svg`.

## Taking this to another encounter

I'd like to reuse this work for other RuneScape encounters. The simulation, viewer, replay tools and training loop give us a useful starting point. The exact observations, rewards and learned weights need a separate argument.

Even changing gear inside Fight Caves showed why. In the historical no-supply Bowfa diagnostic (`t2rtfitt`), the final checkpoint reached Jad in 96.4% of episodes and completed none, while the preceding Twisted bow run (`jta3lkgx`) reached him in 68.5% and completed 43.2%. The swap changed attack cadence and the backend's target-dependent damage calculation: faster attacks came with different damage against the late enemies. Those historical run reports give us a concrete transfer problem to investigate. The learner gets more practice at the boss, yet still fails to finish him.

The Inferno is an obvious direction to explore: another wave encounter, with its own enemies and rules, as described in Jagex's [revised design post](https://forums.rs/en/380%2C381%2C256%2C65733712.html). I'd expect the same approach to building and checking a simulator to be useful. I'd also expect to revisit the observation capacity, objectives and reward accounting before spending time tuning the learner.

| Proposed extension | What we could reuse | What we'd need to work out |
|---|---|---|
| A more demanding wave encounter | Ticks, collision/LOS, pending hits, batched training and wave metrics. | New enemies, hazards, protected objectives and enough observation capacity. |
| Multiple Jad-style threats | Attack-style/deadline features and protection actions. | Whether overlapping demands allow a feasible strategy, and how targeting changes it. |
| A boss requiring equipment or style changes | Combat formulas, viewer and event/replay diagnostics. | Switching actions, inventory observations and the delays/costs of those actions. |
| A multi-stage or cooperative encounter | Simulation and validation infrastructure. | Persistent resources, stage objectives, other players' state and possibly a multi-agent learner. |

These are future experiments. A sensible first test would implement one bounded encounter, then compare training from scratch with a transferred initialization under the same budget. That would let us measure whether the old policy helps, beyond the engineering time saved by reusing the framework.

> **OPTIONAL FIGURE 16 — What carries over.** Place a small diagram here separating reusable simulation/training/diagnostic components from the mechanics, observations, actions and rewards that need encounter-specific work. Mark proposed encounters as future work. Export: `writeup-assets/encounter-transfer.svg`.

## What I'll take from this project

The most useful habit was checking what a number actually meant. Survival could mean spending almost 200,000 ticks without finishing. Damage could be healed away. A reward curve could mostly measure successful prayer use. Even reaching Jad could hide a policy that lost every fight once it got there.

We made better progress when we could connect those measurements to the underlying decisions. A shared simulator and deterministic tests helped us check the rules. Required-work rewards gave damage and healing a sensible relationship. Parameter searches found recipes that learned effectively, and repeated training exposed how much those recipes still depended on the seed.

The result is an agent that completes most full caves under a documented no-supply setup. There's still useful work ahead: understand the weak seeds, test the dependence on exact timing information and fixed gear, and see what transfers to another encounter. For now, we can load the selected weights, start at wave 1 and watch the agent finish the cave over and over. Getting there taught us a lot more than the final percentage can fit on its own.

---

## Sources and reproduction notes

The evidence here was reviewed through **September 14, 2026**. Current mechanics and learner details use the latest native simulation snapshot. Historical scores keep their original gear, supplies, mechanics and budgets; they're milestones rather than one directly comparable leaderboard.

The media blocks are production instructions at the intended insertion points. Suggested filenames are future exports, not existing recordings or plots.

### Code and experiment records

- Current pinned source: [simulation and policy interface](https://github.com/jordanbailey00/PufferLib/blob/9ad3633d3563a8e467026160e90209146d2a733c/ocean/fight_caves/simulation.h), [adapter](https://github.com/jordanbailey00/PufferLib/blob/9ad3633d3563a8e467026160e90209146d2a733c/ocean/fight_caves/binding.c), [network/optimizer](https://github.com/jordanbailey00/PufferLib/blob/9ad3633d3563a8e467026160e90209146d2a733c/src/algo.cu), [trainer](https://github.com/jordanbailey00/PufferLib/blob/9ad3633d3563a8e467026160e90209146d2a733c/src/pufferl.cu). The selected settings below supersede the commit's original training defaults.
- Historical context: [run history](runescape-rl/docs/run_history.md), [spring sweep history](runescape-rl/docs/sweep_history.md), [movement correction](runescape-rl/docs/archive/fc_movement_combat_correction), [mechanics/parity work](runescape-rl/docs/archive/fc_cleanup_and_parity_history.md), [August sweep analysis](sweep_top8.md), [experiment configurations](runescape-rl/config/experiments).
- Current selected run: [`f146cdcf307ed881` on W&B](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/f146cdcf307ed881), native trial `sweep_1789246135940_0024`, seed 73.
- Historical reward cases: [`cfuyizo1`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/cfuyizo1), [`l2l7lf6b`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/l2l7lf6b), [`ruuq4231`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/ruuq4231), [`eifkgdut`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/eifkgdut).
- Matched comparisons: July [`8rg9wurg`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/8rg9wurg)/[`mmyxbyn4`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/mmyxbyn4); prayer removal [`i215ulj4`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/i215ulj4)/[`txqsiahp`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/txqsiahp); Magic correction [`r16nvaq7`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/r16nvaq7)/[`399wemq3`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/399wemq3).

<details>
<summary>Exact current training settings and historical comparison details</summary>

The selected run used 4,096 agents, two buffers, 16 environment threads, horizon 256, minibatches of 32,768 and a three-layer, 512-wide MinGRU. Training was synchronous with recurrent-state resets each horizon; final evaluation carried state across horizons. Learning-rate annealing was disabled.

| Parameter | Selected value |
|---|---:|
| Learning rate | 0.00123960734 |
| Muon momentum | 0.991584122 |
| Entropy coefficient | 0.00026363763 |
| Replay ratio | 2.20972681 |
| Policy clip | 0.0250000004 |
| Maximum gradient norm | 0.217330709 |
| Value-loss coefficient | 0.275081992 |
| Value clip | 0.0884169564 |
| Gamma / GAE lambda | 0.9991261141073255 / 0.9 |
| V-trace rho / c limits | 2.0 / 0.9746667741536915 |

The July comparison also changed V-trace rho/c from 0.5/0.503727 to 2.0/0.974667, replay-priority alpha from 0.968236 to 0.911074, and beta0 from 0 to 0.225813. Those differences belong to the same multi-parameter change as the main table.

The current training-mode comparison's final counts were: asynchronous/reset-off 0/12,048; asynchronous/reset-on 4/10,227; synchronous/reset-off 6,353/10,000; synchronous/reset-on 7,441/10,269. These are individual same-seed experiments, not multi-seed estimates of each mode's average performance.

The current selected run requested 500M steps and completed 499,122,176. Its recorded training time was 434.08 seconds on an RTX 5070 Ti, excluding final evaluation and the cost of searching for the recipe.

</details>

<details>
<summary>Local sources for making the figures and capturing the policy</summary>

These paths are relative to the local `fight-cave-rl_clones` workspace. They are retrieval notes; several raw logs and reports aren't published in this GitHub repository.

| Evidence | Local source |
|---|---|
| Current log/config | `fc5-protein-90x500m/runs/fc5_protein_90x500m_async0_reset1_r2/logs/fight_caves/sweep_1789246135940_0024.ini` |
| Selected weights | Same campaign, `checkpoints/fight_caves/sweep_1789246135940_0024/0000000499122176.bin` |
| Executable/assets/source provenance | Same campaign, `manifest.json`; `fc-rl-5.0/baseline.md` |
| Current search outcomes | Same campaign, `trial_status.jsonl` and trial INIs |
| Seed confirmations | `fc5-seed-confirmation-12x500m/logs/fight_caves/confirm_<trial>_seed<seed>.ini` |
| Denser rounded current training display | `fc5-seed-confirmation-12x500m/console/confirm_0024_seed73.log` |
| Seed analysis | `TEMP_FC5_FOCUSED_SWEEP.md` |
| Historical raw metrics | `v38/pufferlib_4/logs/fight_caves/<run_id>.json` |
| Reward redesign and diagnostic windows | `v38/runescape-rl/docs/archive/fc_revamp.md` |
| July campaign report | `v38/sweep_history/v3_simple_reward_sweep.md` |
| Early parser, architecture and speed history | `v38/runescape-rl/docs/archive/history.md` |
| Configuration/mechanics reproduction audit | `v38/runescape-rl/docs/archive/SOTA_DIFF_AUDIT.md` |
| Current maps | `fc-rl-5.0/resources/fight_caves/runtime/fightcaves.{collision,movement,los}` |

Selected weight SHA-256:

```text
963d9949a3c1d954fb2c746860a226c0c34915f1f19b1b2f4ffb15e56a97d568
```

Recorded training executable SHA-256:

```text
474850506a21b5022e2ed4b598e997ef1fc0c895945e37e81d2ee0514d2eb64d
```

For native INIs, read comma-separated arrays in `[metrics]`. The selected log's first four entries are training-bin averages; the last is final evaluation. Historical JSONs put configuration sections and `metrics` at the root. Keep the logged window attached to each measurement. The early May sweep result and the historical Bowfa comparison above come from retained reports; their raw run logs weren't available in this review.

For each video, retain checkpoint, compatible code/executable/assets, episode seed/rotation, action-sampling mode, initial recurrent state, source ticks, outcome and playback speed. CPU and GPU sampling need not produce identical trajectories. Use the matching replay path when claiming reproduction of a particular policy episode.

Use rates ×100 for percentages, internal HP tenths ÷10 for game HP, and ticks ×0.6 for simulated seconds. Keep simulated time, training wall time and agent steps distinct. Export measured graphs with standard plotting tools and retain the extracted data/scripts alongside them.

Resource-at-Jad plots, duration distributions and wave/rotation heatmaps need per-episode records. The current aggregate logs can't supply those distributions. Replays also need to be inspected before filling in claims about the policy's particular safespot or healer strategy.

</details>
