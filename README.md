# Super Mario Galaxy Patch

Play **Super Mario Galaxy** (Wii) with a **GameCube controller** — standalone in port 1 without needing a Wii Remote — or with a **Classic Controller**. Works across all four regional releases: USA (`RMGE01`), Europe (`RMGP01`), Japan (`RMGJ01`), and Korea (`RMGK01`).

The patches are applied to your own clean disc image: drop a clean `.wbfs` or `.iso` onto the patcher and play the result on a Wii (USB loader) or in Dolphin. Nothing copyrighted from the game is included in this repository.

![Super Mario Galaxy](assets/logo.png)

## How It Works

Super Mario Galaxy was designed exclusively for the Wii Remote and Nunchuk, using accelerometer shakes for Mario's Spin Attack and the IR camera for the Star Pointer.

This patch synthesises native Classic Controller state directly into Nintendo's `KPAD` sample ring buffer inside `main.dol`. Downstream hooks redirect Mario's movement to the left analog stick, the Star Pointer to the right analog stick (or GameCube C-Stick), and spin attacks to dedicated buttons.

The game never polls the GameCube ports itself (it has no `PAD` library), so the GameCube patch drives the Wii's Serial Interface auto-poller directly: it probes the port until a pad answers, enables polling, and re-probes after an unplug. While a pad answers, it is presented to the game as a Classic Controller on the matching Wii Remote channel, so no Wii Remote is needed.

## Controls

### GameCube Controller (Port 1)

Play with a GameCube controller connected to Port 1. **No Wii Remote is needed.**

| Input | Action |
| --- | --- |
| Control Stick | 360° Mario movement |
| C-Stick | Aim Star Pointer (collect star bits, pull Launch Stars) |
| A | Jump · Swim · Confirm |
| B | Back · Dive · Cancel |
| X / Y | Spin Attack (replaces Wii Remote shake) |
| Z | Crouch · Ground Pound · Long Jump (replaces Nunchuk Z) |
| L | Center / Reset Camera behind Mario |
| R | Shoot Star Bit / Lock On (replaces Wii Remote B) |
| Start | Pause menu |
| D-Pad | Camera adjustment |

### Classic Controller

Plug a Classic Controller or Classic Controller Pro into the Wii Remote:

| Input | Action |
| --- | --- |
| Left Stick | 360° Mario movement |
| Right Stick | Aim Star Pointer |
| A | Jump · Swim · Confirm |
| B | Back · Dive · Cancel |
| X / Y | Spin Attack |
| ZL | Crouch · Ground Pound · Long Jump |
| L | Center / Reset Camera |
| ZR / R | Shoot Star Bit / Lock On |
| + | Pause menu |
| D-Pad | Camera adjustment |
| HOME | Wii HOME Menu |

## Installing

### Patch Your Disc Image

You need a clean `.wbfs` or `.iso` of the game. Run the patcher from source (requires Python 3 and [Wiimms ISO Tool](https://wit.wiimm.de/) `wit` on your `PATH`):

```bash
python3 tools/gui.py
```

Tick the patches you want, then drop your disc image onto the window (or click to browse). The patcher verifies the disc ID, patches `sys/main.dol`, rebuilds the image in place, and saves a backup as `<name>.bak`.

There is also a command-line tool:

```bash
python3 tools/patch_disc.py "Super Mario Galaxy.wbfs" --cc --gc
```

### Gecko Codes (Dolphin & USB Loaders)

Copy `codes/<disc id>.ini` (`RMGE01`, `RMGP01`, `RMGJ01`, or `RMGK01`) into Dolphin's `GameSettings` directory, or use `codes/<disc id>.txt` with USB loaders (Ocarina / Gecko OS).

In Dolphin:
1. Enable **Cheats / Gecko Codes**.
2. Set **GameCube Port 1** to **Standard Controller**.
3. Launch the game!

### Riivolution

Copy the `riivolution/` directory to the root of your SD card / USB drive for Riivolution on real hardware.

## Building from Source

To verify or rebuild everything from the assembly sources:

```bash
python3 tools/gen_prebuilt.py    # compiles src/*.s to tools/prebuilt/*.json (requires devkitPPC)
python3 tools/build.py           # generates codes/ and riivolution/ files
python3 tools/check.py           # runs consistency checks
python3 tools/verify.py          # verifies against retail DOLs
```

## License

MIT License. Copyright (c) 2026 quatric.

### Modded images

Disc patchers match the first four characters of the game ID (ID4), so mods can change the last two characters. The original disc ID and filename are preserved. Revision and executable patch-site checks still apply; mods that change required code may be incompatible.
