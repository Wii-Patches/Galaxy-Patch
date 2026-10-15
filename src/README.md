# Assembly Sources and Feature Generators

This directory holds the PowerPC assembly sources and Python builders that compile the patches into the static JSON definitions in `tools/prebuilt/`.

## Architecture

Super Mario Galaxy links the Nintendo RVL_SDK KPAD library (release build Aug 29 2007).
The game natively expects input from a Wii Remote and Nunchuk, using accelerometer shakes for Mario's Spin Attack and the Wii Remote IR camera for the Star Pointer.

The patch provides two features:
1. **Classic Controller (`cc`)**: Hooks into the game engine's input manager, pointer routines, and panic handlers to natively process Classic Controller extension data (left stick moves Mario, right stick controls the Star Pointer, buttons map to jump, spin attack, crouch, star bit firing, and camera reset).
2. **GameCube Controller (`gc`)**: The game links the SDK's `SI` library but not `PAD`, so nothing ever polls the GameCube ports and, on a real console, the pad's response registers stay empty. Three hooks, all in `gcpad.c`, fix that:
   - **POLL** (`KPADRead` entry): probes the port with `SIGetType` (at most every 0.25 s) until a standard pad answers, then turns on the Serial Interface's own auto-polling for that port (poll command in `OUTBUF`, `SISR` write latch, `SIPOLL` enable bits, mirrored into the SDK's `SIPOLL` shadow so the retrace handler keeps them). It also re-probes after an unplug and clears a wedged SI transfer.
   - **SAMPLE** (`KPADRead`, the `lbz r0,0x10f(r31)` sample-count check): writes the pad as Classic Controller samples (`dev=2, format=8`) into KPAD's 120-entry sample ring. Without a Wii Remote it queues two fresh samples per frame and fires the game's own WPAD connect / extension callbacks so it drops the "communications interrupted" prompt; with a bare Wii Remote, the pad becomes its extension. A real Nunchuk or Classic Controller is never overridden.
   - **PROBE** (`WPADProbe` entry): reports a connected Classic Controller while a pad answers on the matching port.

   GameCube port N drives Wii Remote channel N. Hook state lives in zeroed padding in `.init` (`0x80005C00`), not in the patch, because Gecko loaders rewrite their codes every frame.

## Files

- `gcpad.c`: The GameCube controller hooks (compiled once per hook with `-DHOOK_POLL`, `-DHOOK_SAMPLE` or `-DHOOK_PROBE`).
- `gc_hooks.S`: Entry stubs that save the volatile registers, call the C hook and run the displaced instruction.
- `gc_link.ld`: Linker script that lays out each hook blob (entry branch, C code, stub).
- `gen_common.py`: Per-region addresses for the GameCube controller feature.
- `gen_cc.py`: Generates the Classic Controller feature for each region.
- `gen_gc.py`: Generates the GameCube controller feature for each region.
- `cc_xml/`: Reference Classic Controller patch definitions across USA, Europe, Japan, and Korea.
