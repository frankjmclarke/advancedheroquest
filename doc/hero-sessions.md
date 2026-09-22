# Heroes and saved games

Open **Party > Heroes and Reserve...** (Alt+H) with a generated or loaded dungeon.
Choose Warrior, Dwarf, Elf, Wizard, Henchman, Warrior Priest, Rogue, Fighter or
Mage, then **Add**. The party holds up
to 16 heroes, including those in reserve. Each class has its own coloured symbol.
Markers are drawn by the program, so they add no image files to the executable.

Name each hero and edit WS, BS, S, T, Sp, Br, Int, maximum Wounds, PV, current
Wounds and Fate. Starting numbers are editable suggestions, not official character
generation or enforced class rules. Apply saves edits; Close also applies them.
Zero Wounds crosses out a hero marker. A Henchman at zero Wounds is dead: its
model is removed and its party record is retained.

## Moving and the reserve

Select a hero and choose **Place / Move**, then click an empty dungeon square.
The crosshair indicates placement mode. Right-click cancels it.
Once placed, drag the marker to another square, or click the marker and then its
destination. Drops outside the dungeon or onto another token are rejected.
Movement allowance, routes, combat and healing remain the players' decisions.
The player window also prevents placement in unexplored areas. **Place / Move**
returns to whichever map view you were using: Player View remains on top with
fog restrictions intact, and the GM view stays selected when used instead.

**To reserve** removes one marker while keeping its record. **Party > Leave
Dungeon** returns everyone to reserve. **Next Dungeon with Current Party** rolls
another dungeon and keeps every hero's edited stats, with everyone initially in
reserve. It uses the currently selected campaign tables and map settings.
**Undo Token Change** reverses the last edit, placement, addition or reserve action;
selecting it again reverses the undo. Undo does not span dungeon changes.

## Saving and recovery

**File > Save Game** (Ctrl+S) writes an `.hqg` file. **Save Game As** creates a
separate save; **Load Game** restores it. Saves include the exact dungeon layout,
room descriptions, party records, token positions and explored rooms. Opening
the saved dungeon does not require regenerating it from its campaign tables.
The save does not bundle those tables: select the desired quest through
**Options > Load Tables** when continuing into a different campaign.

A generated dungeon starts a persistent session with an empty board. Hero changes, monster changes, door reveals and new dungeons write a separate
recovery file under `%LOCALAPPDATA%\HQ-Map`. This does not overwrite your named
save. Startup automatically resumes recovery without asking. **File > Recover Last
Session** can reload it later. The previous recovery is retained as a fallback if the newest
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
python tests/monster-check.py
python tests/view-check.py
python tests/fog-check.py
python tests/room-contents-check.py
```

The game integration check exercises real generated maps, save round trips,
corrupt-file rejection, atomic replacement, recovery fallback, movement at
multiple zoom levels with scrolling, non-overlap, the native Party dialog,
reserve/undo, and carrying heroes into the next dungeon.

The rendering check opens real Win32 GM/player map windows, loads saved games
with placed and reserve heroes in both graphics modes, and checks empty room text.

## Additional classes

Warrior Priest, Rogue, Fighter and Mage follow the class adjustments described
in `ahqHeros.pdf` (pages 1-4). The new presets use the existing Warrior suggestion
as a human base. Priest reduces Strength by 1 and raises Bravery by 1. Rogue
reduces WS and Bravery by 1 and raises BS and Speed by 1. The Fighter already
meets the WS/Strength thresholds. Mage trades one WS for one Intelligence.
These remain suggestions: adjust the stats for a rolled character or another race.

**Class rules** describes the selected hero's abilities and restrictions,
including healing, trap skills, attack rerolls, optional berserker adjustments,
and magic. Abilities are applied manually. There are no new resource counters;
Fate is not a counter for remaining healing spells or rerolls. To distinguish a
berserker, include that in the hero's name and edit the relevant stats.

Existing saves retain the original five class identities. Saves containing new
classes require this updated executable.

## Monster encounters

Opening a door in the player view automatically creates recognized monsters in
newly revealed rooms. New maps start with no hero or monster tokens on the board. Heroes remain in
reserve; visible corridor contents do not automatically create tokens.
Tokens use circular markers with a contrasting border and individual numbers.
Monster types have consistent colours: skeletons are bone-coloured, zombies olive,
orcs green, and goblins teal. Other common creatures have their own colours; custom
names receive a repeatable colour. Colours are derived from names, including when
loading existing saves.
Placement spreads them across free squares, avoiding doors and their immediate
surroundings while space permits. No token shares a square with another token.
Revealing the same room again does not recreate dead or moved monsters.

**Drag** a monster to move it to any revealed, unoccupied dungeon square.
**Double-click** it to inflict exactly one wound. At zero Wounds its model is
removed immediately; its dead record remains in the encounter. Monster labels
show remaining Wounds at larger zoom levels. Surprise, movement allowances,
combat and healing are manually adjudicated; there is no surprise dialog.
**Party > Undo Token Change** reverses the most recent hero or monster change,
including damage, death and the character death record. Choosing Undo again
reverses the undo. Loading a save, revealing a new encounter or changing dungeons
clears Undo; Undo history itself is not saved.

Open **Party > Monsters...**, or right-click a monster, to edit the roster.
Select a monster, edit its name and Wounds, then choose **Apply edits**. Increasing
remaining Wounds heals it. **Add new** creates another monster using the entered
fields in the selected revealed room. **Remove** sets Wounds to zero and follows
the death rule. **Room contents** shows the original text and reference profiles.
Edits are applied only with **Apply edits** (or **Add new**); Close discards
unapplied field changes.

Monsters that cannot fit stay in the roster as **unplaced**. After freeing a
square, select the monster and choose **Place in room**. Room annotations and the
roster show unplaced counts. The current limits are 512 monster records per
dungeon (including dead ones) and 256 character death identities per adventure.
Encounter quantities beyond that capacity are flagged for roster review.

Automatic rosters use the existing exact monster aliases and reference profiles
for counts and starting Wounds. Alternatives containing "or", ambiguous profiles,
unsupported statistics and encounter-like unknown names require manual review;
the app does not guess between profiles. Such rooms are marked **review** on the
map and in the room selector. Review the original contents and add or correct
monsters manually. The review marker remains as a reminder that the source could
not be interpreted completely. Free-form narrative is not a structured monster
roster, so unusual campaign text may require manual additions. No dice are rerolled
and original room descriptions are preserved.

### Character monsters and adventures

Check **Unique character** for an individual character monster. Its name is its
persistent identity: use the same individual encounter name whenever it appears,
not a generic species name. Names are normalized for case, whitespace and hyphens;
automatically recognized names also use their canonical singular alias. Character
status is assigned manually; a generic champion profile is not assumed unique.
Only one living token can use a character's identity.

Killing a character records that identity for the continuing adventure. Future
rosters skip recognized monsters with that identity, and the editor rejects
adding or healing a character recorded as dead. Undo can reverse an accidental
killing. Original room prose is retained as a reference, even when its character
has been suppressed from the roster.

**Next Dungeon with Current Party** clears the old monster encounters but retains
character deaths. Deaths belong to the saved adventure, not a global file: loading
another adventure replaces the ledger. Keep separate named saves for separate adventures. Save As makes another copy of the same adventure, including its deaths.

Saved games and recovery include monster profiles, remaining Wounds, positions,
unplaced/dead records, already-populated encounters and character deaths. The new
version-3 format reads existing version-1 hero saves and version-2 monster saves.
Revealed encounters in version-1 saves are populated on load. New saves require
this updated executable.

The monster regression checks automatic counts and profiles, ambiguity handling,
dragging and double-clicks through the map handler, collisions, editor changes,
Undo of character deaths, overflow, serialization, old-save compatibility and
continuation into another dungeon. The rendering regression also opens saved
monster tokens in both native GM/player graphics modes.

## Remembering Player View (fog of war)

The most recently selected map view is remembered. If Player View is on top,
**Next Map** and **Next Dungeon with Current Party** reopen Player View on top
of the new dungeon, with that dungeon's exploration reset as usual.

Named saves and recovery record the selected map view as well as explored rooms.
Loading a game saved in Player View reopens it on top; a game saved in the GM
view reopens the GM map on top. Switching map views also updates recovery, even
when no tokens or doors have changed. Older version-1/version-2 saves have no
view preference, so loading them retains the current view choice.

On normal exit, the last-used view is stored in the program settings, including
when automatic option saving is disabled. Restarting restores that view. Restoring
other windows cannot accidentally cover Player View with the GM map. Clicking the
GM map makes it the remembered choice again. This does not change fog visibility
or the exploration recorded in a saved game.

`tests/view-check.py` exercises the actual Next Map menu, both save/load directions,
view-only recovery updates, shutdown/settings restoration, automatic recovery and native window order.
It isolates profile I/O so it does not modify the user's settings.

Startup restores the latest usable recovery automatically, before displaying any
new dungeon. If the latest copy is damaged, it tries the previous generation.
Only when no usable recovery exists does startup generate a dungeon. To replace
the recovered dungeon, use **Next Map** or **Next Dungeon with Current Party**.
Window-restoration callbacks do not reload an already restored session. Startup
recovery has no confirmation or save-before-recovery dialog.
