# Teaching an agent to finish Fight Caves

In Old School RuneScape's Fight Caves, you work through 63 waves of enemies, with Jad waiting at the end. We set out to turn that encounter into a reinforcement learning environment and train an agent to finish it consistently. That meant building a C simulator, a viewer where we could inspect what was happening, and a training setup where the agent could learn movement, targeting and protection prayers through experience.

The best agent in our latest search completed **9,570 of 10,800 full caves (88.61%)**, using a Twisted bow and no food or Prayer potions. It trained for roughly 500 million steps with PPO and a recurrent MinGRU network, taking about seven minutes on an RTX 5070 Ti, excluding evaluation. Finding a setup that could do that took months of building, testing and working through runs that went nowhere—or got very good at something we hadn't intended to reward.

This writeup follows that journey: how we built and checked the simulator, chose what the agent could observe and do, and worked through reward exploits, stalled policies and training settings. We'll use the experiment data to explain what changed, what helped and what remains uncertain. That includes one of the more interesting loose ends: an agent can learn to beat Jad repeatedly, while another training run with the same settings can still struggle to get there.

> **VIDEO 1 — Opening successful cave.** Insert a 20–40-second excerpt from the selected trial `0024`, showing a readable Jad prayer response and the final kill. Keep player HP, Prayer, active protection and Jad HP visible. Identify checkpoint, episode seed/rotation and playback speed. Link the full, uncut wave-1-to-completion episode underneath. Capture with the compatible current runtime; the existing archival demo is not tied to this checkpoint. Suggested files: `writeup-assets/current-jad.mp4` and `writeup-assets/full-cave.mp4`. Use an uploaded GitHub video URL or a linked poster image when the footage is ready.

## What is Fight Caves?

Fight Caves is a sequence of 63 combat waves from Old School RuneScape. The early waves introduce individual enemy types; later waves combine them. The last wave contains TzTok-Jad, whose attack styles require different protection prayers, and healers that can undo damage dealt to him.

The interesting part for reinforcement learning is the combination of timescales. Protecting against one attack is an immediate decision. Preserving enough HP and Prayer to survive the cave requires many such decisions to work together. Learning Jad adds another dependency: an agent starting from wave 1 must first become good enough at the preceding waves to encounter him regularly.

Our current simulation uses a 64 × 64 arena, 15 spawn rotations and the following progression:

| First wave | New enemy | New demand |
|---|---|---|
| 1 | Tz-Kih | Melee attacks and Prayer drain. |
| 3 | Tz-Kek | A defeated large Tz-Kek splits into two smaller enemies. |
| 7 | Tok-Xil | Ranged pressure and contact melee. |
| 15 | Yt-MejKot | Melee pressure and healing that can reverse damage. |
| 31 | Ket-Zek | Magic pressure, large footprints and contact melee. |
| 63 | TzTok-Jad | Protection timing while continuing the fight and dealing with healers. |

This structure makes the cave useful as a training environment. There are clear intermediate measurements—wave reached, remaining work, resources, damage and healing—but none individually guarantees success. A policy can become very good at staying alive, dealing damage, or reaching Jad while remaining bad at finishing the task.

In the current implementation, Jad has 250 HP and spawns up to four healers below 150 HP. Each can restore 5 HP every four ticks while within its healing range. A resolved player hit, including a zero-damage hit, permanently distracts that healer onto the player and stops its healing. This creates a concrete interaction between targeting, damage and survival: changing a target can reduce future healing while adding a pursuing enemy. Which response the policy actually learned has to be established from its replay.

> **FIGURE 1 — Arena and progression.** Place an arena map here, with actual terrain and illustrative 1×1, 3×3 and 5×5 NPC footprints. Beneath it, add a wave strip marking 1, 3, 7, 15, 31 and 63. Use the current collision/LOS maps and `WAVE_TABLE`; label example positions as illustrative rather than a learned route. Suggested export: `writeup-assets/arena-and-waves.svg`.

### The agent's equipment, stats and inventory

The current result uses the simulator's `FC_LOADOUT_SOTA_TBOW` preset. Strong gear is part of the experiment, and matters when interpreting both survivability and damage output.

| Slot or setting | Selected setup |
|---|---|
| Weapon and ammunition | Twisted bow; 50,000 starting Dragon arrows. |
| Armour | Masori mask (f), body (f) and chaps (f). |
| Cape and necklace | Ava's assembler; Necklace of anguish. |
| Gloves, boots and ring | Zaryte vambraces; Pegasian boots; Venator ring. |
| HP, Prayer, Defence and Ranged | 99 each. |
| Attack, Strength and Magic | 1 each in the preset definition. |
| Supplies | Zero sharks and zero Prayer-potion doses. |
| Run-energy assumptions | Fixed Agility 99 and weight 30 kg. |

These are the implemented preset values. They are not a claim that every item/stat interaction exactly matches the live game. Earlier experiments also used different gear and supplies, so their scores need their original task conditions attached.

> **IMAGE 2 — Starting loadout.** Insert a current-runtime screenshot of the equipment, stats and inventory panels immediately after this table. Show the exact preset and empty food/potion supply, before an episode begins. If the panels cannot fit legibly in one image, use an equipment crop and a small stats/inventory crop. Suggested export: `writeup-assets/starting-loadout.png`.

### Building an engine that could be trained and inspected

The project began with a Kotlin/JVM prototype and moved to a C simulation in March. Cache decoding remained an offline asset-processing problem: models, terrain, objects, animation and collision were exported into runtime data. Combat and state transitions ran in C; the viewer provided the visual interface.

That separation was important for both speed and correctness. Training needs to step many environments without rendering them. Debugging needs to expose the same underlying mechanics in a form a person can inspect. The simulator therefore owns player and NPC state, movement, collision, line of sight, pending hits, prayer, healing, waves and terminal events. The training adapter publishes observations, actions, rewards and episode boundaries. The viewer displays state and translates controls; it does not implement a second combat system.

The current tick is explicit. It snapshots protection, processes player prayer/target/attack and permitted movement, updates timers and resources, runs NPC behavior, resolves pending hits, handles deaths and wave transitions, and locks any due prayer decisions before the next action. A player attack that fires uses the current tile and suppresses movement for that tick. Projectile arrival and the point when protection is decided are separate events.

Seeded simulation and action replay made these rules inspectable. State hashes help detect divergence; viewer overlays expose footprints, routes, LOS, observations, reward components and event logs. A suspicious policy can therefore be investigated as a sequence of state transitions rather than just a low score.

> **FIGURE 3 — Engine and diagnostics.** Insert a two-part figure here. Part A: offline cache exports → arena/collision data and visual assets; shared C state/tick → training adapter, viewer and replay/checker. Route visual assets to the viewer. Part B: annotate the existing `runescape-rl/assets/readme/viewer-debug-los.png` with callouts for footprint, route, collision and LOS. Label Part B as an archival debugging view. Suggested exports: `writeup-assets/engine.svg` and `writeup-assets/viewer-diagnostics.png`.

## Training harness: translating the game into a learning problem

A human and this agent face related combat decisions through substantially different interfaces. A person can recognize enemies, read guides, remember prior attempts and form a plan before entering the cave. They must also interpret the display and execute the right input at the right time. The agent starts from randomly initialized weights, but receives structured state and can collect experience from thousands of environments simultaneously.

Those differences make some parts easier and others harder. For the agent, identifying a supplied NPC type or selecting an explicit protection action avoids the visual recognition and mouse-control problem. Learning which combination of legal actions leads to a distant outcome is still difficult. For a human, understanding a rule such as “use this protection against this attack” can sharply reduce exploration, while maintaining attention and execution throughout a long attempt remains demanding. This is a qualitative comparison of the interfaces; we did not measure a human learning curve against the agent's.

### Observations: the same decisions, different information

The policy receives **320 floating-point inputs**, organized as follows:

| Observation block | Size | Examples |
|---|---:|---|
| Player | 23 | HP, Prayer, position, attack timer, protection state, current target, resource loss and run energy. |
| NPCs | 8 × 31 = 248 | Type, position, HP, distance, LOS, attack timer, pending attack style/timing, prayer deadline, healing and target state. |
| Wave and progress | 15 | Wave, rotation, remaining enemies, wave/cave progress, remaining required work and time since progress. |
| Action legality | 34 | One legality bit for each movement, attack and prayer choice. |

A human uses enemy appearance and animation to infer what is happening. We supplied NPC identity and structured attack information. A human sees health bars and notices healing; the policy receives HP and explicit healing-related features. A human interprets terrain and distances; the policy receives coordinates, NPC distance/LOS and legality information rather than the rendered image.

The conversion is deliberately imperfect as a model of human perception. Some features, including exact pending-hit deadlines and healing-source flags, come directly from the engine. Ordinary-NPC style predictions can be available from distance even without LOS. The policy therefore has information a pixel-based agent would need to infer.

There are also information limits. The simulator can hold 16 NPCs, but the observation includes the eight nearest, sorted by distance to their footprint and then spawn index. Empty slots are zero-filled. A slot's position in the observation is not a permanent identity. The selected action is mapped to the pre-action NPC before movement, so moving cannot silently change which enemy that action selected.

### Actions: movement, targeting and protection

The policy has three categorical action heads:

| Head | Choices | Control provided |
|---|---:|---|
| Movement | 17 | Idle, eight walk directions, eight run directions. |
| Attack | 9 | Keep the existing selection or select one of eight observed NPCs. |
| Prayer | 8 | Keep, off, three protection prayers, three explicit protection-flick actions. |

Target selection persists. An existing target can continue to produce attacks when the attack head chooses zero. Explicit movement without an explicit attack clears that target; an actual shot can consume the movement opportunity. This distinction matters when analyzing logs: “attack action zero” is not, by itself, proof that the agent stopped attacking.

Flicking is an explicit primitive that performs the engine's off/on transition within one action. The agent learns when to choose it; it does not learn the underlying mouse-click sequence. There are no policy heads for eating, drinking, equipment changes or spellbook clicks in this task.

The legality mask is both part of the observation and an input to action sampling. Illegal choices are excluded before sampling. That reduces invalid-action noise, but it does not make the remaining actions useful: standing still without a target can be perfectly legal.

> **FIGURE 4 — Human cues and the policy interface.** Place a side-by-side human-view/structured-view diagram here. Match enemy appearance to NPC type, health bars to HP, attack cues to pending style/deadline, and movement to the 17-way head. Continue the structured side through the 320-input blocks, three 512-wide MinGRU layers, 17/9/8 action heads and value output. Draw a separate mask-to-sampler arrow and a recurrent-state loop. Mark exact timing features as engine-provided and the nine zeroed aggregate incoming-hit channels as ablated. Suggested export: `writeup-assets/policy-interface.svg`.

### Feedback and the learner

A human can treat surviving an attack, clearing a wave and preserving supplies as signs of progress toward the final goal. We turned analogous measurements into numerical feedback. That is an engineering choice about what to reinforce, not a reconstruction of human motivation.

The learner uses PPO-style clipped policy and value objectives, an entropy term, replay and V-trace corrections. A three-layer, 512-wide MinGRU supplies recurrent state and has 2,541,056 parameters. Its actor produces the action distributions; its critic estimates return. Updates use estimated advantages to increase or decrease the probability of sampled actions relative to the critic's expectation.

The current setup uses 4,096 agents, two buffers, 16 environment threads, a 256-step rollout horizon and 32,768-step minibatches. Each full rollout contains 1,048,576 agent steps. The requested 500M budget therefore finished after 476 rollouts, at 499,122,176 steps. Training is synchronous and resets recurrent state at each horizon; final evaluation carries it across horizons. Muon performs the parameter updates. Exact selected settings appear below.

## Training behavior: first surviving, then finishing

There are two timelines to distinguish: learning within one run and the months of changes that made good runs possible. The current run provides a useful view of the first; earlier failed experiments explain the second.

### Early and middle training

The current native log retains four training-bin averages, followed by a separate final evaluation. The step values below are the bins' mean locations, not four independently evaluated checkpoints.

| Mean training-step location | Mean wave | Mean episode ticks | Reached Jad | Completed cave |
|---:|---:|---:|---:|---:|
| 63.44M | 21.29 | 1,214.46 | 0% | 0% |
| 187.70M | 33.90 | 1,985.52 | 0% | 0% |
| 312.48M | 52.40 | 3,761.38 | 14.64% | 0.19% |
| 437.26M | 62.23 | 5,014.18 | 86.24% | 63.49% |

At first, the policy was learning to survive ordinary waves. In the third bin, it reached Jad in a meaningful fraction of episodes but almost never finished the cave. Later, both reach and completion increased sharply. Getting access to the final encounter and learning to finish it were distinct stages.

These measurements support that progression, but they do not tell us which targeting or healer tactic caused it. A higher mean wave is evidence of deeper survival; it is not evidence that the policy discovered a particular safespot. The behavior claims that go beyond these aggregates need matching checkpoint replays.

> **FIGURE 5 — Current learning progression.** Insert aligned panels for mean wave, episode length, and Jad reach/completion. Use the four training bins above, with their averaging made visible. Alternatively, parse the denser rounded console history from `confirm_0024_seed73.log`, excluding final-evaluation displays and labeling it as the exact seed-73 confirmation. Plot final evaluation as separate markers at 499.12M: 89.25% reach and 88.61% completion, n=10,800. Do not draw a dense validation curve from five native log entries. Suggested export: `writeup-assets/current-learning.svg`.

### What late-stage competence means

For the final checkpoint, the mean episode length was 4,930.82 ticks across all outcomes—about 49.31 minutes of simulated game time at 0.6 seconds per tick. That is neither training wall time nor a success-only completion duration.

Its high conversion after reaching Jad tells us the final encounter was usually finished under the arrival states this policy produced. It does not imply perfect protection timing or establish that it followed a human healer strategy. Those are finer behavioral questions.

> **VIDEO 2 — Decisions through training.** Insert matched early, middle and final-checkpoint clips here, identifying each checkpoint and episode. For ordinary waves, show target persistence, actual attacks, movement and protection. For the final policy, show a Jad/healer sequence with healer distraction, healing events and Jad HP. Use the same source episode as Video 1 when possible. Add a 6–12-tick trace linking the committed attack, visible deadline, chosen protection, prayer lock and resolved hit. Establish from the trace whether the policy tags, kills or outdamages healers before describing that tactic in the text. Suggested files: `writeup-assets/training-stages.mp4` and `writeup-assets/jad-decision-trace.svg`.

### The first runs did not look like this

The first recorded 500M run, `xgsb170g`, reached a mean wave of 27.6 and earned a mean return of 554.3, but recorded no completions in 10,174 episodes. A later tick-cap experiment, `ss966rf9`, ran for 2B steps with a 200,000-tick episode limit. Mean episode length reached 199,939 ticks while mean wave stayed near 30, again with no completions in 4,239 episodes.

Extremely long survival had become a failure mode. The summaries alone do not establish whether that early policy was hiding, kiting or exploiting a movement error. They establish the important mismatch: allowing an episode to continue almost indefinitely did not turn survival into progress. These runs also differed in settings and training budget; the comparison is a diagnostic history, not an isolated test of the cap.

> **FIGURE 6 — Survival without completion.** Insert three paired panels here for `xgsb170g` and `ss966rf9`: mean wave 27.6/30.0, episode ticks 3,110/199,939, and completions 0/0 with denominators 10,174/4,239. Label the 500M/2B budgets and changed cap. Use summary bars or dots, not interpolated learning curves. Suggested export: `writeup-assets/survival-without-completion.svg`.

## Reward hacking and how to spot issues

The useful question was whether rewarded behavior advanced the cave. An agent can accumulate reward from a repeatable local interaction while making little progress toward completion. It can also stall for reasons that have nothing to do with earning extra reward. Separating those cases required looking at reward components, state changes and action outcomes together.

### Rewarding protection more than progress

One clear example came from `cfuyizo1`. We restored correct-prayer rewards alongside net-progress feedback. Survival improved relative to the preceding experiment, but the final per-episode reward decomposition exposed another problem:

| Final metric | Recorded value |
|---|---:|
| Correct-danger-prayer reward | 86.87 |
| Progress reward | 2.92 |
| Total post-clipping return | 63.97 |
| Time without a target | About 83.1% of the episode |
| Time without progress | About 73.8% of the episode |
| Cave completions | 0 / 10,043 |

The direct prayer component was about 30 times the progress component before their contributions were combined and clipped. The policy was being paid mainly for local defensive behavior while still failing every cave. This is much stronger evidence of a misaligned incentive than simply observing many prayer switches.

Much later, a controlled removal of the remaining correct-danger-prayer reward improved final completion from **88.02% to 92.74%** at the same 750M budget and training seed (`i215ulj4` → `txqsiahp`). The weight changed from 0.005 to zero; other shaping remained. That supports removing this incentive in that setup, not a general conclusion that dense rewards are harmful.

> **FIGURE 7 — Reward composition versus objective progress.** Place two panels here. A: `cfuyizo1` prayer/progress reward bars, 86.87 versus 2.92; separate annotations for no-target/no-progress time and zero completion. Label the reward components as pre-total/clipping components. B: final completion for `i215ulj4` and `txqsiahp`, 88.0188% and 92.7431%, with n=10,216/10,018 and the 0.005→0 change. Suggested export: `writeup-assets/prayer-reward-diagnosis.svg`.

### Legal actions can still produce a healing loop or a stall

Hard action masks removed another source of confusion. In `l2l7lf6b`, all 2,382 logged history rows reported zero invalid movement, attack and prayer actions. The mask path was working, but the policy still did not finish a cave.

During the reported 1.75–2.0B-step window, enemies healed an average of 156,930 internal HP units per episode against 178,834 units of gross damage. Healing erased the equivalent of **87.75% of gross damage**. Later, over the final 100M steps before the terminal flush, attack-none occupied 98.81% of ticks and no-target occupied 98.59%; mean wave fell to 3.32.

The first measurement describes damage repeatedly undone by healing; the later measurement describes disengagement. Neither should be summarized as “the mask failed.” Nor does high healing alone prove a profitable exploit: the relationship between healing, net progress and reward must be checked under that run's actual reward function.

We tested increasing the negative-progress multiplier from 1.0 to 1.1. At the same 2,499,805,184 steps and seed 73, final mean wave improved from **3.7458 to 25.2685**, and mean cave progress from **0.050324 to 0.395250**. Both runs still recorded zero Jad kills. It was a partial repair, and the current setup uses multiplier 1.0.

> **FIGURE 8 — Healing and disengagement.** Insert same-window damage/healing bars, labeled in internal tenths of HP or converted consistently to game HP. Add separately labeled late-window attack-none/no-target bars; these overlap, so do not stack them. Below, show the multiplier comparison's final mean wave/progress and zero wins. The windows and final evaluations are different measurements. Optional video: recover an authenticated historical loop/stall replay with HP, targets and reward components. Suggested export: `writeup-assets/healing-and-stalling.svg`.

### Using policy failures to debug the simulator

A policy that repeatedly encounters an odd interaction is a useful source of test cases. It is not proof of the bug's cause. The diagnostic sequence we needed was: locate the behavior in metrics, inspect the state/action trace, check the implemented rule, and reproduce the issue in a deterministic fixture.

Two mechanics corrections show why this matters. Natural HP regeneration had been implemented as one HP per 10 ticks; it was corrected to one HP per 100 ticks. A subsequent run reached 82.51% completion, but its budget also increased from 750M to 1.5B, so the difference from the earlier high score cannot be attributed only to regeneration.

A cleaner comparison corrected the missing +60 Magic attack bonus on Ket-Zek and Jad's Magic attacks. With the 750M recipe and seed held fixed, completion changed from **92.74% to 91.49%**. The correction made the recorded score slightly worse while making the intended combat model more accurate. That is useful progress for an environment project.

Prayer timing was similarly important. In the current implementation, a non-melee Jad attack launched at tick T locks protection at T+2, before the T+2 action, with impact delayed at least three ticks. The post-tell action at T+1 can affect the lock; a later visual impact does not reopen the decision. An analysis aligned only to hitsplats would misclassify the relevant action window.

> **FIGURE 9 — A mechanic that changes the decision.** Place a current-code tick diagram here: launch/tell at T, actionable response at T+1, lock at boundary T+2, later impact. Use separate rows for observation, selected prayer, locked prayer and pending hit. Add a small adjacent table for the HP-regeneration and Magic-accuracy corrections, including their different controls. Suggested export: `writeup-assets/prayer-window-and-corrections.svg`.

A stall or training collapse is therefore a diagnostic signal, not a test that uniquely identifies rewards as too strict or too loose. The cause could be the objective, missing information, action semantics, incorrect mechanics, numerical instability or the update settings. Reward decomposition, deterministic tests and controlled training comparisons help distinguish them.

## Training balance: useful feedback over an increasingly long episode

### From damage to remaining work

Removing direct damage rewards did not automatically solve the objective problem. In `eifkgdut`, we removed per-hit damage feedback, emphasized kill/progress events and added a healing penalty. Final mean wave fell from about 28.05 in the preceding observation experiment to 21.68. The policy spent about 83.64% of its episode without a target. The experiment retained other feedback, so it was not a pure sparse-reward ablation; it showed that this particular replacement did not sustain useful combat behavior.

The retained design instead rewards **net required work removed**. Before the last wave, required work includes living NPC HP and the two future small Tz-Kek inside each unsplit parent. On wave 63, it counts Jad's HP. Healers need not die for completion, but healing Jad restores required work.

Let `W` be remaining required work in internal HP units, `S` the work at the start of the wave, and `q` its normalized progress:

```text
q = clamp(1 - W / S, 0, 1)
C = ((wave - 1) + q) / 63
progress reward = 0.001 × (C_now - C_previous) × 63 × S
```

Completion forces `C` to one, and wave-transition handling prevents a fresh wave's spawn from counting as negative progress. In the normal unclamped range, dealing damage reduces work and healing restores it. This makes progress dense without paying repeatedly for the same restored HP as if each hit moved the cave closer to completion.

For example, removing 10 game HP—100 internal units—produces +0.1 progress reward. Restoring that HP produces −0.1 progress reward, plus the separate −0.005 penalty for one effective heal event. These are code-derived component values before final reward clipping, not a measured historical trace.

> **FIGURE 10 — Required-work accounting.** Insert the 10-HP damage/heal example here. Show remaining work falling and rising, with +0.1/−0.1 and the separate −0.005 heal-event term. Include a small parent→two-children Tz-Kek diagram and a note that Jad's healers are not mandatory terminal work. Suggested export: `writeup-assets/required-work.svg`.

### The current reward budget

The current policy still uses shaping. The important distinction is what the terms measure and how their accumulated scale compares with useful progress.

| Term | Active rule |
|---|---|
| Net progress | 0.001 × net required work removed, with the progress accounting above. |
| Cave completion / player death | +1 / −1. |
| Damage taken | −0.25 × fraction of maximum HP lost that tick. |
| Prayer lost | −0.02 per game Prayer point. |
| Time | −0.0001 every tick. |
| Effective NPC healing | −0.005 per positive healing event, separate from restored-work cost. |
| Sustained lack of positive progress | After strictly more than 800/1,600/2,400 ticks, add −0.001/−0.005/−0.02; the tiers accumulate. |
| No attack | After the ready-without-attack counter exceeds 50, −0.005 × [1 + 0.05 × (wave−1)]. |
| Invalid action | −0.1 if an invalid choice occurs; native masks normally prevent it. |

Gross-damage, NPC-kill, wave-clear, separate Jad-kill and correct-prayer reward weights are zero. The environment publishes the sum; the current trainer clamps rollout rewards to [−1, +1] before learning.

The inactivity definitions matter. An ordinary cooldown tick is not the same as a prolonged stall. The no-attack counter increments when enemies remain and the attack timer is ready, and resets on an actual attack attempt. The no-progress tiers are cumulative: the active costs become −0.001, −0.006 and −0.026 per tick. Their purpose is to distinguish a brief period between useful events from sustained inactivity.

### Longer episodes change the learning problem

As the policy progressed, its mean training episode grew from roughly 1,214 to 5,014 ticks. At a fixed interaction budget, longer episodes mean fewer fresh starts and terminal outcomes. Later-wave experience becomes available only through successful earlier behavior, and its frequency changes as the policy improves.

We addressed this with dense work-based feedback, parallel environments, recurrent rollouts and explicit resource/inactivity costs. We did not normalize every episode to the same reward total or automatically reduce penalties as it grew longer. Consequently, reward accounting had to include accumulated costs: the current tick penalty contributes about −0.12 over 1,200 ticks and −0.50 over 5,000 ticks, before other terms and per-tick clipping.

Discounting also makes a terminal-only objective difficult. With the selected gamma of approximately 0.999126, a reward 5,000 ticks away has a direct discount factor of about 0.0126. The critic can bootstrap intermediate value estimates, but that does not create successful late-game experience before the policy reaches those states. Dense progress supplies nearer feedback while completion remains the actual evaluation target.

### Being selective about observations

We kept observations structured and bounded instead of adding every available diagnostic. NPC slots are normalized and zero-padded; type, timing, healing and progress have explicit meanings. The model receives 320 inputs, while the larger core diagnostic layout contains 475 values. The 20 raw reward-feature channels are not simply appended to the policy input.

The selected configuration zeros nine aggregate incoming-hit channels while retaining per-NPC pending style/timing and prayer deadlines. This removes one redundant representation, not all attack-timing information. NPC distance and validity remain enabled.

These are documented interface choices. We have not established that this is a minimal sufficient observation set, or isolated the contribution of each feature. Claims that noise was reduced should identify what was removed; claims that removal improved learning need matched experiments. The same applies to recurrence: it is part of the successful model, but its necessity remains an ablation question.

## Critical hyperparameters: what the experiments actually establish

### A large improvement under the same task

The clearest tuning result came from the July 140-trial search. The baseline and selected run used identical environment, reward, observation, vector and model settings at 749,731,840 actual steps. Final completion changed from **0.2089% to 95.5813%**, and Jad reach from **43.50% to 97.86%**.

Several trainer settings changed together:

| Setting | Baseline `8rg9wurg` | Selected `mmyxbyn4` |
|---|---:|---:|
| Learning rate | 0.000939323 | 0.002075675 |
| Entropy coefficient | 0.00644464 | 0.000625461 |
| Gamma / GAE lambda | 0.999518847 / 0.9995 | 0.999126114 / 0.9 |
| Minibatch size | 4,096 | 32,768 |
| Policy clipping | 0.132360 | 0.05 |
| Replay ratio | 1.333590 | 2.055184 |
| V-trace rho / c limits | 0.5 / 0.503727 | 2.0 / 0.974667 |
| Replay priority alpha / beta0 | 0.968236 / 0 | 0.911074 / 0.225813 |

This establishes a large effect from the training recipe under a fixed task. It does not identify the individual effect of each row. Learning rate controls update scale; entropy weights the incentive to retain action diversity; batch size, clipping, replay and correction settings affect how the collected experience changes the policy. Their interaction matters, especially after changing which actions are legal or which behaviors earn reward.

The search's successful region favored lower entropy, GAE 0.9, horizon 256 and larger minibatches. Those are useful empirical directions for subsequent search. Boundary values and correlations in adaptive trials are not substitutes for one-variable ablations.

The July simulator still had the faster HP regeneration discussed earlier. Its 95.58% remains a turning point in the training story, not a directly comparable score for the current task.

> **FIGURE 11 — The effect of searching the recipe.** Insert separate July and current-campaign panels here: trial index versus final completion percentage. Label each campaign's task/budget, baseline and selected run. July: 0.2089%→95.5813%. Current: 72.4608%→88.6111%. Recover July campaign membership before plotting all 140 trials. Current trial `0008` has nonfinite weights: mark it in an invalid-trial strip, not as a valid score. Suggested export: `writeup-assets/hyperparameter-searches.svg`.

### Selecting stable learning, not just a high endpoint

A later 130-trial search considered architecture, agent count and value/optimizer settings. All eight highest-final performers used three recurrent layers; seven used hidden size 512 and six used 4,096 agents. That concentration helped select the model used in later work.

The highest final score did not tell the whole story:

| August run | Final evaluation | Final-quarter training mean | Final-quarter training q10 | First recorded ≥90% |
|---|---:|---:|---:|---:|
| Selected `1nvvx5qu` | 94.79% | 94.98% | 93.20% | 316.7M steps |
| `pozhjer2` | 96.41% | 56.80% | 9.04% | 692.1M steps |

The selected run acquired high performance earlier and sustained it through the final quarter. The other was a successful late learner; its low final-quarter statistics do not establish an end-of-run collapse. The q10 is a quantile of training measurements, not a confidence interval for final evaluation. The full comparison is recorded in [the August analysis](sweep_top8.md).

> **FIGURE 12 — Learning stability versus endpoint.** Insert grouped training mean/q10 markers with separate final-evaluation dots for these two runs. Annotate first-90% steps and agent counts, 4,096 versus 8,192. Use dense learning curves only after retrieving the original histories. Suggested export: `writeup-assets/stability-versus-endpoint.svg`.

### The current recipe and what remains uncertain

The latest fixed-500M search varied eight parameters while holding the task, architecture, agent count, horizon, minibatch, gamma/GAE, V-trace settings and training seed fixed. It produced 90 attempts, 89 finite checkpoints and one numerically unstable checkpoint. The selected result improved on the prior 72.46% baseline to 88.61%.

| Parameter | Current selected value |
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

Learning-rate annealing is disabled. An earlier same-seed 500M comparison also motivated synchronous training with horizon resets: final completion was 0% for async/reset-off, 0.0391% for async/reset-on, 63.53% for synchronous/reset-off, and 72.4608% for synchronous/reset-on. That result applies to the tested recipe; it is not a general verdict on asynchronous training.

We cannot honestly rank every hyperparameter from “largest” to “smallest” effect. The August finalists spanned a broad range of value-loss coefficients, which suggests several viable combinations rather than proving the coefficient unimportant. Parameters held fixed in the latest search were not tested for low impact. A useful next step is to ablate promising settings across multiple seeds while holding the rest of the task constant.

## How consistently does the trained agent finish?

The current final evaluation reloaded the selected checkpoint, started full episodes at wave 1, used masked categorical sampling, and carried recurrent state across horizons. The requested minimum was 10,000 completed episodes; batched evaluation finished with 10,800.

| Outcome | Episodes | Share |
|---|---:|---:|
| Failed before reaching Jad | Approximately 1,161 | 10.75% |
| Reached Jad but failed | Approximately 69 | 0.6389% |
| Completed the cave | 9,570 | 88.6111% |

The recorded reach rate is 89.25%. Multiplying by 10,800 and rounding gives approximately 9,639 Jad reaches; 9,570/9,639 gives the 99.28% conditional conversion. The reach/failure counts are reconstructed from aggregate floating-point rates, not a retained per-episode outcome table.

Most failed episodes in this evaluation ended before Jad. That is useful for deciding where to investigate this checkpoint, but does not establish that every training seed has the same bottleneck.

> **FIGURE 13 — Final outcome decomposition.** Insert a 100% stacked bar here: 1,161 pre-Jad failures, 69 Jad failures, 9,570 completions. Label reconstructed counts, total n=10,800 and the conditional denominator 9,639. Put the small Jad-failure label outside its segment. Do not add a per-wave or per-rotation breakdown without episode records. Suggested export: `writeup-assets/final-outcomes.svg`.

### Repeating episodes is different from repeating training

We trained three finalist recipes with seeds 73, 101, 202 and 303, producing twelve completed confirmation runs:

| Recipe | Seed 73 | Seed 101 | Seed 202 | Seed 303 | Equal-weight mean |
|---|---:|---:|---:|---:|---:|
| 0024 | 88.61% | 38.51% | 74.56% | 81.07% | 70.69% |
| 0084 | 83.75% | 87.65% | 88.44% | 7.44% | 66.82% |
| 0028 | 80.23% | 44.29% | 79.24% | 3.91% | 51.92% |

All twelve were finite. Seed 73 reproduced the original selected checkpoint for each recipe; it was also the search's selection seed. It therefore supplies reproduction evidence, while the three new seeds better test training reliability. For recipe 0024, their mean completion was 64.72%.

Recipe 0084 with seed 303 reached Jad in 81.19% of episodes but completed only 7.44%: about 9.16% conversion after arrival. Its recorded training dashboards never showed a strong completion policy. This was failure to acquire reliable Jad behavior, not evidence of a strong agent suddenly collapsing at the end.

The project therefore produced weights that finish the cave repeatedly, but not a recipe that always learns equally strong weights. The next diagnosis needs to compare arrival resources and encounter traces across strong and weak seeds. Aggregate rates cannot decide whether the difference lies in protection timing, targeting, healing or positioning.

> **FIGURE 14 — Training-seed reliability.** Insert paired Jad-reach/completion markers for all twelve runs, grouped by recipe and labeled by seed. Mark seed 73 as used for selection; highlight the 0084/303 gap. Read final `env/jad_kill_rate`, `env/reached_wave_63` and `env/n` from each confirmation INI. Show all seeds rather than pooling their episodes into one success rate. Suggested export: `writeup-assets/training-seeds.svg`.

## Future RuneScape encounters

The most transferable part of this project is the way the environment is built and measured: a shared deterministic simulation, explicit observations/actions, reproducible resets, inspectable reward components and identified policy replays. Reusing those tools would save work on another encounter. Reusing the exact weights, 320-feature schema or selected training settings is a separate hypothesis.

The Inferno is a natural direction to consider because it also combines waves with a final boss, while introducing different encounter rules. Jagex's [revised Inferno design post](https://forums.rs/en/380%2C381%2C256%2C65733712.html) described a separate challenge with new enemy types. My expectation is that a similar simulation/training architecture would be useful, but its objectives, hazards and state representation would need their own validation.

Other extensions probe different limitations:

| Possible extension | What could carry over | What would need to change or be tested |
|---|---|---|
| More demanding wave encounters, including the Inferno | Tick simulation, collision/LOS, pending attacks, batched training and wave analytics. | Encounter-specific mechanics, observation capacity, protected objectives and reward accounting. |
| Multiple simultaneous Jad-style threats | Attack-style/deadline representation and protection actions. | Whether timing, target selection and available protection produce a feasible strategy under overlapping demands. |
| A single boss requiring equipment or attack-style changes | Combat formulas, viewer, replay and event diagnostics. | Additional action heads, inventory/equipment observations and the costs/delays of switching. |
| Multi-stage or cooperative encounters | Simulation/testing infrastructure and explicit stage metrics. | Persistent resources, new objectives, teammate/opponent state and possibly a multi-agent learner. |

These are proposed applications, not transfer results. The exact reward scale is especially unlikely to be portable unchanged: enemy HP, fight length, healing and resource demands determine what a given progress or time penalty means. Our own fixed-task sweeps already showed why the learner settings should be treated as a starting point for another encounter.

A useful first transfer experiment would keep the framework fixed, implement one bounded new encounter, and compare a fresh policy against any transferred initialization under the same declared budget. That would separate reuse of engineering from reuse of learned behavior.

> **OPTIONAL FIGURE 15 — What transfers.** Place a compact diagram here: reusable simulation/training/diagnostic components on the left, encounter-specific mechanics/observations/actions/rewards on the right. Label every proposed encounter as future work. Avoid a capability chart implying these encounters have already been trained. Suggested export: `writeup-assets/encounter-transfer.svg`.

## Conclusion

Building Fight Caves exposed several different ways for apparent progress to diverge from the intended task. More survival time could mean a stall. More gross damage could be undone by healing. More prayer reward could mean less incentive to finish the wave. A high final checkpoint could conceal late learning, and a strong selection seed could conceal unreliable training.

The useful response was to make those differences measurable. Shared simulation and replay made mechanics inspectable. Required-work feedback distinguished damage from lasting progress. Controlled comparisons separated some reward and mechanics changes from trainer effects. Large searches found settings that produced successful behavior, while seed confirmations showed the limits of that recipe.

The resulting policy completes full caves repeatedly under a documented setup. The next work is to preserve that competence across fresh training seeds and harder conditions: compare strong and weak Jad traces, evaluate declared episode/rotation sets, and test how much success depends on exact timing observations, recurrence and the fixed loadout. Those experiments would turn a strong checkpoint into a more dependable training result.

---

## Sources, experiment identity and remaining media

This article describes the project evidence reviewed through **September 14, 2026**. Present-tense mechanics and learner details use the latest native simulation snapshot; older runs retain their historical task conditions. Repository defaults and older baseline pages may describe different experiments.

### Code and historical records

- [Current simulation and policy interface](https://github.com/jordanbailey00/PufferLib/blob/9ad3633d3563a8e467026160e90209146d2a733c/ocean/fight_caves/simulation.h), [adapter](https://github.com/jordanbailey00/PufferLib/blob/9ad3633d3563a8e467026160e90209146d2a733c/ocean/fight_caves/binding.c), [network/optimizer](https://github.com/jordanbailey00/PufferLib/blob/9ad3633d3563a8e467026160e90209146d2a733c/src/algo.cu), and [trainer](https://github.com/jordanbailey00/PufferLib/blob/9ad3633d3563a8e467026160e90209146d2a733c/src/pufferl.cu). The selected trial settings in this article supersede that commit's original training defaults.
- [Project run history](runescape-rl/docs/run_history.md), [mechanics and cleanup history](runescape-rl/docs/archive/fc_cleanup_and_parity_history.md), [August sweep analysis](sweep_top8.md), and [historical sweep configurations](runescape-rl/config/experiments).
- [Current selected W&B run: `f146cdcf307ed881`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/f146cdcf307ed881), native trial `sweep_1789246135940_0024`.
- Historical reward cases: [`cfuyizo1`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/cfuyizo1), [`l2l7lf6b`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/l2l7lf6b), [`ruuq4231`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/ruuq4231), and [`eifkgdut`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/eifkgdut).
- Matched comparisons: July [`8rg9wurg`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/8rg9wurg)/[`mmyxbyn4`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/mmyxbyn4); prayer removal [`i215ulj4`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/i215ulj4)/[`txqsiahp`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/txqsiahp); Magic correction [`r16nvaq7`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/r16nvaq7)/[`399wemq3`](https://wandb.ai/jbailey8531-oakton-college/fight%20caves%20rl/runs/399wemq3).

The twelve seed confirmations and several detailed historical analyses are retained locally; they are not all published as GitHub assets or W&B runs. Their measured values are included above, and the source locations below identify what to retrieve when producing figures. All image/video blocks are production placeholders, not claims that a recording has already been captured.

<details>
<summary>Selected checkpoint and local source locations for producing the figures</summary>

The paths below are relative to the local `fight-cave-rl_clones` workspace, not this repository or GitHub. They are retrieval notes, not downloadable links.

| Evidence | Local source |
|---|---|
| Current log/config | `fc5-protein-90x500m/runs/fc5_protein_90x500m_async0_reset1_r2/logs/fight_caves/sweep_1789246135940_0024.ini` |
| Current weights | Same campaign, `checkpoints/fight_caves/sweep_1789246135940_0024/0000000499122176.bin` |
| Source/assets/executable provenance | Same campaign, `manifest.json`; `fc-rl-5.0/baseline.md` |
| Current search outcomes | Same campaign, `trial_status.jsonl` and all trial INIs |
| Seed confirmations | `fc5-seed-confirmation-12x500m/logs/fight_caves/confirm_<trial>_seed<seed>.ini` |
| Denser rounded current training display | `fc5-seed-confirmation-12x500m/console/confirm_0024_seed73.log` |
| Seed analysis | `TEMP_FC5_FOCUSED_SWEEP.md` |
| Historical raw metrics | `v38/pufferlib_4/logs/fight_caves/<run_id>.json` |
| Reward redesign and diagnosis | `v38/runescape-rl/docs/archive/fc_revamp.md` |
| July campaign report | `v38/sweep_history/v3_simple_reward_sweep.md` |
| Early engine history | `v38/runescape-rl/docs/archive/history.md` |
| Current maps | `fc-rl-5.0/resources/fight_caves/runtime/fightcaves.{collision,movement,los}` |

Selected weight SHA-256:

```text
963d9949a3c1d954fb2c746860a226c0c34915f1f19b1b2f4ffb15e56a97d568
```

Recorded training executable SHA-256:

```text
474850506a21b5022e2ed4b598e997ef1fc0c895945e37e81d2ee0514d2eb64d
```

For native INIs, read comma-separated arrays in `[metrics]`. The first four entries of the selected run are training-bin averages; the last is final evaluation. For historical JSON logs, config sections and `metrics` are at the root. Check the logging window before interpreting a point as an instantaneous measurement.

Each new video should retain checkpoint identity, compatible source/executable/assets, episode seed/rotation, action-sampling mode, initial recurrent state, source ticks, terminal outcome and playback speed. CPU and GPU replay need not sample identical trajectories. Use the correct replay path for any claimed reproduction of the selected policy's behavior.

Publish per-episode records before adding wave/rotation heatmaps, duration distributions or resource-at-Jad distributions. The current aggregate metrics do not contain those distributions.

</details>
