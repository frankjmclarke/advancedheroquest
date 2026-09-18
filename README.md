# HQ-Map — Advanced HeroQuest Map Generator

A random dungeon map generator for Games Workshop's *Advanced HeroQuest*, written
in C for Windows. It rolls up a dungeon from user-selectable quest tables, draws
it with the tile artwork in `rsh/`, and can export the map and its monster list.

Copyright © Jürgen Albuschies, 1999. Source version `1.0` (`version.h`);
the resource `VERSIONINFO` block carries product version `2.0.5.0`.

> **Provenance:** this is a preservation mirror of someone else's work. No
> license has been granted by the copyright holder and all rights remain
> reserved — see [NOTICE.md](NOTICE.md). If you are the author or rights
> holder, open an issue and any request will be honoured.

Writing your own adventures: **[Guide to creating and editing table
files](doc/TABLE-GUIDE.md)**.

Screenshots of the workflow are in [`doc/`](doc/), numbered in the order you'd
use them — table selection, generation, monster display, saving.

Quests appear in *Options → Load Tables* by scanning `tables/` when the dialog
opens, so a hand-written one shows up simply by being dropped in. Besides the
1999 originals, the tree carries two campaigns written since: **Ravenloft**, ten
quests of Warhammer undead, and **[Sentinel V](tables/sentinel/README.md)**, a
five-deck Advanced HeroQuest campaign played with the Milton Bradley *Space
Crusade* miniatures — the adventurers are Space Marines, and the last deck is
held by the Eldar.

---

## What gets built

Two executables, both from the same object set:

| Binary | Built from | Notes |
| --- | --- | --- |
| `bin/hq_map.exe` | `main.c` + `wind.c` | Full version |
| `bin/hq_mapdm.exe` | `maindm.c` + `winddm.c` | Demo version |

The demo is not a separate program. `maindm.c` and `winddm.c` are two lines
each — they `#define DEMO 1` and then `#include "main.c"` / `#include "wind.c"`.
`demo.h` defaults `DEMO` to `0`, so the same sources compile both ways.

Everything else (`makemap.c`, `table.c`, `rolldice.c`, the `my_lib` layer, …) is
compiled once and linked into both.

## Toolchain

The build predates anything you likely have installed. The original toolchain is
**Borland C++ 4.52** — not Turbo C++, which never shipped a 4.52. The makefile
declares it at line 15 and the resource compiler is invoked with
`-D__BORLANDC__=0x452`.

The build is driven by **Borland MAKE**, not GNU make: `makefile` uses `!if` /
`!ifdef` / `!error` directives and `<<` inline response files, neither of which
GNU make understands.

Three toolchains are selectable via `COMPILER=`, but only one is complete:

- `BORLANDC` (default) — `bcc32`, `tlink32`, `brc32`. **The only one that links.**
- `MSVC20` / `MSVC40` — tool paths and flags are defined (makefile lines 140-212),
  but the rules that actually link `hq_map.exe` and compile the `.res` files are
  wrapped in `!if "$(COMPILER)"=="BORLANDC"` (lines 890, 910, 931, 940). Selecting
  MSVC compiles objects and then silently produces no executable.

A separate `makefile.gcc` targets gcc/MinGW with `windres` for resources. It is
GNU make syntax and builds `hq_map` only — there is no `hq_mapdm` target in it.

## Before you build: fix the hardcoded paths

Every makefile hardcodes absolute paths from the author's machine. Nothing will
build until these are pointed at your own installation.

`makefile`:

| Line | Variable | Value to change |
| --- | --- | --- |
| 8 | `TOP` | `P:\hero` → this source tree |
| 79 | `BC_DIR` | `p:\bc\bc45` → your Borland C++ 4.52 install |
| 95 | `TLINK32` | `p:\bc\bc\bin\tlink32` → note this points *outside* `BC_DIR` |

`makefile.bcc` is the same file with a different set of hardcoded paths
(`TOP=h:\src\hq_mapw`, `BC_DIR=f:\bc45`, `TLINK32=c:\bc\bin\tlink32`) — it differs
from `makefile` in exactly those three lines. Use whichever is the closer starting
point. `makefile.gcc` has its own at line 8 (`TOP=//h/src/hq_mapw`).

`bccw32.cfg` — the checked-in 32-bit compiler config — also carries absolute
include paths on line 1: `-IP:\hero\my_lib`, `-IP:\hero\my_lib\windows`,
`-Ip:\bc\bc45\include`.

Note that the makefile has rules to *generate* `bccw16.cfg`, `bccw32.cfg`,
`bccdos.cfg`, `bccd16.cfg` and `bccd32.cfg` from the `INCLUDES`/`DEFINES`
variables (makefile line 387 onward). Only `bccw32.cfg` is checked in. If you
delete it after fixing `TOP`, make will regenerate it with correct paths — that is
easier than editing it by hand.

## Building

From the source root, with Borland's `make` on `PATH`:

```
make                        # SYSTEM=WIN32, COMPILER=BORLANDC, both exes
make SYSTEM=WIN16           # 16-bit Windows
make SYSTEM=DOS             # 16-bit DOS
make hq_map                 # full version only
make hq_mapdm               # demo version only
make dirs                   # create obj/{win16,win32} and bin/
make clean                  # delete objects and .res files
make strip                  # run tdstrp32 over the built exes
```

`SYSTEM` accepts `WIN16`, `WIN32`, `DOS`, `DLL16`, `DLL32`; it defaults to `WIN32`
(makefile line 52). An unrecognised value trips `!error ... system not supported`.
`MODEL` defaults to `l` (large) and only affects the 16-bit targets.

`make dirs` runs as a dependency of the `hq_map` targets, so you don't normally
need to call it yourself. Objects land in `obj/win32\` (or the matching
per-system directory) and executables in `bin\`.

For gcc instead: `make -f makefile.gcc`.

### Building with Visual Studio instead

If you have Visual Studio rather than a Borland install, run `build-msvc.bat`.
It bypasses `makefile` entirely — there is nothing to salvage there for MSVC, as
noted above — and drives `cl` / `rc` / `link` directly:

```
build-msvc.bat          build bin\hq_map.exe and bin\hq_mapdm.exe
build-msvc.bat clean    remove obj\msvc and the generated .rc
```

It locates the toolchain with `vswhere` (requiring the *Desktop development with
C++* workload), builds 32-bit, and puts objects in `obj\msvc\`. `obj\win32\`,
which holds the original 1999 Borland objects, is left alone.

Verified against MSVC 14.44 (VS2022): **all 40 objects compile with zero errors
and no changes to the 1999 sources.** `my_lib/portab.h` already carries `_MSC_VER`
branches, which is why 27-year-old code builds on a modern compiler untouched.

Two things the script has to work around:

- **`rc` rejects one construct `brc` accepts.** `rsh/menu.rc` lines 189 and 208
  use adjacent string-literal concatenation (`CAPTION "...V" AHQ_MAP_VERSION`).
  Microsoft's resource compiler does not concatenate there, and reports it as
  `RC2112: BEGIN expected in dialog` followed by `RC2135: file not found: MS Sans
  Serif` — neither of which points at the real cause. The script generates
  `rsh/menu.msvc.rc` with the version folded into a single literal (preserving
  CP1252, so the umlauts survive) and leaves `menu.rc` untouched.
- **The `.def` file is not reused.** `hq_map32.def`'s `IMPORTS` section is Borland
  module-definition syntax that `link.exe` rejects, so the script passes
  `/STACK:20480 /HEAP:4096` explicitly to match what the `.def` declares.

Two `RC2182: duplicate dialog control ID 2` warnings are pre-existing in the 1999
`menu.rc` and appear in both toolchains.

Because `bin/*.exe` is gitignored, an original 1999 binary sitting in `bin/`
cannot be restored from git once overwritten. The script therefore copies any
pre-existing `hq_map.exe` / `hq_mapdm.exe` to `*.prebuild.exe` once before
overwriting it.

### How the link works

Worth knowing, because it's unusual and makes link failures confusing to read.
The link rule (makefile line 891) does `cd` into the object directory, then feeds
`tlink32` an inline response file containing, in order: link flags, the C0 startup
object, the object list, output name, map name, libraries, the `.def` file, and
the compiled resources. So:

- **Module definition** — `hq_map32.def` for 32-bit, `hq_map.def` for 16-bit. Both
  set `NAME HQ_MAP`, a 4 KB heap and a ~20 KB stack, and list explicit `IMPORTS`
  (`KERNEL32.*` and `KERNEL.*` respectively). `hq_mapdm.exe` links against the
  *same* `.def` (line 920), so it also reports itself as `HQ_MAP`.
- **Libraries** — `import32.lib`, `ole2w32.lib`, `cw32.lib` for Win32/Borland.
- **Resources** — `menu.res` and `grafic.res`, built by `brc32 -R` from
  `rsh/menu.rc` and `rsh/grafic.rc`. Both depend on `version.h`, so bumping the
  version string forces a resource rebuild.

## Repository layout

```
build-msvc.bat      Visual Studio build script (see above)
*.c, *.h            Generator core: makemap, map, pice, stairs, features,
                    rolldice, table, set, queue — plus the Win32 GUI
                    (wind, icon, image, bmp, pcx, text)
my_lib/             Portability layer (headers + generic .c)
my_lib/windows/     Its Windows backend: window, w_draw, w_dialog, w_mouse,
                    w_print, file_io, filesel, memory, mfdb, profile
rsh/                Resources: menu.rc/.rh, grafic.rc/.rh, ja.ico,
                    ~200 map tile .bmp files, and two .RWS files
tables/             Quest tables (.tab) — standard, sonne, dark, terror,
                    faces, ritual, priests, oath, amulett, eyes, rivers,
                    ravenloft, sentinel
maps/               Sample generated maps (.bmp + .mon monster lists)
doc/                Usage screenshots
bin/ahq_map.ini     Runtime settings
```

Build output (`obj/`, `*.exe`) is excluded by `.gitignore`.

## Running it

`bin/ahq_map.ini` stores window positions, generation parameters and — the part
that matters — absolute paths to the data directories. It ships pointing at the
author's drive:

```ini
[Karte]
path=P:\hero\maps
[Tabelle]
path=P:\hero\tables
[Editor]
path=P:\hero\bin\
```

Point `[Karte]`, `[Tabelle]`, `[Fenster]`, `[Editor]` and `[Editpath]` at your own
checkout, or the file selectors will open on a drive that doesn't exist.

The **UI is English**; the **INI keys are still German** (`Karte` = map, `Tabelle`
= table, `Fenster` = window, `Treppe` = stairs, `Eingang` = entrance). That split
is deliberate: renaming the keys would silently reset every setting in an
existing `ahq_map.ini`, since the old names would no longer be read.

Only the 32-bit build is worth targeting on a modern machine — 64-bit Windows
dropped the 16-bit subsystem, so the `WIN16` and `DOS` targets need DOSBox or a
VM to run.

### Inspecting a room

Click a room in the graphic map to open **Room contents**, showing its room
number and the generated encounter, treasure, and other description text.
The window stays open while you click other rooms; move it beside the map
for reference during play. Long descriptions scroll and can be selected and
copied. Clicking corridors or blank space leaves the current contents visible.
Closing or regenerating the map closes the contents window.

Room contents also includes AHQ monster references beneath matching encounter
entries: characteristics, hand-to-hand and ranged tables, and equipment notes.
Explicit singular/plural aliases are matched without guessing creature subtypes.
When the source has multiple profiles (for example Wights or Ghouls), all are
labelled with variant numbers. Unrecognized counted
creatures are marked as missing; their original encounter text is retained.
The reference area uses a fixed-width font with horizontal and vertical scrolling.

The shared source is `data/ahq-monsters.json`. To update it, edit the profile or
its explicit aliases, then regenerate both outputs:

```powershell
python tools/ahq-monster-sheet.py
python tools/generate-ahq-reference.py
python tools/generate-ahq-reference.py --check
.\build-msvc.bat
```

The generated `ahq-reference-data.h` is checked in with the sources. The Windows
application reads the embedded C data, so it needs neither Python nor the PDF at
runtime. Printed values, source variants and blank cells are preserved.

### Player view and fog of war

Choose **Map → Player View (fog of war)** after generating a dungeon. The
separate map window initially shows all corridors, stairs, and their doors.
Rooms stay blank until you click a visible doorway. That reveals the adjoining
room, its map contents, and all its doors; it does not reveal further rooms.
All doors includes secret doors adjoining a visible corridor or revealed room.

Click a revealed room to read its contents. The normal graphic map remains
fully visible for the GM. Both maps support zoom and scrolling; **Fit to Window**
uses the active map window. Reopening Player View preserves discoveries for the
current map. Generating a new map or using Close All resets discoveries.
Discovery state is kept only for the current application session.

### Colour

The map is drawn in colour on screen, keyed to what each piece is:

| | |
| --- | --- |
| corridors | cool slate grey |
| rooms / hazards | amber |
| water, wells, bridges, chasms | blue |
| growth, slime, cess pits | green |
| chests and treasure | gold |
| tombs, apparitions, wights | violet |
| occupants (monsters, NPCs) | crimson |
| furnishings, stairs, doors | brown |
| magic circles | magenta |
| lairs | green |
| quest rooms | purple |

**No artwork was redrawn.** The tiles are still the original 1-bit bitmaps.
`draw_mfdb` blits them with GDI, and blitting a 1-bit source into a colour
destination makes GDI expand it — clear bits take the ink colour, set bits take
the paper colour. So each tile acts as a stencil that `pice_colors()` in
`icon.c` tints per feature. Room numbers are reset to black so they stay
legible.

This finishes work the original author started and abandoned; there were three
colour `TODO`s left in the tree (`mfdb.c`, `wind.c`, `bmp.c`). The disabled
block in `wind.c` called `GetScreenPlanes()`, which does not exist anywhere in
the source — the function is `GetNumPlanes()` — so it could never have
compiled, which is presumably why it was switched off.

**Saved and printed maps are still monochrome.** The image writers are 1-bit
only (`planes = 1; /* TODO: color formats */` in `bmp.c`), so rather than feed
them a colour bitmap and get garbage, `save_grafic()` renders the map again
into a fresh 1-bit bitmap and saves that. Output files are unchanged: a saved
BMP is still 368x224, 1 bpp, 2 colours, exactly as before.

### Zoom

The map opens magnified and can be rescaled live from the **View** menu:

| Action | Shortcut |
| --- | --- |
| Zoom In | `Ctrl` `+` |
| Zoom Out | `Ctrl` `-` |
| Actual Size | `Ctrl` `0` |
| Fit to Window | `Ctrl` `9` |

The setting persists as `[Config] Zoom` and is clamped to 1-16.

Why this is needed: at 1:1 a dungeon cell is **8 screen pixels**, so a full
52x26 map is only about 416x208 - sized for a 640x400 Atari ST. On a 2560-wide
display that is roughly a sixth of the screen. The default is now 3, and *Fit to
Window* picks the largest whole-number zoom at which the whole map still fits.

No artwork was resized to achieve this. The map is rendered once at 1:1 into an
offscreen bitmap and scaled by `StretchBlt` when painted (`mfdb.c:247`), so zoom
costs no extra memory. Because the tiles are 1-bit line art, whole-number
scaling is exact pixel replication and stays sharp; a fractional factor would
drop or double rows unevenly, which is why the zoom is integer only.

`ZOOM_MAX` is a safety bound rather than a display one: `draw_grafic()` in
`wind.c` multiplies the bitmap width by the zoom into a `_WORD` (16-bit), so an
unclamped profile value would overflow and corrupt the layout.

### Fullscreen

The app opens maximized, and opens its map windows maximized inside it. Turn it
off to get the original behaviour back:

```ini
[Config]
Fullscreen=0
```

The 1999 default was a fixed 768×537 window, stored under `[Windows]` as
`0=0, 1, {132, 132, 768, 537}, 0` — sized for the screens of the day. That
rectangle is still read and written as before; fullscreen just ignores it when
placing the main window, so switching back to `Fullscreen=0` restores the exact
old geometry rather than a lost one.

Note this shows **more of the map, not a bigger map**. On-screen tiles are drawn
at a fixed size — the `zoom` in `bmp.c` and `image.c` applies only when *saving*
an image, never to the display — so a larger window fits more of the dungeon in
view at the same tile size.

## A note on the code's history

This was ported from **Atari ST GEM**, which explains several things that look odd
in a Windows codebase:

- `my_lib/portab.h` line 342 still carries an `#if defined(__TOS__) || defined(atarist)`
  branch, with Pure C (`__PUREC__`) support alongside it.
- `my_lib/mfdb.h` is the GEM VDI *Memory Form Definition Block* — an Atari bitmap
  descriptor, reimplemented on top of Windows GDI in `my_lib/windows/mfdb.c`.
- `rsh/grafic.rc` line 2 announces itself as *"GEM resource RC output of grafic,
  created by ORCS 2.09"* — the `.rc` files were machine-converted from GEM
  resource files.
- The `.RWS` files in `rsh/` are Borland Resource Workshop projects, the one part
  of the workflow that used a Borland GUI tool rather than the command line.

`my_lib/` as a whole is the abstraction layer from that port, which is why the
GUI code calls `w_draw`/`w_dialog` rather than Win32 APIs directly.
