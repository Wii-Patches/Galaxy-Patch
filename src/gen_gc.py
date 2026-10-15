"""Build the GameCube controller feature for one region from src/gcpad.c."""
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', 'tools'))
import asm
import gen_common as g
from layout import GC_BASE, GC_END
from ops import Feature, Hook

# (hook, site key, retail instruction at the site, note)
HOOKS = [
    ('POLL', 'kpad_read', 0x9421FF40,
     'KPADRead entry: drive the Serial Interface auto-poller for the GameCube port'),
    ('SAMPLE', 'sample_count', 0x881F010F,
     'KPADRead: GameCube pad -> Classic Controller samples in the KPAD ring'),
    ('PROBE', 'probe', 0x9421FFF0,
     'WPADProbe: a GameCube pad counts as a connected Classic Controller'),
]

NOP = 0x60000000


def word(dol, addr):
    return struct.unpack('>I', dol.read(addr, 4))[0]


def build(region, dol):
    sites = dict(g.GC_SITES[region])
    sites['sample_count'] = sites['kpad_read'] + g.SAMPLE_COUNT_OFF
    if word(dol, sites['si_get_type']) != 0x9421FFE0:
        raise SystemExit('%s: SIGetType not at 0x%08X' % (region, sites['si_get_type']))
    if dol.read(g.STATE, 0x40) != bytes(0x40):
        raise SystemExit('%s: state area 0x%08X is not zero' % (region, g.STATE))

    defs = {
        'STATE': '0x%08Xu' % g.STATE,
        'SI_TYPES': '0x%08Xu' % sites['si_types'],
        'SI_BUSY': '0x%08Xu' % (sites['si_types'] - 0x18),
        'SI_SHADOW': '0x%08Xu' % (sites['si_types'] - 0x14),
        'FN_SIGETTYPE': '0x%08Xu' % sites['si_get_type'],
        'FN_OSDISABLE': '0x%08Xu' % sites['os_disable'],
        'FN_OSRESTORE': '0x%08Xu' % sites['os_restore'],
        'FN_CTRL_OBJ': '0x%08Xu' % sites['ctrl'],
        'WPD_TBL': '0x%08Xu' % sites['wpd'],
    }
    ops, cur = [], GC_BASE
    for name, key, orig, note in HOOKS:
        site = sites[key]
        if word(dol, site) != orig:
            raise SystemExit('%s: unexpected instruction at %s 0x%08X' % (region, key, site))
        blob = asm.compile_c(os.path.join(HERE, 'gcpad.c'), os.path.join(HERE, 'gc_hooks.S'),
                             os.path.join(HERE, 'gc_link.ld'), dict(defs, **{'HOOK_' + name: '1'}))
        words = asm.words(blob)
        # the trailing nop is the slot the installer turns into the branch back
        if words[-1] != NOP:
            raise SystemExit('%s: hook %s does not end in its branch-back slot' % (region, name))
        words[-1] = 0
        ops.append(Hook(site, orig, words, cur, note=note))
        cur += (len(words) * 4 + 15) & ~15

    if cur > GC_END:
        raise SystemExit(f'gc code overflows its window: 0x{cur:X} > 0x{GC_END:X}')

    return Feature('gc', 'GameCube controller', region, ops)
