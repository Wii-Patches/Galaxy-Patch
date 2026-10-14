# hook: KPAD read, `lbz r0, 0x10f(r31)` -- the read of sample count waiting in the ring
# (r31 = channel's KPAD struct, r27 = channel number).
# Scratch: r0, r3-r12; r3 holds the slot pointer.
    lbz     0, 0x10f(31)
    cmpwi   0, 0
    bne     9f                          # samples are waiting: a remote is producing them
    cmplwi  27, 3
    bgt     9f
    lbz     0, 0x10e(31)
    cmplwi  0, 0x78
    blt     slot
    li      0, 0
slot:
    mulli   3, 0, 0x38
    add     3, 3, 31
    addi    3, 3, 0x110                 # the next ring slot
    li      0, 0
    stw     0, 0x00(3)
    stw     0, 0x04(3)
    stw     0, 0x08(3)
    stw     0, 0x0c(3)
    stw     0, 0x10(3)
    stw     0, 0x14(3)
    stw     0, 0x18(3)
    stw     0, 0x1c(3)
    stw     0, 0x20(3)
    stw     0, 0x24(3)
    stw     0, 0x28(3)
    stw     0, 0x2c(3)
    stw     0, 0x30(3)
    stw     0, 0x34(3)
    .set    CHAN, 27
    .set    SMP, 3
#include gc_convert.s
    lbz     0, 0x37(3)
    andi.   0, 0, 2
    beq     9f                          # no pad answered: nothing to add
    lbz     5, 0x10e(31)
    cmplwi  5, 0x78
    blt     idx
    li      5, 0
idx:
    addi    5, 5, 1
    stb     5, 0x10e(31)
    lbz     5, 0x10f(31)
    cmplwi  5, 0x78
    bge     full
    addi    5, 5, 1
    stb     5, 0x10f(31)
full:
    # Announce the pad to the game.  The game registers its own WPAD-level callbacks
    # (not KPAD's), and its error screen shows the "Wii Remote / Nunchuk communications
    # interrupted" prompt until they have fired: connect(chan, 0), then extension(chan, 2).
    # The game clears its connected / extension flags (+0x35 / +0x36 of the channel object)
    # on scene changes, so announce again whenever either has dropped.
    # WPAD control block of the channel = WPD_TABLE[chan] (connect callback at +0x8A4,
    # extension callback at +0x8A0); WPD_TABLE and CTRL_OBJ are per region.
    lbz     0, 0x1be2(31)
    cmpwi   0, 0
    beq     announce                    # never announced yet
    mr      3, 27
    lis     12, CTRL_OBJ@ha
    addi    12, 12, CTRL_OBJ@l
    mtctr   12
    bctrl                               # r3 = the game's channel object
    cmpwi   3, 0
    beq     9f
    lbz     0, 0x35(3)
    cmpwi   0, 0
    beq     announce
    lbz     0, 0x36(3)
    cmpwi   0, 0
    bne     9f
announce:
    lis     12, WPD_TABLE@ha
    addi    12, 12, WPD_TABLE@l
    slwi    4, 27, 2
    lwzx    12, 12, 4
    cmpwi   12, 0
    beq     9f
    lwz     11, 0x8A4(12)
    cmpwi   11, 0
    beq     9f                          # game has not registered yet: retry next frame
    li      0, 1
    stb     0, 0x1be2(31)
    li      0, 0
    stb     0, 0x1be3(31)
    li      0, 1
    stb     0, 0x1bdf(31)
    mr      3, 27
    li      4, 0
    mtctr   11
    bctrl                               # connect callback (chan, 0)
    lis     12, WPD_TABLE@ha
    addi    12, 12, WPD_TABLE@l
    slwi    4, 27, 2
    lwzx    12, 12, 4
    lwz     11, 0x8A0(12)
    cmpwi   11, 0
    beq     9f
    mr      3, 27
    li      4, 2
    mtctr   11
    bctrl                               # extension callback (chan, 2 = Classic Controller)
9:
    lbz     0, 0x10f(31)                # displaced instruction
