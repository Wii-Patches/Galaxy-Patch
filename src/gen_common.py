"""Shared pieces of the Classic Controller and GameCube controller builders."""
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'tools'))
import asm
from ops import Hook, Patch

# Wii Remote / Nunchuk button bits (WPAD)
WM = dict(WM_LEFT=0x0001, WM_RIGHT=0x0002, WM_DOWN=0x0004, WM_UP=0x0008, WM_PLUS=0x0010,
          WM_2=0x0100, WM_1=0x0200, WM_B=0x0400, WM_A=0x0800, WM_MINUS=0x1000,
          WM_Z=0x2000, WM_C=0x4000, WM_HOME=0x8000)

# Classic Controller button bits (WPAD)
CC = dict(CC_UP=0x0001, CC_LEFT=0x0002, CC_ZR=0x0004, CC_X=0x0008, CC_A=0x0010, CC_Y=0x0020,
          CC_B=0x0040, CC_ZL=0x0080, CC_R=0x0200, CC_PLUS=0x0400, CC_HOME=0x0800,
          CC_MINUS=0x1000, CC_L=0x2000, CC_DOWN=0x4000, CC_RIGHT=0x8000)

# GameCube pad button bits (PAD)
PAD = dict(PAD_LEFT=0x0001, PAD_RIGHT=0x0002, PAD_DOWN=0x0004, PAD_UP=0x0008, PAD_Z=0x0010,
           PAD_R=0x0020, PAD_L=0x0040, PAD_A=0x0100, PAD_B=0x0200, PAD_X=0x0400, PAD_Y=0x0800,
           PAD_START=0x1000)

GC_SITES = {
    'RMGE01': dict(sample=0x8045121C, ring=0x804507F0, probe=0x804D8ACC, radio=0x80450720, wpd=0x80660170, ctrl=0x803AB568),
    'RMGP01': dict(sample=0x80451238, ring=0x8045080C, probe=0x804D8ACC, radio=0x8045073C, wpd=0x80660170, ctrl=0x803AB584),
    'RMGJ01': dict(sample=0x80451218, ring=0x804507EC, probe=0x804D8AAC, radio=0x8045071C, wpd=0x80660170, ctrl=0x803AB564),
    'RMGK01': dict(sample=0x80452C14, ring=0x804521E8, probe=0x804D9F9C, radio=0x80452118, wpd=0x80674AF0, ctrl=0x803ACB48),
}


def read(name):
    out = []
    for line in open(os.path.join(HERE, name)).read().split('\n'):
        out.append(read(line.split()[1]) if line.startswith('#include ') else line)
    return '\n'.join(out)


def hook(site, orig, base, source, syms, consts, note):
    words = asm.words(asm.assemble(read('macros.s') + '\n' + source, base, syms, consts)) + [0]
    return Hook(site, orig, words, base, note=note), (len(words) * 4 + 15) & ~15
