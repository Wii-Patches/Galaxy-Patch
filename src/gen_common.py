"""Per-region addresses for the GameCube controller feature.

Written against USA and carried to the other releases by masked-signature
matching of the code that uses each address (branch displacements and address
halves ignored, exactly one match required); data addresses were read back from
the matched lis/addi pairs.
"""

# kpad_read    KPADRead entry, `stwu r1,-192(r1)`; the sample-count check
#              `lbz r0,0x10f(r31)` is at +0x118 in every release
# probe        WPADProbe entry, `stwu r1,-16(r1)`
# si_get_type  SIGetType;  si_types: its per-port type cache (Si = types - 0x18:
#              +0 transfer-busy channel, +4 SIPOLL shadow)
# os_disable / os_restore   OSDisableInterrupts / OSRestoreInterrupts
# wpd          WPAD control block pointer per channel
# ctrl         the game's channel object for a channel (connected flag +0x35,
#              extension flag +0x36)
GC_SITES = {
    'RMGE01': dict(kpad_read=0x804506D8, probe=0x804D8ACC, si_get_type=0x804B1118, si_types=0x805E8768,
                   os_disable=0x804A80E8, os_restore=0x804A8110, wpd=0x80660170, ctrl=0x803AB568),
    'RMGP01': dict(kpad_read=0x804506F4, probe=0x804D8ACC, si_get_type=0x804B1118, si_types=0x805E8768,
                   os_disable=0x804A80E8, os_restore=0x804A8110, wpd=0x80660170, ctrl=0x803AB584),
    'RMGJ01': dict(kpad_read=0x804506D4, probe=0x804D8AAC, si_get_type=0x804B10F8, si_types=0x805E8768,
                   os_disable=0x804A80C8, os_restore=0x804A80F0, wpd=0x80660170, ctrl=0x803AB564),
    'RMGK01': dict(kpad_read=0x804520D0, probe=0x804D9F9C, si_get_type=0x804B359C, si_types=0x805FD1A0,
                   os_disable=0x804A9778, os_restore=0x804A97A0, wpd=0x80674AF0, ctrl=0x803ACB48),
}

SAMPLE_COUNT_OFF = 0x118

# Hook state: zeroed padding in .init at 0x80005BF4..0x800060C0 in every release,
# referenced by nothing.
STATE = 0x80005C00
