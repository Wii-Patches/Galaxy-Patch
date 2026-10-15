/* GameCube controller -> Classic Controller bridge for Super Mario Galaxy.
 *
 * Three hooks, one blob each (gc_hooks.S provides the register-saving entry
 * stubs).  Super Mario Galaxy links the SI library but not PAD, so nothing ever
 * polls the GameCube ports and the pad's response registers stay empty on a
 * real console.  gc_poll() drives the Serial Interface's own auto-polling,
 * gc_sample() turns the pad's state into Classic Controller samples in KPAD's
 * ring, and gc_probe() makes WPADProbe report a Classic Controller on the
 * channel, so the game and the Classic Controller patch treat the pad as one.
 * GameCube port N drives Wii Remote channel N.
 *
 * Addresses come in as -D macros, resolved per region by gen_gc.py.
 */
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned char u8;

#define R32(a) (*(volatile u32 *)(a))
#define SI_OUT(c)  (0xCD006400u + (c) * 12)
#define SI_INH(c)  (0xCD006404u + (c) * 12)
#define SI_INL(c)  (0xCD006408u + (c) * 12)
#define SI_POLL    (0xCD006430u)
#define SI_COMCSR  (0xCD006434u)
#define SI_SR      (0xCD006438u)

/* KPAD channel struct: sample ring of RING entries at +0x110, 0x38 bytes each */
#define RING 120
#define K_IDX  0x10E
#define K_CNT  0x10F

/* WPAD control block of a channel (WPD_TBL[chan]) */
#define WPD_EXT_CB  0x8A0           /* extension callback (chan, type) */
#define WPD_CONN_CB 0x8A4           /* connect callback (chan, result) */
#define WPD_STATUS  0x8BC           /* -1 = no Wii Remote */
#define WPD_EXT     0x8C1           /* extension type */

/* State lives in zeroed padding in .init (identical in every region), not in
 * the patch itself: Gecko loaders rewrite the patch's own bytes every frame. */
struct st {
    u32 probe_tb[4];    /* last SIGetType per port */
    u32 busy_tb;        /* when si:: was first seen busy (0 = idle) */
    u8 norep[4];        /* consecutive reads with NOREP per port */
    u8 ours[4];         /* the last KPAD samples on the channel were ours */
    u16 prev_btn[4];    /* Classic Controller buttons of the previous frame */
};
#define ST ((volatile struct st *)STATE)

static inline u32 tb(void)
{
    u32 t;
    __asm__ volatile("mftb %0" : "=r"(t));
    return t;
}

/* SIGetType's cached answer for the port is a standard (or WaveBird) pad */
static inline int pad_type(u32 type)
{
    return !(type & 0x8F) && (type & 0x18000000u) == 0x08000000u;
}

/* a valid, error-free pad response on port `chan` while it is being polled */
static inline int gc_in(u32 chan, u32 *h, u32 *l)
{
    u32 v;

    if (!(R32(SI_SHADOW) & (0x80u >> chan)))
        return 0;           /* not polled: INBUF holds a stale response */
    v = R32(SI_INH(chan));
    if ((v & 0x80000000u) || !(v & 0x00800000u))
        return 0;
    *h = v;
    *l = R32(SI_INL(chan));
    return 1;
}

#if defined(HOOK_POLL)
/* KPADRead(chan, ...) entry: once per channel per frame */
void gc_poll(u32 chan)
{
    volatile u32 *types = (volatile u32 *)SI_TYPES;
    u32 sisr, bits, mask, poll, shadow;
    s32 busy;

    if (chan > 3)
        return;

    /* probe the port until a standard pad answers, at most every 0.25 s:
     * probing every frame collides with the pad's own polling on hardware */
    if (!pad_type(types[chan])) {
        u32 now = tb();
        if (now - ST->probe_tb[chan] >= 15187500u) {
            ST->probe_tb[chan] = now;
            ((u32 (*)(u32))FN_SIGETTYPE)(chan);
        }
    }

    /* an unplugged pad latches NOREP; si:: never reads it (no PAD library),
     * so copy a persistent one into the type cache ourselves, which makes
     * SIGetType probe the port again once a pad is plugged back in */
    sisr = R32(SI_SR);
    if (sisr & (0x08000000u >> (chan * 8))) {
        if (ST->norep[chan] < 10)
            ST->norep[chan]++;
        else
            types[chan] = 8;
    } else {
        ST->norep[chan] = 0;
    }

    R32(SI_OUT(chan)) = 0x00400300u;            /* poll command: buttons, sticks, analog L/R */
    /* ack this port's errors; WR latches the OUTBUF values into the hardware */
    R32(SI_SR) = (sisr & (0x0F000000u >> (chan * 8))) | 0x80000000u;

    bits = 0x88u >> chan;                       /* ENn + VBCPYn */
    mask = pad_type(types[chan]) ? bits : 0;    /* a port can only be probed while not polled */
    poll = R32(SI_POLL);
    if (!(poll & 0xFF00u))
        poll |= 0x0100u;                        /* at least one poll per frame */
    R32(SI_POLL) = (poll & ~bits) | mask;
    /* si:: rewrites SIPOLL from its own shadow on every retrace */
    shadow = R32(SI_SHADOW);
    R32(SI_SHADOW) = (shadow & ~bits) | mask;

    /* a pad unplugged mid-transfer leaves si::'s global busy flag wedged
     * (nothing times it out); force it idle after a second */
    busy = (s32)R32(SI_BUSY);
    if (busy == -1) {
        ST->busy_tb = 0;
    } else {
        u32 now = tb();
        if (ST->busy_tb == 0) {
            ST->busy_tb = now | 1;
        } else if (now - ST->busy_tb >= 60750000u) {
            u32 lvl = ((u32 (*)(void))FN_OSDISABLE)();
            R32(SI_BUSY) = (u32)-1;
            R32(SI_COMCSR) = 0x80000000u;
            ((void (*)(u32))FN_OSRESTORE)(lvl);
            ST->busy_tb = 0;
        }
    }
}
#endif

#if defined(HOOK_SAMPLE)
/* Classic Controller buttons as WPAD reports them */
#define CC_UP    0x0001
#define CC_LEFT  0x0002
#define CC_ZR    0x0004
#define CC_X     0x0008
#define CC_A     0x0010
#define CC_Y     0x0020
#define CC_B     0x0040
#define CC_ZL    0x0080
#define CC_R     0x0200
#define CC_PLUS  0x0400
#define CC_MINUS 0x1000
#define CC_L     0x2000
#define CC_DOWN  0x4000
#define CC_RIGHT 0x8000

static inline s16 stick(u32 raw)
{
    s32 v = ((s32)(raw & 0xFF) - 128) * 3;   /* pad ~+-100 -> Classic ~+-300 */
    if (v > 308)
        v = 308;
    if (v < -308)
        v = -308;
    return (s16)v;
}

static __attribute__((noinline)) u32 cc_buttons(u32 h, u32 l)
{
    u32 b = 0;

    if (h & 0x01000000u) b |= CC_A;          /* A: jump / confirm */
    if (h & 0x02000000u) b |= CC_B;          /* B: back / cancel */
    if (h & 0x04000000u) b |= CC_X;          /* X: spin */
    if (h & 0x08000000u) b |= CC_X;          /* Y: spin */
    if (h & 0x10000000u) b |= CC_PLUS;       /* Start: pause */
    if (h & 0x00100000u) b |= CC_ZL;         /* Z: crouch / ground pound */
    if ((h & 0x00400000u) || ((l >> 8) & 0xFF) >= 0x80)
        b |= CC_L;                           /* L: centre camera */
    if ((h & 0x00200000u) || (l & 0xFF) >= 0x80)
        b |= CC_ZR;                          /* R: shoot star bit */
    if (h & 0x00080000u) b |= CC_UP;
    if (h & 0x00040000u) b |= CC_DOWN;
    if (h & 0x00020000u) b |= CC_RIGHT;
    if (h & 0x00010000u) b |= CC_LEFT;
    return b;
}

static __attribute__((noinline)) void fill_cc(u8 *s, u32 h, u32 l, u32 b)
{
    *(u16 *)(s + 0x2A) = (u16)b;
    *(s16 *)(s + 0x2C) = stick(h >> 8);      /* control stick x */
    *(s16 *)(s + 0x2E) = stick(h);           /* control stick y */
    *(s16 *)(s + 0x30) = stick(l >> 24);     /* C-stick x */
    *(s16 *)(s + 0x32) = stick(l >> 16);     /* C-stick y */
    s[0x34] = (b & CC_L) ? 180 : 0;          /* analog L / R */
    s[0x35] = (b & CC_ZR) ? 180 : 0;
    s[0x28] = 2;                             /* extension: Classic Controller */
    s[0x29] = 0;                             /* no extension error */
    s[0x36] = 8;                             /* classic + accel + pointer data */
}

static inline u8 *slot(u8 *k, u32 i)
{
    return k + 0x110 + (i % RING) * 0x38;
}

/* one fresh Classic Controller sample at ring slot `i` */
static __attribute__((noinline)) void put_sample(u8 *k, u32 i, u32 h, u32 l, u32 b)
{
    u32 *p = (u32 *)slot(k, i);
    u32 n;

    for (n = 0; n < 0x38 / 4; n++)
        p[n] = 0;
    fill_cc((u8 *)p, h, l, b);
}

/* The game shows its "Wii Remote / Nunchuk communications interrupted" prompt
 * until its own WPAD connect and extension callbacks have fired, and it clears
 * its connected / extension flags (+0x35 / +0x36 of its channel object) on
 * scene changes.  Fire connect(chan, 0) then extension(chan, 2) whenever the
 * pad has not been announced yet or either flag has dropped. */
static __attribute__((noinline)) void announce(u8 *k, u32 chan)
{
    u8 *blk, *obj;
    void (*cb)(u32, s32);

    if (k[0x1BE2]) {
        obj = ((u8 *(*)(u32))FN_CTRL_OBJ)(chan);
        if (!obj || (obj[0x35] && obj[0x36]))
            return;
    }
    blk = *(u8 **)(WPD_TBL + chan * 4);
    if (!blk)
        return;
    cb = *(void (**)(u32, s32))(blk + WPD_CONN_CB);
    if (!cb)
        return;                              /* the game has not registered yet: retry next frame */
    k[0x1BE2] = 1;                           /* KPAD: connection reported */
    k[0x1BE3] = 0;                           /* KPAD: disconnection not reported */
    k[0x1BDF] = 1;
    cb(chan, 0);
    blk = *(u8 **)(WPD_TBL + chan * 4);
    cb = blk ? *(void (**)(u32, s32))(blk + WPD_EXT_CB) : 0;
    if (cb)
        cb(chan, 2);
}

/* KPADRead before the sample-count check: r31 = KPAD channel struct, r27 = channel.
 *
 * Two samples per frame: KPADRead hands the game several statuses, as a Wii
 * Remote delivers two or three per read.  The older carries the previous
 * frame's buttons, so press and release edges land in the newest entry. */
void gc_sample(u8 *k, u32 chan)
{
    u32 h, l, b, i, idx, cnt, lvl, sync = 0;
    u8 *blk;

    if (chan > 3)
        return;
    if (!gc_in(chan, &h, &l)) {
        ST->ours[chan] = 0;
        return;
    }

    b = cc_buttons(h, l);
    lvl = ((u32 (*)(void))FN_OSDISABLE)();  /* the sampling callback fills the ring from an interrupt */
    idx = k[K_IDX];
    if (idx >= RING)
        idx = 0;
    cnt = k[K_CNT];
    if (cnt == 0) {
        /* no sample queued: unless a Wii Remote with a real Nunchuk or
         * Classic Controller is connected, the pad is this channel's controller */
        blk = *(u8 **)(WPD_TBL + chan * 4);
        if (blk && *(s32 *)(blk + WPD_STATUS) != -1 && (blk[WPD_EXT] == 1 || blk[WPD_EXT] == 2)) {
            ((void (*)(u32))FN_OSRESTORE)(lvl);
            return;
        }
        put_sample(k, idx, h, l, ST->prev_btn[chan]);
        put_sample(k, idx + 1, h, l, b);
        k[K_IDX] = (idx + 2) % RING;
        k[K_CNT] = 2;
        ST->prev_btn[chan] = b;
        ST->ours[chan] = 1;
        sync = 1;
    } else {
        /* real samples queued: a bare Wii Remote gets the pad as its extension;
         * a real Nunchuk or Classic Controller is never touched */
        ST->ours[chan] = 0;
        ST->prev_btn[chan] = b;
        if (cnt > RING)
            cnt = RING;
        for (i = 0; i < cnt; i++) {
            u8 *s = slot(k, idx + RING - cnt + i);
            if (s[0x28] == 0 || s[0x28] == 0xFD)
                fill_cc(s, h, l, b);
        }
        if (cnt == 1) {
            /* a lone real sample: queue a copy after it so there is a second entry */
            u32 *src = (u32 *)slot(k, idx + RING - 1);
            u32 *dst = (u32 *)slot(k, idx);
            if (((u8 *)src)[0x28] == 2) {
                for (i = 0; i < 0x38 / 4; i++)
                    dst[i] = src[i];
                k[K_IDX] = (idx + 1) % RING;
                k[K_CNT] = 2;
            }
        }
    }
    ((void (*)(u32))FN_OSRESTORE)(lvl);
    if (sync)
        announce(k, chan);
}
#endif

#if defined(HOOK_PROBE)
/* WPADProbe(chan, &type): report a Classic Controller on the channel while a
 * pad is plugged into the matching port, unless the channel already has a
 * real Nunchuk or Classic Controller */
u32 gc_probe(u32 chan, u32 *type)
{
    u32 h, l;
    u8 *blk;

    if (chan > 3 || !gc_in(chan, &h, &l))
        return 0;
    blk = *(u8 **)(WPD_TBL + chan * 4);
    if (!blk)
        return 0;
    if (*(s32 *)(blk + WPD_STATUS) != -1 && (blk[WPD_EXT] == 1 || blk[WPD_EXT] == 2))
        return 0;
    if (type)
        *type = 2;
    return 1;
}
#endif
