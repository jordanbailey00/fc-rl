#!/usr/bin/env python3
"""Build the article's static figures from the committed evidence snapshot.

Requires matplotlib and numpy. No workspace logs, network or training required.
Usage: python runescape-rl/tools/build_writeup_figures.py
"""
import argparse
import hashlib
import json
import os
from pathlib import Path

os.environ.setdefault('MPLCONFIGDIR', '/tmp/fc-writeup-matplotlib')
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap
from matplotlib.patches import FancyBboxPatch, Rectangle, Patch
from matplotlib.lines import Line2D
import numpy as np

REPO = Path(__file__).resolve().parents[2]
INK='#182c3c'; MUTED='#546777'; BLUE='#376eb4'; TEAL='#087f72'
RED='#be4c51'; GOLD='#bd811d'; PURPLE='#805aa4'; GREY='#a4b0b9'
BG='#fcfcfa'; GRID='#dfe5e6'; WALK='#72bf9e'; BLOCK='#dc7776'
plt.rcParams.update({'font.family':'DejaVu Sans','font.size':11,
    'text.color':INK,'axes.labelcolor':INK,'xtick.color':MUTED,'ytick.color':MUTED,
    'axes.edgecolor':GRID,'axes.spines.top':False,'axes.spines.right':False,
    'axes.facecolor':BG,'figure.facecolor':BG,'savefig.facecolor':BG,
    'svg.fonttype':'none','svg.hashsalt':'fight-caves-writeup-v1',
    'axes.titlesize':12,'axes.titleweight':'bold','axes.labelsize':11})
OUTPUTS=[]


def canvas(title, subtitle='', size=(11.5,6.3)):
    fig=plt.figure(figsize=size)
    fig.text(.045,.96,title,fontsize=19,weight='bold',va='top')
    if subtitle: fig.text(.045,.906,subtitle,fontsize=10.5,color=MUTED,va='top')
    return fig


def finish(fig, name, note, out):
    fig.text(.045,.036,note,fontsize=9,color=MUTED,va='bottom',linespacing=1.45)
    fig.savefig(out/(name+'.svg'),metadata={'Date':None,'Creator':'Fight Caves writeup figure builder'})
    fig.savefig(out/(name+'.png'),dpi=175,metadata={'Software':'Fight Caves writeup figure builder'})
    OUTPUTS.append({'name':name,'title':fig.texts[0].get_text(),'note':note})
    plt.close(fig)


def grid(ax, percent=False, maxval=None):
    ax.set_axisbelow(True); ax.grid(axis='y',color=GRID,linewidth=.75)
    ax.spines['left'].set_visible(False)
    if percent:
        ax.set_ylim(0,105); ax.set_yticks([0,25,50,75,100]); ax.set_ylabel('Episodes (%)')
    elif maxval is not None: ax.set_ylim(0,maxval)
    ax.tick_params(axis='both',length=0,pad=7)


def barlabels(ax, bars, fmt='{:.2f}', offset=3):
    ax.bar_label(bars,labels=[fmt.format(b.get_height()) for b in bars],padding=offset,fontsize=11,weight='bold')


def drawing(fig, rect=(.045,.13,.91,.71)):
    ax=fig.add_axes(rect); ax.set_xlim(0,100);ax.set_ylim(0,100);ax.axis('off');return ax


def box(ax,x,y,w,h,title,body='',color=BLUE,fontsize=11):
    ax.add_patch(FancyBboxPatch((x,y),w,h,boxstyle='round,pad=.5,rounding_size=1.4',
        facecolor=color+'12',edgecolor=color,linewidth=1.2,clip_on=False))
    if body:
        ax.text(x+w/2,y+h-4.1,title,ha='center',va='top',weight='bold',fontsize=fontsize,color=color)
        ax.text(x+w/2,y+h/2-3,body,ha='center',va='center',fontsize=fontsize-1,linespacing=1.5)
    else: ax.text(x+w/2,y+h/2,title,ha='center',va='center',fontsize=fontsize,weight='bold',color=color)


def arrow(ax,start,end,color=MUTED,style='-|>',connection='arc3,rad=0'):
    ax.annotate('',xy=end,xytext=start,arrowprops=dict(arrowstyle=style,color=color,lw=1.6,connectionstyle=connection))


def mapgrid(ax,cells,title):
    ax.imshow(cells,origin='lower',interpolation='nearest',cmap=ListedColormap([BLOCK,WALK]),vmin=0,vmax=1)
    ax.set_title(title,pad=12);ax.set_xlabel('Tile x (east →)');ax.set_ylabel('Tile y (north →)')
    ax.set_xticks([0,8,16,24,32,40,48,56,63]);ax.set_yticks([0,8,16,24,32,40,48,56,63])
    ax.set_xticks(np.arange(-.5,64,1),minor=True);ax.set_yticks(np.arange(-.5,64,1),minor=True)
    ax.grid(which='minor',color=INK,alpha=.18,lw=.35)
    ax.tick_params(which='minor',length=0);ax.tick_params(which='major',length=0,labelsize=9)
    ax.set_xlim(-.5,63.5);ax.set_ylim(-.5,63.5)


def maps(d,out):
    m=d['maps']; cells=np.array(m['collision']); los=np.array(m['los']); flags=m['flags']
    blocked=int((cells==0).sum());walk=int((cells==1).sum())
    fig=canvas('Fight Caves collision grid',f'64 × 64 tiles  •  {blocked:,} blocked  •  {walk:,} walkable',size=(10,10))
    ax=fig.add_axes([.10,.19,.79,.64]);mapgrid(ax,cells,'')
    fig.legend(handles=[Patch(color=BLOCK,label='Blocked tile (0)'),Patch(color=WALK,label='Walkable tile (1)')],
               loc='lower center',bbox_to_anchor=(.5,.105),ncol=2,frameon=False)
    finish(fig,'collision-map','Source: current fightcaves.collision, row-major [y][x].\nTile walkability only; directional movement walls and line-of-sight flags are separate.',out)

    fig=canvas('The arena, its boundaries and the waves',
        'Current runtime maps  •  64 × 64 tiles  •  15 spawn rotations',size=(12.5,10.3))
    a=fig.add_axes([.065,.365,.39,.44]);b=fig.add_axes([.56,.365,.39,.44])
    mapgrid(a,cells,'Tile collision: red blocked / green walkable')
    los_clear=(los & flags['FC_LOS_FULL'])==0
    mapgrid(b,los_clear,'Line of sight: full blockers + wall edges')
    for y,x in np.ndindex(los.shape):
        value=los[y,x]
        for bit,start,end in [('FC_LOS_NORTH',(x-.5,y+.5),(x+.5,y+.5)),
                              ('FC_LOS_EAST',(x+.5,y-.5),(x+.5,y+.5)),
                              ('FC_LOS_SOUTH',(x-.5,y-.5),(x+.5,y-.5)),
                              ('FC_LOS_WEST',(x-.5,y-.5),(x-.5,y+.5))]:
            if value & flags[bit]: b.plot([start[0],end[0]],[start[1],end[1]],color=INK,lw=.8)
    footprint=fig.add_axes([.11,.25,.80,.075]);footprint.axis('off');footprint.set_xlim(0,60);footprint.set_ylim(0,6);footprint.set_aspect('equal')
    for x,size,name in [(0,1,'Player'),(19,3,'Tok-Xil'),(41,5,'Ket-Zek / Jad')]:
        footprint.add_patch(Rectangle((x,0),size,size,facecolor=BLUE+'22',edgecolor=BLUE,lw=1.2))
        footprint.text(x+size+1,2.1,f'{name}\n{size} × {size} tiles',va='center',fontsize=10)
    wave=fig.add_axes([.045,.12,.91,.09]);wave.axis('off');wave.set_xlim(0,6);wave.set_ylim(0,1)
    names=[('NPC_TZ_KIH','Tz-Kih'),('NPC_TZ_KEK','Tz-Kek'),('NPC_TOK_XIL','Tok-Xil'),
           ('NPC_YT_MEJKOT','Yt-MejKot'),('NPC_KET_ZEK','Ket-Zek'),('NPC_TZTOK_JAD','Jad')]
    for i,(key,name) in enumerate(names):
        wave.add_patch(Rectangle((i+.04,.1),.92,.75,facecolor=(TEAL if i<5 else PURPLE)+'12',edgecolor=GRID))
        wave.text(i+.5,.66,f'Wave {m["first_waves"][key]}',ha='center',weight='bold',fontsize=11)
        wave.text(i+.5,.30,name,ha='center',fontsize=10)
    finish(fig,'arena-and-waves','Sources: current maps and WAVE_TABLE. LOS green = no full-tile projectile block; dark edges block directions.\nFootprint swatches and first-appearance cards are illustrative; card spacing does not represent wave duration.',out)


def architecture(d,out):
    fig=canvas('One simulation, several ways to use it','Offline assets feed a shared runtime; training and viewing use the same game rules.',size=(12,7.6))
    ax=drawing(fig)
    box(ax,1,72,23,22,'Offline cache exports','Terrain • objects\nCollision • visual assets',GOLD)
    box(ax,37,72,27,22,'Shared C simulation','State + seeded tick\nMovement • combat • waves',TEAL)
    arrow(ax,(24,83),(37,83));ax.text(30,88,'Arena data',ha='center',fontsize=9)
    box(ax,75,72,23,22,'Policy / training','Observations • actions\nRewards • episode resets',BLUE)
    arrow(ax,(64,85),(75,85));arrow(ax,(75,79),(64,79))
    box(ax,37,28,27,23,'Viewer','Rendering • controls\nHP • targets • LOS overlays',PURPLE)
    box(ax,75,28,23,23,'Replay / checks','Actions • state hashes\nDeterministic fixtures',BLUE)
    arrow(ax,(50,72),(50,51));arrow(ax,(62,72),(81,51))
    arrow(ax,(13,72),(37,40),color=GOLD);ax.text(13,48,'Visual assets',ha='center',color=GOLD,fontsize=10)
    ax.text(50,7,'Render when inspecting; step without rendering when training.',ha='center',weight='bold',fontsize=12)
    finish(fig,'engine','Architecture schematic from current simulation/adapter and retained engine history.\nThe archival viewer screenshot shown alongside this figure is an existing project asset.',out)


def policy(d,out):
    p=d['policy']; heads=[p['FC_MOVE_DIM'],p['FC_ATTACK_DIM'],p['FC_PRAYER_DIM']]
    sizes=[p['FC_OBS_PLAYER_SIZE'],p['FC_OBS_NPC_STRIDE']*p['FC_OBS_NPC_SLOTS'],p['FC_OBS_META_SIZE'],sum(heads)]
    assert sizes==[23,248,15,34] and sum(sizes)==320
    fig=canvas('From game cues to policy actions','Structured engine features replace visual inference and mouse input.',size=(12,8.5))
    ax=drawing(fig)
    for y,title,body in [(78,'Player state','Health bars • prayer • position'),(56,'Nearby enemies','Appearance • health • attack cues'),(34,'Encounter state','Wave • progress • remaining work')]:
        box(ax,0,y,26,17,title,body,BLUE,10.5)
    for y,n,title,body in [(78,sizes[0],'Player','HP, Prayer, timers, target'),(56,sizes[1],'8 NPC slots','Type, HP, LOS, style, deadlines'),(34,sizes[2],'Wave / progress','Remaining work, rotation'),(12,sizes[3],'Action mask','One legality bit per choice')]:
        box(ax,33,y,29,17,f'{n} inputs  ·  {title}',body,TEAL,10.5)
        if y>12: arrow(ax,(26,y+8),(33,y+8))
    box(ax,72,47,27,29,'Recurrent MinGRU',f'{sum(sizes)} inputs\n{p["num_layers"]} layers × {p["hidden_size"]} units',PURPLE,12)
    for y in (86,64,42,20):
        ax.plot([62,67],[y,y],color=MUTED,lw=1.3)
    ax.plot([67,67],[20,86],color=MUTED,lw=1.3)
    arrow(ax,(67,64),(72,64))
    arrow(ax,(89,76),(98,76),PURPLE,connection='arc3,rad=-1.8');ax.text(85,86,'Carried state',ha='center',fontsize=10,color=PURPLE)
    box(ax,72,13,27,23,'Action heads + sampling','Move 17  /  Attack 9  /  Prayer 8\nLegality mask applied here',BLUE,10.5)
    arrow(ax,(85,47),(85,36));arrow(ax,(62,20),(72,20),TEAL)
    ax.text(85,4,'Separate critic: value estimate',ha='center',fontsize=9,color=PURPLE)
    ax.text(16,12,'Human cues',ha='center',weight='bold',fontsize=11)
    finish(fig,'policy-interface','Exact timing/healing features are engine-provided. NPC slots reorder by distance.\nNine aggregate incoming-hit channels are zeroed; per-NPC timing remains. Masks also constrain sampling.',out)

    fig=canvas("Jad's protection deadline comes before impact",'Current non-melee attack timing  •  schematic, not a recorded policy decision',size=(12,6))
    ax=drawing(fig)
    stages=[(1,'T','Attack launches','Style committed\nTell becomes available',BLUE),
            (27,'T + 1','Response opportunity','This action can change\nthe prayer used for the hit',TEAL),
            (53,'Boundary T + 2','Protection locks','Before the T+2 action\nLater prayer changes are too late',RED),
            (79,'T + 3 or later','Impact','Resolve using\nthe locked protection',PURPLE)]
    for x,t,title,body,col in stages:
        ax.text(x+10,84,t,ha='center',fontsize=15,weight='bold',color=col)
        box(ax,x,29,20,40,title,body,col,10.3)
        if x<79: arrow(ax,(x+20,49),(x+26,49))
    ax.text(50,9,'Projectile flight does not extend the response window.',ha='center',fontsize=12,weight='bold')
    finish(fig,'prayer-window','Source: current launch, pending-prayer-lock and tick-order code.\nOrdinary NPC launch snapshots and immediate melee use different timing rules.',out)


def early(d,out):
    runs=d['early']['runs'];labels=['First run\n500M steps','Higher tick cap\n2B steps']
    fig=canvas('Surviving longer did not finish the cave','Two historical experiments with different budgets and episode caps',size=(11.5,6.2))
    axs=fig.subplots(1,3);fig.subplots_adjust(left=.065,right=.97,bottom=.25,top=.80,wspace=.42)
    for ax,key,title,limit in zip(axs,['wave','ticks','wins'],['Mean wave reached','Mean episode ticks','Completed episodes'],[65,235000,1]):
        bars=ax.bar([0,1],[r[key] for r in runs],color=[BLUE,GOLD],width=.55);grid(ax,maxval=limit);ax.set_title(title)
        ax.set_xticks([0,1],labels)
        if key=='ticks':barlabels(ax,bars,'{:,.0f}');ax.ticklabel_format(axis='y',style='plain')
        elif key=='wave':barlabels(ax,bars,'{:.1f}')
        else:
            ax.set_yticks([0,1]);ax.text(0,.14,'0 / 10,174',ha='center',weight='bold');ax.text(1,.14,'0 / 4,239',ha='center',weight='bold')
    finish(fig,'survival-without-completion','Source: retained run reports, xgsb170g / ss966rf9. Episode limits: 30,000 / 200,000 ticks.\nSummary measurements; this comparison does not isolate the tick-cap change.',out)


def history(d,out):
    fig=canvas('The task changed as the project developed','Historical milestones carry different gear, supplies, mechanics and budgets.',size=(12,8))
    ax=drawing(fig)
    cards=[('March','Build and inspect','Kotlin prototype → C simulator\nShared viewer / training rules',BLUE),
           ('April','Survival without completion','Mean episode nearly 200,000 ticks\nMean wave about 30; zero wins',RED),
           ('May','Strong policies; new diagnostic',f'{d["may_sweep"]["completion_pct"]:.1f}% reported with full supplies\nBegin no-food / no-potion tests',TEAL),
           ('June','Define the actual experiment',f'Healer / map / compiled-loadout audit\nRebuilt SOTA loadout: {100*d["historical"]["ov5qfn36"]["metrics"]["env/jad_kill_rate"]:.2f}% wins',GOLD),
           ('July–August','Rewards, mechanics and updates','Required-work feedback; trainer searches\nCompare stability as well as final score',PURPLE),
           ('Current','No-supply checkpoint',f'{100*d["current"]["evaluation"]["env/jad_kill_rate"]:.2f}% over {int(d["current"]["evaluation"]["env/n"]):,} full caves\nFresh training seeds still vary',TEAL)]
    for i,(when,title,body,color) in enumerate(cards):
        x=(i%3)*34;y=55 if i<3 else 7
        ax.text(x+15,y+38,when,ha='center',weight='bold',fontsize=12,color=color)
        box(ax,x,y,30,32,title,body,color,10.3)
    finish(fig,'project-turning-points','Sources: engine, run, sweep and reproduction-audit histories. Dates are milestone labels, not a time scale.\nMay 88.6% is a report summary. June used full supplies; current evaluation uses zero supplies.',out)


def learning(d,out):
    cur=d['current'];train=cur['training'];end=cur['evaluation']
    x=np.array([r['agent_steps'] for r in train])/1e6
    fig=canvas('Learning to reach Jad, then finish','Four native training-bin averages; separate final evaluation at 499.12M steps',size=(12,6.8))
    axs=fig.subplots(1,3);fig.subplots_adjust(left=.065,right=.98,bottom=.25,top=.79,wspace=.38)
    for ax,key,title,color in zip(axs[:2],['env/wave_reached','env/episode_length'],['Mean wave','Mean episode ticks'],[BLUE,PURPLE]):
        ax.plot(x,[r[key] for r in train],marker='o',color=color,lw=1.5,ls='--',label='Training-bin mean')
        ax.scatter(end['agent_steps']/1e6,end[key],marker='D',s=65,color=color,edgecolor=INK,zorder=5,label='Final evaluation')
        grid(ax);ax.set_title(title);ax.set_ylim(0,65 if 'wave' in key else 6000)
    ax=axs[2]
    for key,label,color in [('env/reached_wave_63','Reached Jad',BLUE),('env/jad_kill_rate','Completed cave',TEAL)]:
        ax.plot(x,[r[key]*100 for r in train],marker='o',ls='--',color=color,label=label)
        ax.scatter(end['agent_steps']/1e6,end[key]*100,marker='D',s=70,color=color,edgecolor=INK,zorder=5)
    grid(ax,percent=True);ax.set_title('Reach and completion');ax.legend(loc='upper left',frameon=False,fontsize=9)
    for ax in axs:ax.set_xlim(0,535);ax.set_xticks([0,250,500]);ax.set_xlabel('Agent steps (millions)')
    fig.legend(handles=[Line2D([],[],marker='o',ls='--',color=MUTED,label='Training-bin mean'),
        Line2D([],[],marker='D',ls='',color=MUTED,label='Final evaluation')],ncol=2,loc='lower center',bbox_to_anchor=(.5,.13),frameon=False)
    finish(fig,'current-learning','Source: selected 0024 native log. Bin x positions are average step locations; dashed lines guide the eye.\nFinal evaluation: n=10,800, 89.25% reached Jad, 88.61% completed. No dense checkpoint curve is inferred.',out)


def prayer(d,out):
    m=d['historical']['cfuyizo1']['metrics']
    fig=canvas('Rewarded protection could crowd out progress','Historical reward diagnosis and a later matched removal experiment',size=(12,6.8))
    a=fig.add_axes([.07,.28,.36,.50]);b=fig.add_axes([.58,.28,.36,.50])
    vals=[m['env/rwd_correct_danger_prayer_total'],m['env/rwd_progress_total']]
    bars=a.bar([0,1],vals,color=[PURPLE,TEAL],width=.55);barlabels(a,bars);grid(a,maxval=105)
    a.set_xticks([0,1],['Correct-prayer\nreward','Progress\nreward']);a.set_ylabel('Mean reward component / episode');a.set_title('cfuyizo1: reward composition')
    no_target=100*m['env/no_target_ticks']/m['env/episode_length'];no_progress=100*m['env/no_progress_ticks']/m['env/episode_length']
    fig.text(.08,.16,f'{no_target:.1f}% of ticks without a target\n{no_progress:.1f}% without progress; 0 / {int(m["env/n"]):,} wins',fontsize=10)
    vals=[100*d['historical'][r]['metrics']['env/jad_kill_rate'] for r in ('i215ulj4','txqsiahp')]
    bars=b.bar([0,1],vals,color=[PURPLE,TEAL],width=.55);barlabels(b,bars,'{:.2f}%');grid(b,percent=True)
    b.set_xticks([0,1],['Prayer weight\n0.005','Prayer weight\n0']);b.set_title('Later matched reward removal')
    fig.text(.59,.16,'Same 750M budget and training seed\nn = 10,216 / 10,018',fontsize=10)
    finish(fig,'prayer-reward-diagnosis','Sources: cfuyizo1, i215ulj4, txqsiahp native final metrics. Reward components are before total clipping.\nThe two panels describe different experiments; other shaping remained active in the removal comparison.',out)


def healing(d,out):
    h=d['healing'];fig=canvas('Healing erased damage; later, the policy disengaged','Different windows answer different questions.',size=(12,9))
    a=fig.add_axes([.075,.54,.35,.27]);b=fig.add_axes([.59,.54,.35,.27])
    bars=a.bar([0,1],[h['damage_tenths']/10,h['healing_tenths']/10],color=[BLUE,RED],width=.55)
    barlabels(a,bars,'{:,.0f}');grid(a,maxval=21500);a.set_title('1.75–2.0B-step window');a.set_ylabel('Mean game HP / episode');a.set_xticks([0,1],['Gross damage','NPC healing'])
    b.barh([1,0],[h['late_attack_none_pct'],h['late_no_target_pct']],color=[PURPLE,GOLD],height=.48)
    b.set_xlim(0,112);b.set_yticks([1,0],['Attack-none','No target']);b.set_xlabel('Ticks (%)');b.set_title('Final 100M before terminal flush')
    for y,v in [(1,h['late_attack_none_pct']),(0,h['late_no_target_pct'])]:b.text(v+1,y,f'{v:.2f}',va='center',fontsize=10)
    a=fig.add_axes([.075,.19,.35,.19]);b=fig.add_axes([.59,.19,.35,.19])
    for ax,key,title,limit in [(a,'env/wave_reached','Final mean wave',32),(b,'env/cave_progress','Final cave progress',.50)]:
        vals=[d['historical'][r]['metrics'][key] for r in ('l2l7lf6b','ruuq4231')]
        bars=ax.bar([0,1],vals,color=[RED,TEAL],width=.55);barlabels(ax,bars,'{:.4f}' if 'progress' in key else '{:.2f}')
        grid(ax,maxval=limit);ax.set_xticks([0,1],['Multiplier 1.0','Multiplier 1.1']);ax.set_title(title)
    finish(fig,'healing-and-stalling','Top: transcribed l2l7lf6b report windows; healing = 87.75% of gross damage. Inactivity measures overlap.\nBottom: native final evaluations, same 2.4998B steps / seed 73. Both have zero wins; current multiplier is 1.0.',out)


def movement(d,out):
    fig=canvas('A movement correction let training get farther','Range alone is not enough: line of sight and valid contact also matter.',size=(12,7.4))
    for rect,after in [([.045,.38,.27,.40],False),([.355,.38,.27,.40],True)]:
        ax=fig.add_axes(rect);ax.set_xlim(-.5,7.5);ax.set_ylim(-.5,7.5);ax.set_aspect('equal')
        ax.set_xticks(np.arange(-.5,8,1));ax.set_yticks(np.arange(-.5,8,1));ax.grid(color=GRID);ax.set_xticklabels([]);ax.set_yticklabels([]);ax.tick_params(length=0)
        ax.add_patch(Rectangle((.5,.5),3,3,facecolor=TEAL+'33',edgecolor=TEAL,lw=2));ax.text(2,2,'N',ha='center',va='center',weight='bold')
        ax.add_patch(Rectangle((4.5,4.5),1,1,facecolor=BLUE+'33',edgecolor=BLUE,lw=2));ax.text(5,5,'P',ha='center',va='center',weight='bold')
        ax.plot([3.5,3.5],[.5,5.5],color=INK,lw=5);ax.plot([3,5],[3,5],ls='--',color=RED,lw=1.2)
        ax.set_title('Corrected: keep seeking a valid tile' if after else 'Failure mode: stop while blocked',fontsize=10)
        if after:
            ax.add_patch(Rectangle((.5,1.5),3,3,facecolor='none',edgecolor=TEAL,lw=1.4,ls='--'))
            ax.annotate('',xy=(2,3.8),xytext=(2,2.5),arrowprops=dict(arrowstyle='-|>',color=TEAL,lw=2))
            ax.text(3.5,6.65,'Continue pursuit',ha='center',color=TEAL,fontsize=10)
        else:ax.text(3.5,6.65,'No valid attack',ha='center',color=RED,fontsize=10)
    ax=fig.add_axes([.73,.35,.23,.43]);vals=[d['historical'][r]['metrics']['env/wave_reached'] for r in ('mzqf7iml','7mxnrzua')]
    bars=ax.bar([0,1],vals,color=[RED,TEAL],width=.55);barlabels(ax,bars);grid(ax,maxval=65);ax.set_xticks([0,1],['Before','After']);ax.set_ylabel('Final mean wave')
    fig.text(.07,.235,'N = 3×3 NPC   P = player   Dark edge = wall\nLayout illustrates the rule; it is not a recovered replay or fixture.',fontsize=10,color=MUTED)
    fig.text(.735,.235,'0 wins in both evaluations\nn = 10,070 / 10,021',fontsize=10)
    finish(fig,'movement-correction','Source: documented July movement/positioning fix and mzqf7iml / 7mxnrzua final metrics.\n1.499B steps each; reward, learner, vector and policy settings match. Multiple movement rules changed.',out)


def work(d,out):
    reward=d['reward'];progress=100*reward['w_progress'];heal=reward['shape_npc_heal_penalty']
    fig=canvas('Reward the work that stays done','A 10-HP damage / healing cycle, using the current component weights',size=(12,8))
    ax=drawing(fig)
    box(ax,0,60,26,29,'Before damage','Required work: W\n10 game HP = 100 internal units',BLUE,10.5)
    box(ax,37,60,26,29,'After 10 HP damage',f'Required work: W − 100\nProgress reward: +{progress:.1f}',TEAL,10.5)
    box(ax,74,60,26,29,'After 10 HP healing',f'Required work: W\nProgress: −{progress:.1f}  /  heal event: {heal:.3f}',RED,10.5)
    arrow(ax,(26,75),(37,75));arrow(ax,(63,75),(74,75))
    ax.text(50,45,f'Cycle total: {heal:.3f} before other reward terms and trainer clipping',ha='center',weight='bold',fontsize=12)
    box(ax,0,3,45,28,'Tz-Kek: count the children in advance','Remaining parent HP + both future children\nSplitting does not create unexpected new work.',GOLD,11)
    box(ax,55,3,45,28,'Jad: count the boss HP','Healers need not die for completion.\nHealing Jad restores required work.',PURPLE,11)
    finish(fig,'required-work','Source: current reward configuration and required-work accounting. Code-derived illustration within the unclamped range.\nThe +0.1 / −0.1 components cancel; the separate effective-heal event costs −0.005.',out)


def searches(d,out):
    rows=sorted(d['july_sweep'],key=lambda r:(r['metrics']['env/jad_kill_rate'],r['run']))
    fig=canvas('July search: the distribution of all 140 results','Each dot is one final evaluation, sorted by completion rate.',size=(12,6.4))
    ax=fig.add_axes([.08,.24,.88,.55]);ys=[r['metrics']['env/jad_kill_rate']*100 for r in rows]
    ax.scatter(np.arange(1,141),ys,s=27,color=BLUE,alpha=.7,edgecolors='none')
    baseline=d['historical']['8rg9wurg']['metrics']['env/jad_kill_rate']*100
    ax.axhline(baseline,color=RED,ls='--',lw=1.4,label=f'Baseline: {baseline:.4f}%')
    idx=next(i for i,r in enumerate(rows) if r['run']=='mmyxbyn4')+1
    ax.scatter(idx,ys[idx-1],s=140,marker='*',color=TEAL,edgecolors=INK,zorder=5)
    ax.annotate(f'Selected mmyxbyn4\n{ys[idx-1]:.4f}%',xy=(idx,ys[idx-1]),xytext=(87,69),
        fontsize=10,color=TEAL,arrowprops=dict(arrowstyle='-',color=TEAL),bbox=dict(facecolor=BG,edgecolor='none',pad=3))
    grid(ax,percent=True);ax.set_xlim(-2,145);ax.set_xlabel('Trial rank by final completion (not launch order)');ax.legend(loc='upper left',frameon=False)
    finish(fig,'july-search','Source: all 140 native logs matched to the July 27 sweep manifest. Requested budget 750M steps.\nHistorical task with faster HP regeneration. Selected by peak performance; its final result is plotted here.',out)

    fig=canvas('Current search: 90 attempts at the same task','89 finite checkpoints; one numerically unstable attempt.',size=(12,6.6))
    ax=fig.add_axes([.08,.32,.88,.47]);valid=[r for r in d['current_sweep'] if r['checkpoint_valid']]
    ax.scatter([r['trial'] for r in valid],[r['completion'] for r in valid],s=35,color=BLUE,alpha=.75,edgecolors='none')
    baseline=100*d['current_baseline']['evaluation']['env/jad_kill_rate']
    ax.axhline(baseline,color=GOLD,ls='--',label=f'Prior baseline: {baseline:.2f}%')
    selected=next(r for r in valid if r['trial']==24);ax.scatter(24,selected['completion'],s=170,marker='*',color=TEAL,edgecolors=INK,zorder=5)
    ax.annotate('0024: 88.61%',(24,selected['completion']),(33,94),color=TEAL,weight='bold',fontsize=10,arrowprops=dict(arrowstyle='-',color=TEAL))
    grid(ax,percent=True);ax.set_xlim(-2,91);ax.set_xticks([0,20,40,60,80]);ax.tick_params(labelbottom=False);ax.legend(loc='lower right',frameon=False)
    strip=fig.add_axes([.08,.20,.88,.06]);strip.set_xlim(-2,91);strip.set_ylim(0,1);strip.set_yticks([]);strip.set_xticks([0,20,40,60,80]);strip.set_xlabel('Trial number (launch order)')
    for row in d['current_sweep']:
        if not row['checkpoint_valid']:
            strip.scatter(row['trial'],.5,marker='x',color=RED,s=65);strip.text(row['trial']+2,.5,f'{row["trial"]:04}: invalid weights',va='center',fontsize=10,color=RED)
    finish(fig,'current-search','Sources: trial_status.jsonl and final native metrics; 499,122,176 actual steps for completed 500M attempts.\nTrial 0008 is excluded from valid scores. Task, architecture and selection seed 73 are fixed.',out)


def stability(d,out):
    rows=d['stability'];fig=canvas('Strong endpoints can hide different learning histories','August comparison: training-window statistics and final evaluation',size=(12,6.8))
    ax=fig.add_axes([.09,.32,.86,.46]);positions=np.arange(2)
    for offset,key,label,col,marker in [(-.20,'late_q10','Final-quarter training q10',GOLD,'v'),
       (0,'late_mean','Final-quarter training mean',BLUE,'o'),(.20,'final','Final evaluation',TEAL,'D')]:
        vals=[r[key] for r in rows];ax.scatter(positions+offset,vals,s=95,color=col,marker=marker,label=label,zorder=5)
        for x,v in zip(positions+offset,vals):ax.text(x,v+3,f'{v:.2f}%',ha='center',fontsize=10,color=col)
    grid(ax,percent=True);ax.set_ylim(0,115);ax.set_xlim(-.5,1.5);ax.set_xticks(positions,[f'{r["run"]}\n{r["agents"]:,} agents' for r in rows])
    for i,r in enumerate(rows):
        fig.text(.30 if i==0 else .73,.19,f'First ≥90%: {r["first90_m"]:.1f}M steps\nFinal evaluation n = {r["n"]:,}',ha='center',fontsize=10)
    fig.legend(*ax.get_legend_handles_labels(),loc='upper center',bbox_to_anchor=(.52,.867),ncol=3,frameon=False,fontsize=9)
    finish(fig,'stability-versus-endpoint','Sources: August top-eight report and native final metrics. q10 is a training quantile, not an evaluation confidence interval.\nThe later learner improved near the end; low quarter-wide statistics alone do not establish an end-of-run collapse.',out)


def outcomes(d,out):
    end=d['current']['evaluation'];n=round(end['env/n']);reach=round(n*end['env/reached_wave_63']);wins=round(n*end['env/jad_kill_rate'])
    counts=[n-reach,reach-wins,wins];colors=[GOLD,RED,TEAL]
    fig=canvas('Most failures of the selected policy happen before Jad',f'{n:,} full-cave evaluations  •  selected checkpoint 0024',size=(12,5.5))
    ax=fig.add_axes([.065,.48,.885,.21]);left=0
    for count,col in zip(counts,colors):ax.barh(0,100*count/n,left=left,color=col,height=.65);left+=100*count/n
    ax.set_xlim(0,100);ax.set_ylim(-.5,.5);ax.set_yticks([]);ax.set_xticks([0,25,50,75,100]);ax.set_xlabel('Episodes (%)');ax.spines['bottom'].set_visible(False)
    fig.text(.08,.34,f'Before Jad\n{counts[0]:,}  /  {100*counts[0]/n:.2f}%',color=GOLD,fontsize=12,weight='bold')
    fig.text(.38,.34,f'At Jad\n{counts[1]:,}  /  {100*counts[1]/n:.2f}%',color=RED,fontsize=12,weight='bold')
    fig.text(.68,.34,f'Completed\n{wins:,}  /  {100*wins/n:.2f}%',color=TEAL,fontsize=12,weight='bold')
    center=100*(counts[0]+counts[1]/2)/n
    ax.annotate('',xy=(center,-.30),xytext=(37,-1.25),annotation_clip=False,arrowprops=dict(arrowstyle='-',color=RED,lw=1))
    fig.text(.50,.17,f'After reaching Jad: {wins:,} / {reach:,} = {100*wins/reach:.2f}% completion',ha='center',fontsize=13,weight='bold')
    finish(fig,'final-outcomes','Source: selected native final evaluation. Arrival and failure counts are rounded from aggregate rates.\nConditional completion describes the states this policy reaches Jad in.',out)


def seeds(d,out):
    fig=canvas('A good checkpoint does not guarantee a good new training run','All twelve confirmations  •  paired reach and completion rates',size=(12,7))
    axs=fig.subplots(1,3);fig.subplots_adjust(left=.085,right=.975,bottom=.25,top=.79,wspace=.30)
    for ax,recipe in zip(axs,['0024','0084','0028']):
        rows=[r for r in d['confirmations'] if r['recipe']==recipe]
        for y,r in zip([3,2,1,0],rows):
            ax.plot([r['completion'],r['reach']],[y,y],color=GRID,lw=4,zorder=1)
            ax.scatter(r['reach'],y,marker='o',s=75,facecolors='none',edgecolors=BLUE,lw=1.8,zorder=3)
            ax.scatter(r['completion'],y,marker='D',s=55,color=TEAL,zorder=4)
            ax.text(r['completion'],y-.22,f'{r["completion"]:.2f}%',ha='center',va='top',fontsize=9,color=TEAL)
        ax.set_xlim(-5,105);ax.set_ylim(-.6,3.6);ax.set_xticks([0,50,100]);ax.set_yticks([3,2,1,0],['73*','101','202','303']);ax.set_xlabel('Episodes (%)');ax.set_title(f'Recipe {recipe}')
        ax.grid(axis='x',color=GRID);ax.spines['left'].set_visible(False);ax.tick_params(length=0)
        if recipe=='0024':ax.set_ylabel('Training seed')
    handles=[Line2D([],[],marker='o',ls='',markerfacecolor='none',color=BLUE,label='Reached Jad'),Line2D([],[],marker='D',ls='',color=TEAL,label='Completed cave')]
    fig.legend(handles=handles,ncol=2,loc='lower center',bbox_to_anchor=(.5,.13),frameon=False)
    finish(fig,'training-seeds','Source: twelve confirmation INIs, final evaluation; episode denominators are retained in figure-data.json.\n* Seed 73 was used to select the recipes. Highlighted gap: 0084 / seed 303 reaches Jad 81.19%, completes 7.44%.',out)


def transfer(d,out):
    fig=canvas('What could carry over to another encounter','Proposed applications only; no transfer result is claimed.',size=(12,7.4))
    ax=drawing(fig)
    box(ax,0,18,37,67,'Reusable infrastructure','Deterministic ticks and resets\nCollision / line of sight\nBatched training\nViewer and replay\nReward / episode diagnostics',TEAL,12)
    box(ax,63,18,37,67,'Encounter-specific work','Enemies and attack rules\nHazards and objectives\nObservation capacity\nActions and switching costs\nReward scale and resources',PURPLE,12)
    arrow(ax,(37,51),(63,51));ax.text(50,66,'New encounter',ha='center',fontsize=11,weight='bold')
    ax.text(50,1,'Test: train from scratch versus transferred weights under the same declared budget.',ha='center',fontsize=11)
    finish(fig,'encounter-transfer','Conceptual future-work diagram based on the article. Engineering reuse and policy transfer are separate measurements.',out)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--data',type=Path,default=REPO/'writeup-assets/figure-data.json')
    p.add_argument('--output-dir',type=Path,default=REPO/'writeup-assets')
    args=p.parse_args();data=json.loads(args.data.read_text())
    if data['schema_version']!=1:raise ValueError('Unsupported figure data schema')
    args.output_dir.mkdir(parents=True,exist_ok=True)
    for fn in (maps,architecture,policy,early,history,learning,prayer,healing,movement,work,searches,stability,outcomes,seeds,transfer):
        fn(data,args.output_dir)
    index={'data_sha256':hashlib.sha256(args.data.read_bytes()).hexdigest(),
           'matplotlib':matplotlib.__version__,'numpy':np.__version__,'figures':OUTPUTS}
    (args.output_dir/'figure-index.json').write_text(json.dumps(index,indent=2)+'\n')
    print(f'Built {len(OUTPUTS)} SVG figures and {len(OUTPUTS)} PNG previews in {args.output_dir}')


if __name__=='__main__':main()
