"""Build the GameCube controller feature for one region from src/gc_*.s."""
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', 'tools'))
import gen_common as g
from layout import GC_BASE, GC_END
from ops import Feature, Patch

USA_DOL = None


def build(region, dol):
    sites = g.GC_SITES[region]
    consts = dict(g.WM, **g.CC, **g.PAD)
    ops, cur = [], GC_BASE

    # 1. KPAD sampling callback hook: convert GC pad to Classic Controller sample
    sample_site = sites['sample']
    orig_sample = struct.unpack('>I', dol.read(sample_site, 4))[0]
    h, size = g.hook(sample_site, orig_sample, cur, g.read('gc_sample.s'), {}, consts,
                     'KPAD sampling callback: GameCube pad -> Classic Controller sample')
    ops.append(h)
    cur += size

    # 2. Remote-less synthesizer hook: generate sample when ring is empty
    ring_site = sites['ring']
    orig_ring = struct.unpack('>I', dol.read(ring_site, 4))[0]
    h, size = g.hook(ring_site, orig_ring, cur, g.read('gc_synth.s'), {'WPD_TABLE': sites['wpd'], 'CTRL_OBJ': sites['ctrl']}, consts,
                     'KPAD read: synthesise Classic Controller sample from GameCube pad')
    ops.append(h)
    cur += size

    # 3. WPADProbe hook: report Classic Controller connected when GC pad answers
    probe_site = sites['probe']
    orig_probe = struct.unpack('>I', dol.read(probe_site, 4))[0]
    h, size = g.hook(probe_site, orig_probe, cur, g.read('gc_probe.s'),
                     {'PROBE_BODY': probe_site + 4}, consts,
                     'WPADProbe: GameCube pad counts as connected Classic Controller')
    ops.append(h)
    cur += size

    # 4. Radio check bypass in KPADRead: allow reading GC pad without paired remote
    radio_site = sites['radio']
    orig_radio = dol.read(radio_site, 4)
    ops.append(Patch(radio_site, bytes.fromhex('4800000C'), orig_radio,
                     'KPADRead: bypass Bluetooth radio check for GameCube controller'))

    if cur > GC_END:
        raise SystemExit(f'gc code overflows its window: 0x{cur:X} > 0x{GC_END:X}')

    return Feature('gc', 'GameCube controller', region, ops)
