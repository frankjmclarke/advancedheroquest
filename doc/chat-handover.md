## Current update — 25 September 2026

The user requested publishing the previous branch and all unignored files to
GitHub main. Completed at `31e6436` (includes the staged v1.0 release archive).
New work is on `codex/campaign-character-packs`; keep it separate from main.

Implemented runtime character packs in `pack.c` / `pack.h`, JSON sources in
`data/packs/`, and `tools/compile-packs.py`. Each Sentinel quest declares
`;character-pack sentinel/characters.hqp`. Undeclared quests use the generated
fantasy fallback. Both fantasy heroes and all 64 monster profiles are now in
`data/packs/fantasy.json`; runtime packs, references and the embedded fallback
compile directly from that complete source. The duplicate transcription file
and its adapter have been removed. Legacy saves without pack metadata now use
the selected campaign pack, with fantasy as the no-selection fallback. Nine
fantasy hero definitions moved from C into the shared source format.

Sentinel has nine hero equipment presets, 34 enemy profiles including weapon
variants, and six individually identified Eldar leaders. Its table shorthand was
expanded into explicit creature names. All use the existing AHQ melee/ranged
rules; the user expressly rejected separate science-fiction or WWII rules.
No new combat mechanics were added.

Version 7 saves embed the full pack and retain party identity/class notes and
named-monster IDs. Versions 1–6 migrate to fantasy without guessing a theme.
Saved packs work without their source files, including unrevealed encounters.
Campaign Builder materializes the base pack into independent campaign copies
and rejects known incompatible monster-pack selections.

See `doc/character-packs.md` for format, limitations and tests. Build using
`build-msvc.bat`; run native checks with `tools/test-msvc.bat` and test basenames.
The new `pack` check covers all five decks, every listed encounter, native party
creation, no duplicate reveals, unique deaths, corruption, missing-source loads,
legacy migration and preserving heroes when switching themes. Existing game,
monster, combat, ranged, reference, builder, fog, rendering and recovery checks
are also relevant. Do not run package.bat over the local playable copy.

---

## Previous update — 23 September 2026

Branch: `codex/orthogonal-melee-combat`, based on main at `236476a`.
The user authorized implementation after creating this feature branch.
Both combat dialogs now use one Roll attack button for hit, damage (including
bonus dice) and calculation. Dice displays are read-only; Apply/Cancel remain.

Implemented native melee previews by dragging heroes/monsters onto orthogonal
enemies. `combat.inc` holds profile editing, adjacency checks, manual/generated
D12 handling, critical/fumble follow-ups and transactional Apply/Cancel. Undo
restores the whole exchange. `MELEE_PROFILE` lives in heroes and monsters;
`liste.c` extracts only exact unambiguous printed monster rows. Hero equipment
and hit rows now default from the supplied heroesStats.png for Warrior, Dwarf,
Elf and Wizard; other classes have explicitly labelled invented suggestions,
as authorized by the user. See doc/hero-weapons.md. Loading upgrades only wholly
blank hero melee profiles; existing entries and attributes remain unchanged. Saves are version
6 with the hero fired-this-turn flag and ranged profiles, movement/focus and opened doors; readers support versions
1–4. See the melee
section of `doc/hero-sessions.md` for usage and deliberately manual rules.

Ranged combat is implemented in `ranged.inc` (profiles, native preview, rolls,
post-fumble friendly-fire notice, transactional Apply) and `ranged-rules.inc` (range,
generous ray visibility, shortest-path movement and focused death zones).
Click hero then enemy, or drag: orthogonal adjacency routes to melee, all other
positions including diagonal neighbours route to ranged. A combined ranged roll
button rolls and previews attack/damage. `Party > Next Turn` resets movement for
both sides. No ammo/initiative enforcement. Heroes have one normal ranged attack per turn; melee free attacks remain unchanged. Focus clears on leaving
the zone or model removal. Other weapon movement and friendly-fire damage remain manual. Friendly fire
is mentioned only after a rolled fumble, in a simple message; no advance selector
or question. Ranged critical Toughness is halved rounding down. `map.c` now stores open-door bits in the fog
array; older saves infer open doors between explored areas. Tests additionally
include `tests/ranged-check.py`. See the guide for the full agreed behaviour.

Preserve these user decisions from the preceding feature work:
- Automatic recovery on startup, without a prompt or placeholder dungeon.
- Player View remains on top across next map, save/load, restart and placement.
- New maps have no placed heroes or old monsters; heroes go to reserve.
- Revealed-room monsters auto-place; surprise positioning is manual.
- Species colours, drag movement and double-click one-wound damage remain.
- Character monster deaths persist per continuing adventure, not globally.

Build with `build-msvc.bat`. Regression scripts now include `combat-check.py`,
`monster-check.py` and `view-check.py` as well as the original four listed below.
Keep the runnable `dist/HQ-Map/hq_map.exe` in sync after successful testing;
back up the previous executable and never run package.bat over local user data.

The earlier handover below is historical and some feature/version details are
superseded by this update and `doc/hero-sessions.md`.

---

# Chat handover — 22 September 2026

## Where to resume

The likely next topic is improving dungeon compactness through room-aware
corridor placement. This was discussed, but no algorithm change was requested
or implemented. Read `doc/dungeon-generation.md` before proposing changes.

The user asked whether corridors grow with room dimensions in mind. The answer:
they check their own footprints and prospective junction branches, but do not
reserve space for future rooms. Pending corridor extensions take priority over
expanding room exits. However, corridor doors cause attached rooms to be placed
immediately, so those rooms constrain later corridor growth. It is inaccurate
to say that all corridors are completed before any rooms appear.

Compactness currently emerges from direct attachment, fitting candidate
positions, smaller-room fallbacks, shortened corridors, simplified junctions
and doors connecting adjacent existing pieces. There is no compactness score,
global layout plan or final connectivity validation. Whether reserving room
space would improve the results is still an open design question.

## Repository and actual Git state

- Workspace: `C:\dev\hero2`; native Windows application in legacy C.
- Current branch: `codex/hero-tokens-and-saves`.
- Current HEAD: `f3d9efb` — `New feature: Hero Tokens plus load/save`.
- Local main: `f9581c4` — `Add HQ-Map promotional feature graphic`.
- The hero branch has no upstream shown by `git branch -vv`.
- No fetch was performed for this handover, so remote freshness is unverified.
- Earlier chat statements that the entire hero feature was uncommitted are
  outdated: the core feature is now in HEAD. Preserve that commit and the
  remaining local changes.

Immediately before this handover was created, the working tree contained:

```text
 M README.md
 M doc/hero-sessions.md
 M game-data.c
 M game.c
 M game.h
 M rsh/game.rc
 M rsh/game.rh
 M tests/game-check.py
?? doc/dungeon-generation.md
```

Those edits add four additional hero classes and generation documentation.
This handover is another new file. No commit, merge, push or release publication
was requested for the handover. Recheck Git status when resuming.

## Completed hero/session feature

- Compact Party dialog, Alt+H, with names and editable WS, BS, S, T, Sp, Br,
  Int, maximum/current Wounds, PV and Fate.
- Up to 16 heroes, including reserve; distinctive code-drawn class markers.
- Drag movement or select-and-click movement; no overlapping tokens.
- Place / Move closes the dialog and activates the map. Left-click places;
  right-click cancels. The player window prohibits placement in unexplored areas.
- Reserve individual heroes or return the whole party to reserve.
- Continue into another dungeon, retaining hero records and resetting positions.
- Undo the last hero change; selecting Undo again reverses the undo.
- Save Game, Save Game As, Load Game and recovery. Ctrl+S saves the game.
- `.hqg` saves contain the actual map, generated room text, party, positions and
  explored rooms. They do not regenerate the dungeon from a seed.
- Atomic save replacement; autosave and a previous recovery generation under
  `%LOCALAPPDATA%\HQ-Map`. Named saves remain separate from recovery.
- Saved games do not bundle campaign tables. Next dungeon uses the currently
  selected campaign and generation settings.

Design preference: a flexible digital record, with manual adjudication rather
than automatic movement/combat rules. No custom resource counters yet. Keep the
interface compact. Custom class creation remains future work.

### Bugs fixed during testing

1. Placement clicks could be consumed by the legacy window activation handler
   (`MA_ACTIVATEANDEAT`). The map subclass now preserves the click, returns focus
   after the Party dialog and resets the crosshair after successful placement.
2. Loading crashed during graphic-map creation. The decoder turned absent
   piece text into an allocated empty list, and the renderer dereferenced its
   null first entry while reading a room number. The decoder now preserves NULL
   for zero text entries; both number-rendering paths tolerate missing/blank
   text. The real-window regression reproduced the crash before the fix.

The user explicitly confirmed save/load now works. The chat has no explicit
confirmation of successful manual placement after its fix, although automated
placement and movement tests pass.

## Additional classes (local changes)

Source supplied by the user:
`C:\Users\qazrr\Downloads\ahqHeros.pdf`, four pages. Embedded font encoding made
text extraction garbled; all four rendered pages were read visually.

There are now nine choices, preserving existing saved class IDs 0–4:

```text
0 Warrior       1 Dwarf       2 Elf       3 Wizard       4 Henchman
5 Warrior Priest             6 Rogue     7 Fighter      8 Mage
```

The PDF describes adjustments to a racial/rolled profile, not fixed full stat
blocks. New presets use the existing Warrior suggestion as a human base:

- Warrior Priest: Strength -1, Bravery +1.
- Rogue: WS -1, Bravery -1, BS +1, Speed +1.
- Fighter: base already meets its WS/Strength thresholds, so no adjustment.
- Mage: WS -1 and Intelligence +1.

Every stat remains editable. A new **Class rules** button summarizes each
class's abilities and restrictions from the PDF. Healing, trap skills, rerolls,
spells and optional berserker rules are references, not automated actions or
tracked counters. Fate must not be mistaken for remaining healing/reroll uses.
The optional berserker can be identified in the name with manually edited stats.
Existing saves retain their class identities; new-class saves need the new EXE.

## Important files

| Files | Purpose |
| --- | --- |
| `doc/dungeon-generation.md` | Detailed generator explanation, flowchart, pseudocode, source links and limitations |
| `doc/hero-sessions.md` | Party, movement, save/recovery and class usage guide |
| `game.h`, `game-data.c` | Hero model, defaults/class references, bounded versioned save codec |
| `game.c` | Party UI, markers, mouse subclass, session lifecycle, save/load and recovery |
| `rsh/game.rc`, `rsh/game.rh`, `rsh/menu.rc` | Native dialog, IDs and menus |
| `main.c` | Menu dispatch, generation wrapper, seeds, retry and game lifecycle |
| `wind.c` | GM/player windows, fog, drawing and room contents |
| `makemap.c` | Entrance and corridor/room growth loop |
| `test.c` | Placement fitting, candidate searches and fallbacks; production code despite its name |
| `set.c`, `move.c` | Inserting pieces/contents/queued continuations and relative geometry |
| `map.c`, `pice.c`, `queue.c` | Grid, piece storage, map restore and two FIFO queues |
| `table.c`, `rolldice.c`, `random.c`, `stairs.c` | Tables, runtime rolls, seeded randomness and stair postprocessing |

Generation details worth preserving: initial entrance attempts two corridor
sections and a T-junction; sections are 2×5 squares; normal rooms are 5×5 and
large/lair/quest rooms 5×10. Large room fitting can fall back to Hazard, changing
contents as well as dimensions. Startup retries are capped at 50; interactive
automatic retries have no fixed cap. Same-seed reproduction also depends on
tables, settings and build/random-call order.

## Build, testing and runnable copy

Build with `build-msvc.bat` using Visual Studio 2022 x86 tools. Two existing
resource warnings about duplicate dialog control ID 2 are expected.

Run in an x86 MSVC developer shell:

```text
python tests/game-check.py
python tests/game-render-check.py
python tests/fog-check.py
python tests/room-contents-check.py
```

All four passed after the load-crash fix. The first two passed again after the
additional classes. New-class tests cover creation through the actual dialog,
expected stat adjustments, class IDs surviving serialization and invalid IDs
being rejected. The rendering regression opens actual Win32 GM/player windows
in both graphics modes. The Party dialog layout was visually checked.
Generation documentation source links and `git diff --check` were checked.

The current runnable application is `dist\HQ-Map\hq_map.exe`, matching the last
built `bin\hq_map.exe`. It includes all nine classes; size at handover is
2,216,960 bytes. Restarting the running application is necessary to use updates.
Previous executables were preserved in the same directory with names such as
`hq_map.before-extra-classes.exe`, `hq_map.before-load-fix.exe`,
`hq_map.before-placement-fix.exe` and `hq_map.before-hero-sessions.exe`.

Do not run `package.bat` merely to update the executable: its staging operations
can remove local maps/profile data. Updates in this chat renamed the old EXE to
a unique backup, copied the new EXE and verified matching hashes. User data was
preserved; running instances were not terminated.

Existing C/RC files generally use CP1252 and CRLF; preserve encoding when
editing. Git operations may need elevated tool permission for `.git`. Warnings
about access to the global Git ignore file occurred during read-only checks.

## Earlier application context

The application already has enhanced board-game tile graphics, a toggle back
to legacy bitmaps, room-content/monster-stat inspection, player fog of war
(Alt+W), and a campaign builder that combines existing tables into a new folder.
Enhanced graphics are the default. Doors use custom artwork with red secret
doors. Preserve these features when changing generation or rendering.

The repository/release is `https://github.com/frankjmclarke/advancedheroquest`,
with the previously discussed release tagged `v1.0`. This session's current
changes have not been published to that release by the assistant.

## Suggested opening for the next chat

“Read doc/chat-handover.md and doc/dungeon-generation.md. I want to discuss
whether room-aware corridor placement could make the dungeon more compact.
Inspect the current generator before proposing changes.”

Heroes who have applied a ranged attack see an already-fired explanation and a
**Next Turn** button in the ranged dialog. Applied misses and fumbles also use
the shot; cancelled previews do not. Next Turn immediately resets all movement
and hero ranged attacks, even if the dialog is later cancelled; Undo can restore
the prior turn. The fired flag is saved and recovered; older saves default to
not fired. Melee free attacks and their chains are unaffected.

The ranged dialog always displays Next Turn, including when the bow/crossbow
movement warning says to use it. It is disabled only while a rolled preview
awaits Apply/Cancel.
