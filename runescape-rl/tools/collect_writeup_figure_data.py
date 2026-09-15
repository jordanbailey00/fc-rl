#!/usr/bin/env python3
"""Extract immutable evidence for the writeup; no training or network access.

Usage: python collect_writeup_figure_data.py --workspace /path/to/fight-cave-rl_clones
The output contains only plotted data, source paths and SHA-256 checksums.
"""
import argparse
import configparser
import hashlib
import json
import math
import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
CAMPAIGN = 'fc5-protein-90x500m/runs/fc5_protein_90x500m_async0_reset1_r2'
SELECTED = 'sweep_1789246135940_0024'


def finite(value):
    result = float(value)
    if not math.isfinite(result):
        raise ValueError(f'Expected finite metric, got {value!r}')
    return result


def collect(workspace):
    sources = {}

    def read(relative):
        path = workspace / relative
        blob = path.read_bytes()
        sources[relative] = {'sha256': hashlib.sha256(blob).hexdigest()}
        return blob

    def ini(relative):
        cfg = configparser.ConfigParser()
        cfg.read_string(read(relative).decode())
        metrics = {k: [float(v) for v in s.split(',')]
                   for k, s in cfg['metrics'].items()}
        return cfg, metrics

    def old(run, keys=('agent_steps', 'env/n', 'env/jad_kill_rate',
                       'env/reached_wave_63', 'env/wave_reached',
                       'env/episode_length')):
        source = f'v38/pufferlib_4/logs/fight_caves/{run}.json'
        raw = json.loads(read(source))
        return {'run': run, 'source': source, 'kind': 'native_final_evaluation',
                'metrics': {k: finite(raw['metrics'][k][-1]) for k in keys}}

    code_path = 'fc-rl-5.0/ocean/fight_caves/simulation.h'
    code = read(code_path).decode()
    def constant(name):
        match = re.search(r'^#define\s+' + name + r'\s+(\d+)\b', code, re.M)
        if not match:
            raise ValueError(f'Cannot read numeric constant {name}')
        return int(match[1])

    map_data = {}
    for suffix in ('collision', 'movement', 'los'):
        source = f'fc-rl-5.0/resources/fight_caves/runtime/fightcaves.{suffix}'
        cells = list(read(source))
        if len(cells) != 64 * 64:
            raise ValueError(f'Map has wrong shape: {source}')
        if suffix == 'collision' and set(cells) != {0, 1}:
            raise ValueError('Expected binary walkability map')
        map_data[suffix] = [cells[y*64:(y+1)*64] for y in range(64)]
    first = {}
    for wave, entries in re.findall(r'/\* Wave\s+(\d+) \*/\s*\{\s*\{([^}]+)', code):
        for npc in re.findall(r'NPC_\w+', entries):
            first.setdefault(npc, int(wave))
    flags = {k: 1 << int(v) for k, v in re.findall(
        r'#define\s+(FC_(?:LOS_\w+|MOVE_WALL_\w+))\s+\(1u << (\d+)\)', code)}
    map_data.update({'source': code_path, 'first_waves': first, 'flags': flags,
                     'orientation': 'row-major [y][x]; x east, y north; 0 blocked, 1 walkable'})

    cfg, metrics = ini(f'{CAMPAIGN}/logs/fight_caves/{SELECTED}.ini')
    keys = ('agent_steps', 'env/n', 'env/wave_reached', 'env/episode_length',
            'env/reached_wave_63', 'env/jad_kill_rate')
    lengths = {len(metrics[k]) for k in keys}
    if lengths != {5}:
        raise ValueError(f'Expected four training bins plus evaluation: {lengths}')
    current = {'run': SELECTED, 'source': f'{CAMPAIGN}/logs/fight_caves/{SELECTED}.ini',
               'training': [{k: finite(metrics[k][i]) for k in keys} for i in range(4)],
               'evaluation': {k: finite(metrics[k][-1]) for k in keys}}
    baseline_path = 'fc5-500m-ablation-20260912-a1AIvE/logs/fight_caves/sync_async0_reset1.ini'
    _, baseline_metrics = ini(baseline_path)
    baseline = {'source': baseline_path,
                'evaluation': {k: finite(baseline_metrics[k][-1]) for k in keys}}
    policy = {name: constant(name) for name in (
        'FC_OBS_PLAYER_SIZE','FC_OBS_NPC_STRIDE','FC_OBS_NPC_SLOTS',
        'FC_OBS_META_SIZE','FC_MOVE_DIM','FC_ATTACK_DIM','FC_PRAYER_DIM')}
    policy.update({'hidden_size': int(cfg['policy']['hidden_size']),
                   'num_layers': int(cfg['policy']['num_layers'])})
    reward = {key: float(cfg['env'][key]) for key in
              ('w_progress','shape_npc_heal_penalty')}

    statuses = [json.loads(line) for line in read(f'{CAMPAIGN}/trial_status.jsonl').decode().splitlines()]
    if {r['trial'] for r in statuses} != set(range(90)):
        raise ValueError('Current campaign membership is incomplete')
    sweep = []
    for status in sorted(statuses, key=lambda row: row['trial']):
        trial = status['trial']
        matches = list((workspace/CAMPAIGN/'logs/fight_caves').glob(f'sweep_*_{trial:04}.ini'))
        if len(matches) != 1:
            raise ValueError(f'Expected one log for trial {trial}: {matches}')
        relative = str(matches[0].relative_to(workspace))
        row = dict(status, source=relative)
        if status['checkpoint_valid']:
            _, m = ini(relative)
            row.update({'completion': finite(m['env/jad_kill_rate'][-1])*100,
                        'reach': finite(m['env/reached_wave_63'][-1])*100,
                        'n': int(m['env/n'][-1]), 'steps': int(m['agent_steps'][-1])})
        else:
            read(relative)
            row['completion'] = None
        sweep.append(row)

    july = []
    manifest = '20260727T013946Z-sweep-644900.json'
    for path in sorted((workspace/'v38/pufferlib_4/logs/fight_caves').glob('*.json')):
        raw = json.loads(path.read_bytes())
        if Path(raw.get('run', {}).get('manifest_path', '')).name == manifest:
            july.append(old(path.stem))
    if len(july) != 140:
        raise ValueError(f'Expected 140 manifest-matched July trials; found {len(july)}')
    read('v38/pufferlib_4/logs/fight_caves/manifests/'+manifest)

    confirmations = []
    for recipe in ('0024','0084','0028'):
        for seed in (73,101,202,303):
            path = f'fc5-seed-confirmation-12x500m/logs/fight_caves/confirm_{recipe}_seed{seed}.ini'
            _, m = ini(path)
            confirmations.append({'recipe': recipe, 'seed': seed, 'source': path,
                'n': int(m['env/n'][-1]), 'reach': finite(m['env/reached_wave_63'][-1])*100,
                'completion': finite(m['env/jad_kill_rate'][-1])*100})

    prayer_keys = ('env/rwd_correct_danger_prayer_total','env/rwd_progress_total',
                   'env/no_target_ticks','env/no_progress_ticks','env/episode_length',
                   'env/jad_kill_rate','env/n')
    historical = {run: old(run) for run in ('l2l7lf6b','ruuq4231','mzqf7iml',
                 '7mxnrzua','i215ulj4','txqsiahp','8rg9wurg','mmyxbyn4','il0xq0uf',
                 'ov5qfn36','ymj1j1mi','1nvvx5qu','pozhjer2')}
    historical['cfuyizo1'] = old('cfuyizo1', prayer_keys)
    for run in ('l2l7lf6b', 'ruuq4231'):
        historical[run] = old(run, tuple(historical[run]['metrics']) + ('env/cave_progress',))

    # These are explicitly transcribed report summaries, not reconstructed histories.
    run_report = 'v38/runescape-rl/docs/run_history.md'
    read(run_report)
    early = [dict(run='xgsb170g', wave=27.6, ticks=3110, n=10174, wins=0, budget_m=500, cap=30000),
             dict(run='ss966rf9', wave=30.0, ticks=199939, n=4239, wins=0, budget_m=2000, cap=200000)]
    revamp_path = 'v38/runescape-rl/docs/archive/fc_revamp.md'
    revamp = read(revamp_path).decode()
    for literal in ('156,930','178,834','98.81%','98.59%'):
        if literal not in revamp:
            raise ValueError(f'Historical summary no longer contains {literal}')
    healing = {'source': revamp_path, 'kind': 'transcribed_report_windows',
               'window_steps': [1750000000,2000000000], 'damage_tenths':178834,
               'healing_tenths':156930, 'late_attack_none_pct':98.81,
               'late_no_target_pct':98.59, 'late_window':'Last 100M steps before terminal flush'}
    august_path = 'v38/sweep_top8.md'
    august = read(august_path).decode().split('## Performance and learning-curve data')[1]
    stability = []
    for run in ('1nvvx5qu','pozhjer2'):
        line = next(line for line in august.splitlines() if line.startswith(f'| `{run}` |'))
        cells = [s.strip().replace('*','').replace('%','').replace('M','').replace(',','')
                 for s in line.split('|')[1:-1]]
        stability.append({'run': run, 'source': august_path, 'kind':'report_training_summary',
                          'first90_m':float(cells[3]), 'late_mean':float(cells[5]),
                          'late_q10':float(cells[6]), 'n':int(cells[7]),
                          'final':historical[run]['metrics']['env/jad_kill_rate']*100,
                          'agents':4096 if run=='1nvvx5qu' else 8192})
    for extra in ('v38/runescape-rl/docs/archive/history.md',
                  'v38/runescape-rl/docs/archive/SOTA_DIFF_AUDIT.md',
                  'v38/runescape-rl/docs/sweep_history.md',
                  'v38/runescape-rl/docs/archive/fc_cleanup_and_parity_history.md'):
        read(extra)
    result = {'schema_version':1, 'reviewed_through':'2026-09-14',
        'sources':sources, 'maps':map_data, 'policy':policy, 'reward':reward,
        'current':current, 'current_baseline':baseline,
        'current_sweep':sweep, 'july_sweep':july,
        'confirmations':confirmations, 'historical':historical,
        'early':{'source':run_report,'kind':'transcribed_report_summary','runs':early},
        'healing':healing, 'stability':stability,
        'may_sweep':{'completion_pct':88.6,'kind':'transcribed_report_summary',
                     'source':'v38/runescape-rl/docs/sweep_history.md','run':'a3mi6u2g'},
        'notes':['July plots sort by final completion; launch order is not recovered.',
                 'Current learning uses four native training-bin averages, then final evaluation.',
                 'Mechanics illustrations are schematics, not observed policy trajectories.',
                 'Final outcome counts are rounded from aggregate rates.']}
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--workspace',type=Path,required=True)
    p.add_argument('--output',type=Path,default=REPO/'writeup-assets/figure-data.json')
    args=p.parse_args()
    result=collect(args.workspace.resolve())
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2,sort_keys=True,allow_nan=False)+'\n')
    print(f'Wrote {args.output}: {len(result["sources"])} identified sources, '
          '140 July trials, 90 current attempts, 12 confirmations.')


if __name__ == '__main__':
    main()
