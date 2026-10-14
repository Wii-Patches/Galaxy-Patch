"""Build the Classic Controller feature for one region from XML and retail DOL."""
import os
import sys
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', 'tools'))
from ops import Blob, Feature, Patch

XML_FILES = {
    'RMGE01': 'SMG-ClassicController-NTSCU.xml',
    'RMGP01': 'SMG-ClassicController-PAL.xml',
    'RMGJ01': 'SMG-ClassicController-NTSCJ.xml',
    'RMGK01': 'SMG-ClassicController-NTSCK.xml',
}


# 0x8000299C stands in for `lbz r0,94(r3)` at one call site and `lbz r4,94(r3)` at the
# other, so it has to hand its answer back in both r0 and r4.  The reference XML only sets
# r0; the other caller then sees the loop index in r4 and rejects a lone sample as invalid.
# A Wii Remote delivers several samples per frame and gets away with it, a GameCube pad
# (one synthesised sample per frame) never gets its pointer accepted.
DEV_VALID_ADDR = 0x8000299C
DEV_VALID_FIXED = bytes.fromhex('8803005F2C0000088803005E40820008380000027C0403784E800020')


def build(region, dol):
    xml_name = XML_FILES[region]
    xml_path = os.path.join(HERE, 'cc_xml', xml_name)
    tree = ET.parse(xml_path)
    root = tree.getroot()
    mem_patches = root.findall('.//patch[@id="ccnvidia"]/memory')

    ops = []
    for p in mem_patches:
        addr = int(p.get('offset'), 16)
        new_bytes = bytes.fromhex(p.get('value'))
        if addr == DEV_VALID_ADDR:
            new_bytes = DEV_VALID_FIXED
        orig_hex = p.get('original')
        if addr >= 0x80300000:
            if orig_hex:
                orig_bytes = bytes.fromhex(orig_hex)
            else:
                orig_bytes = dol.read(addr, len(new_bytes))
            ops.append(Patch(addr, new_bytes, orig_bytes, f'In-DOL CC hook at 0x{addr:08X}'))
        else:
            ops.append(Blob(addr, new_bytes, f'CC low-memory routine at 0x{addr:08X}'))

    return Feature('cc', 'Classic Controller', region, ops)
