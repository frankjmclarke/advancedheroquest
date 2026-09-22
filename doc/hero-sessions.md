# Heroes and saved games

Open **Party > Heroes and Reserve...** (Alt+H) with a generated or loaded dungeon.
Choose Warrior, Dwarf, Elf, Wizard or Henchman, then **Add**. The party holds up
to 16 heroes, including those in reserve. Each class has its own coloured symbol.
Markers are drawn by the program, so they add no image files to the executable.

Name each hero and edit WS, BS, S, T, Sp, Br, Int, maximum Wounds, PV, current
Wounds and Fate. Starting numbers are editable suggestions, not official character
generation or enforced class rules. Apply saves edits; Close also applies them.
Zero Wounds crosses out the marker, but does not remove the hero.

## Moving and the reserve

Select a hero and choose **Place / Move**, then click an empty dungeon square.
The crosshair indicates placement mode. Right-click cancels it.
Once placed, drag the marker to another square, or click the marker and then its
destination. Drops outside the dungeon or onto another hero are rejected.
Movement allowance, routes, combat and healing remain the players' decisions.
The player window also prevents placement in unexplored areas.

**To reserve** removes one marker while keeping its record. **Party > Leave
Dungeon** returns everyone to reserve. **Next Dungeon with Current Party** rolls
another dungeon and keeps every hero's edited stats, with everyone initially in
reserve. It uses the currently selected campaign tables and map settings.
**Undo Hero Change** reverses the last edit, placement, addition or reserve action;
selecting it again reverses the undo. Undo does not span dungeon changes.

## Saving and recovery

**File > Save Game** (Ctrl+S) writes an `.hqg` file. **Save Game As** creates a
separate save; **Load Game** restores it. Saves include the exact dungeon layout,
room descriptions, party records, token positions and explored rooms. Opening
the saved dungeon does not require regenerating it from its campaign tables.
The save does not bundle those tables: select the desired quest through
**Options > Load Tables** when continuing into a different campaign.

Adding the first hero or saving a game starts a persistent session. Subsequent
hero changes, door reveals and new dungeons automatically write a separate
recovery file under `%LOCALAPPDATA%\HQ-Map`. This does not overwrite your named
save. Startup offers to resume recovery, and **File > Recover Last Session** can
open it later. The previous recovery is retained as a fallback if the newest
file is damaged. Normal saves are replaced only after the new file is fully
written and flushed. A failed dungeon generation cannot overwrite recovery.

Keep named saves for campaigns you want to retain: recovery holds the latest
session and its previous revision, not an unlimited history. Existing map/image
exports remain separate from saved games. Custom classes, equipment records and
custom resource counters are outside this first version.

## Development checks

Build with `build-msvc.bat`. In an x86 MSVC developer shell, run:

```
python tests/game-check.py
python tests/game-render-check.py
python tests/fog-check.py
python tests/room-contents-check.py
```

The game integration check exercises real generated maps, save round trips,
corrupt-file rejection, atomic replacement, recovery fallback, movement at
multiple zoom levels with scrolling, non-overlap, the native Party dialog,
reserve/undo, and carrying heroes into the next dungeon.

The rendering check opens real Win32 GM/player map windows, loads saved games
with placed and reserve heroes in both graphics modes, and checks empty room text.
