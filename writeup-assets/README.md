# Fight Caves writeup figures

The static figures are built from the project's retained data and documented mechanics. Each has an SVG for the article and a PNG preview. The [article](../WRITEUP.md) places them in context.

The [collision grid](collision-map.svg) uses the actual 64 × 64 runtime file: **red = blocked (0), green = walkable (1)**. It contains 1,872 blocked and 2,224 walkable tiles. The arena figure also shows the separate line-of-sight map; directional walls are not interchangeable with whole-tile collision.

## Figure inventory

| Article item | Status / artifact | Evidence and limits |
|---|---|---|
| Video 1: successful cave | User capture needed | Selected `0024` checkpoint; short clip, full episode and poster. |
| Figure 1: arena and waves | [Arena + LOS + progression](arena-and-waves.svg); [standalone red/green collision grid](collision-map.svg) | Actual runtime maps and wave table. Footprint swatches are illustrative. |
| Image 2: starting equipment | User screenshot needed | Current equipment, stats and empty food/potion inventory, before the first action. |
| Figure 3: engine and debugging | [Engine diagram](engine.svg); [existing archival viewer image](../runescape-rl/assets/readme/viewer-debug-los.png) | Architecture schematic; archival screenshot reused unchanged. A new annotated/current screenshot can replace it later. |
| Figure 4: observations/actions and prayer | [Policy interface](policy-interface.svg); [prayer deadline](prayer-window.svg) | Current code/configuration; timing illustration is not an observed policy trace. |
| Figure 5: early stalls | [Survival without completion](survival-without-completion.svg) | Transcribed historical run summaries; different budgets and caps. |
| Figure 6: project history, optional | [Turning points](project-turning-points.svg) | Historical milestone cards; conditions differ between scores. |
| Figure 7: current learning | [Learning progression](current-learning.svg) | Four native training-bin averages plus separate final evaluation. No dense curve inferred. |
| Video 2: behavior through training | User capture needed | Identified early/middle/final checkpoints. A measured decision-trace graphic can be built once the corresponding state/action log is available. |
| Figure 8: prayer incentives | [Reward diagnosis](prayer-reward-diagnosis.svg) | Native component metrics and matched removal evaluation. |
| Figure 9: healing and stalling | [Healing comparison](healing-and-stalling.svg) | Recorded historical windows and native final evaluations, shown separately. Optional historical loop video still needs capture. |
| Figure 10: mechanics correction | [Movement correction](movement-correction.svg) | Measured before/after results plus an explicitly illustrative geometry schematic. The exact historical fixture has not been recovered. |
| Figure 11: required-work reward | [Reward accounting](required-work.svg) | Worked example derived from current reward weights and documented rules. |
| Figure 12: parameter searches | [July search](july-search.svg); [current search](current-search.svg) | All 140 July members recovered by shared manifest; sorted by final score because launch order wasn't recovered. All 90 current attempts shown; invalid trial separated. |
| Figure 13: stability | [Stability comparison](stability-versus-endpoint.svg) | Training-window statistics from August report; final evaluation from native logs. q10 is not a confidence interval. |
| Figure 14: final outcomes | [Outcome decomposition](final-outcomes.svg) | Counts rounded from selected evaluation rates, n=10,800. |
| Figure 15: fresh training seeds | [All 12 confirmations](training-seeds.svg) | Final reach/completion and denominators from native INIs. Selection seed identified. |
| Figure 16: future encounters, optional | [What transfers](encounter-transfer.svg) | Conceptual future-work diagram, not a transfer experiment. |

## Rebuild and inspect the evidence

[figure-data.json](figure-data.json) contains all plotted values, the runtime maps, source paths and SHA-256 checksums for 268 original inputs. It distinguishes native evaluations from transcribed historical report summaries. Raw source files remain unchanged.

The builder runs offline using only the committed snapshot. From the repository root, in a Python environment with the [plotting dependencies](../runescape-rl/tools/requirements-writeup.txt):

```bash
python runescape-rl/tools/build_writeup_figures.py
```

This regenerates 18 SVGs, their 18 PNG previews and [figure-index.json](figure-index.json). The index records the data checksum and plotting-library versions. Each PNG has the same stem as its SVG.

To refresh the snapshot from the original local workspace:

```bash
python runescape-rl/tools/collect_writeup_figure_data.py --workspace /path/to/fight-cave-rl_clones
python runescape-rl/tools/build_writeup_figures.py
```

The collector requires the original sources and fails on missing required data or incomplete campaign membership. It never launches training or uses the network. Recollection can intentionally change the snapshot if source files changed; review the diff and source checksums before publishing new figures.

Rates are converted to percentages, internal HP tenths to game HP, and tick durations are kept separate from training wall time. Per-wave failure heatmaps, rotation results, resource-at-Jad distributions and observed tactics require episode-level records or matching state/action traces.
