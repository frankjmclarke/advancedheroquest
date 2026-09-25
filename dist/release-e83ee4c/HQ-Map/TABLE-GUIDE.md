# Guide to creating and editing table files

### Making new adventure campaigns for HQ-Map

HQ-Map has no dungeons built into it. Every dungeon it draws is rolled from
*tables* — plain text files describing what to roll and what each result means.
This guide covers writing your own.

Everything here was taken from the program's own parser, so these are the rules
actually enforced rather than a description of how it ought to work.

---

## Contents

1. [What a campaign is made of](#1-what-a-campaign-is-made-of)
2. [Quick start: copy a campaign](#2-quick-start-copy-a-campaign)
3. [Table file syntax](#3-table-file-syntax)
4. [The seven tables you must provide](#4-the-seven-tables-you-must-provide)
5. [Room content tables](#5-room-content-tables)
6. [Tables campaigns define for themselves](#6-tables-campaigns-define-for-themselves)
7. [A worked example](#7-a-worked-example)
8. [Checking your work](#8-checking-your-work)
9. [Error messages, and what they mean](#9-error-messages-and-what-they-mean)
10. [Things that catch people out](#10-things-that-catch-people-out)

---

## 1. What a campaign is made of

Two parts:

```
tables\
    mycampaign.tab        <- the campaign. This is what appears in the menu.
    mycampaign\           <- its parts. Never chosen directly.
        room.tab
        monsters.tab
```

The **campaign file** sits directly in `tables\`. It contains almost nothing
except a title and a list of `include` lines. The **folder** holds the actual
tables.

That split matters. *Options → Load Tables* lists only the `.tab` files directly
in `tables\`, because a file inside a campaign folder is one piece of a campaign
and cannot stand on its own — `room.tab` has the dungeon layout but no monsters,
`monsters.tab` has monsters but no layout. Only the campaign file pulls the
whole set together.

A real one, `tables\amulett.tab`, in full:

```
#The Quest for the Shattered Amulett
#-----------------------------------
#

define ROOM-FURNISH

include amulett\room.tab
include standard\furnish.tab
include standard\hazard.tab
include standard\treasure.tab
include standard\trap.tab
include amulett\skaven.tab
```

Note it borrows four files from `standard\`. Furnishings, hazards, treasure and
traps are much the same in most campaigns, so most campaigns reuse them and
write only their own layout and monsters.

**The campaign's name is the first comment line.** `#The Quest for the Shattered
Amulett` is what shows in the menu and the window title. Rename a campaign by
editing that line.

**Your campaign appears automatically.** The menu is built by scanning `tables\`
each time it opens. Drop a file in and it is there — nothing to register, no
rebuild.

---

## 2. Quick start: copy a campaign

The fastest route to a working campaign is to start from one that already works.

1. Copy `tables\amulett\` to `tables\mycampaign\`
2. Copy `tables\amulett.tab` to `tables\mycampaign.tab`
3. Edit `tables\mycampaign.tab`: change the title, and point the two `amulett\`
   includes at `mycampaign\`

```
#The Halls of Something Unpleasant
#---------------------------------
#

define ROOM-FURNISH

include mycampaign\room.tab
include standard\furnish.tab
include standard\hazard.tab
include standard\treasure.tab
include standard\trap.tab
include mycampaign\skaven.tab
```

4. Start HQ-Map, *Options → Load Tables*, choose your campaign.

It now rolls dungeons identical to the original. Edit
`tables\mycampaign\skaven.tab` to change what lives there, and
`tables\mycampaign\room.tab` to change how the dungeon is shaped.

---

## 3. Table file syntax

### Comments and the title

A `#` at the start of a line makes a comment. In the **campaign file**, comment
lines are also the campaign's name — the first one that is not simply a rule of
dashes becomes the title.

```
#The Halls of Something Unpleasant
#---------------------------------
#
```

### A table

```
Passage-Length
(   1D12
    1-2     Section1
    3-8     Section2
    9-12    Section3
)
```

- A line holding **just a name** begins a table.
- The next line **opens a bracket** and states the dice.
- Then one row per outcome.
- A closing bracket ends it.

`(` `[` `{` all open a table and `)` `]` `}` all close one. The parser does not
insist they match, though matching them is obviously better.

### Dice

`1D12` is one twelve-sided die, `2D12` is two. The form is
`<how many>D<how many sides>`.

**The rows must cover every possible roll, in order, with no gaps.** With `1D12`
that means 1 to 12. With `2D12` it means **2 to 24**, because two dice cannot
total less than 2:

```
Passage-End
(   2D12
    2-3     T-Junction
    4-8     Dead-End
    9-11    Right-Turn
   ...
   22-24    Stairs-Out
)
```

Each row starts at the previous row's end plus one, and the last row must reach
the maximum. Stop short and you get *"Table incomplete (last dice value too
small)"*.

Dice need not be six or twelve sided. `1D15` is used where the author wanted an
outcome rarer than one in twelve.

### What a row can produce

Three kinds of result, combinable with `&`.

**A keyword** the program understands — a room type, a piece of dungeon
furniture, a monster marker. These are fixed; you cannot invent new ones.
Section 4 lists which are allowed where.

```
    1-6     Normal
    7-7     Chest
```

**Text in quotes**, printed in the monster list exactly as written:

```
    1-2     "4 Goblins (15 Gold Crowns)"
```

**Another table**, written with empty brackets, rolled in turn:

```
    1-12    Room-Furnish()
```

**Combined with `&`:**

```
    1-4     Stairs_and_Chest & "Stairs Down" & Quest-Rooms-Matrix() & Treasure-Chest() & Hidden-Treasure()
```

That single row places a stairs-and-chest marker, prints "Stairs Down", then
rolls three further tables and prints all their results together.

A row may also contain a table written **in place**, for when naming it is not
worth the trouble:

```
    1-12    Room-Furnish() &
            (   1D12
                1-4     Hidden-Treasure()
                5-8     Room-Trap()
                9-12    Hidden-Treasure() & Room-Trap()
            )
```

### include

```
include mycampaign\room.tab
```

Paths are relative to `tables\` and use a backslash. Includes may nest.

### define, if, else, endif

```
define ROOM-FURNISH

if ROOM-FURNISH
    ...
else
    ...
endif
```

`if !SYMBOL` tests the other way. This is how `standard\furnish.tab` supplies
real furniture when a campaign asks for it and nothing when it does not — which
is why campaign files say `define ROOM-FURNISH` before including it.

### Names

Table names may use letters, digits, `-`, `_` and `.`, and are matched without
regard to case. Two tables with the same name is an error, so a campaign that
includes `standard\treasure.tab` must not also define `Hidden-Treasure` itself.

---

## 4. The seven tables you must provide

Every campaign must end up with these seven or it will not load. They decide the
shape of the dungeon. Each accepts **only** the keywords listed.

| Table | What it decides | Allowed results |
| --- | --- | --- |
| `Passage-Length` | how far a corridor runs | `Section0` `Section1` `Section2` `Section3` `Nothing` `None` |
| `Passage-End` | what a corridor ends in | `Dead-End` `Left-Turn` `Right-Turn` `T-Junction` `Corner` `Stairs-Down` `Stairs-Out` |
| `Passage-Feature` | what is found along a corridor | `Door1` `Door2` `Wandering-Monsters` `Nothing` `None` |
| `Room-Type` | what kind of room | `Small` `Normal` `Hazard` `Large` `Lair` `Quest` `Big` `Merscha` |
| `Room-Doors` | how many ways out of a room | `Door1` `Door2` `Nothing` `None` |
| `Room-or-Passage` | whether a door opens to a room or a corridor | `Room` `Passage` |
| `Secret-Doors` | whether a secret door is present | `Door1` `Nothing` `None` |

Anything else in one of these gives *"Table contains a key name that is not
allowed!"*.

These normally live in your campaign's `room.tab`. `tables\standard\room.tab` is
a plain version to copy as a starting point.

**Two of these decide whether a dungeon is usable at all.** `Passage-End`, and
whichever room table places `"Stairs Down"`, control how often a map has a way
down. If stairs are too rare and *Only show maps with stairs down* is ticked,
the program keeps rolling and rolling looking for a usable map.

---

## 5. Room content tables

For each room type your `Room-Type` can produce, define a table of the same name
saying what is in such a room. These are **optional** — leave `Merscha`
undefined and you simply never get a Merscha room.

`Small` `Normal` `Hazard` `Large` `Lair` `Quest` `Big` `Merscha`

They take the furniture and creature keywords: `Chest` `Tomb` `Throne` `Statue`
`Pool` `Well` `Bridge` `Chasm` `Grate` `Trapdoor` `Rockfall` `Slime` `Mould`
`Mushrooms` `Cess-Pit` `Moths` `Rats` `Bats` `Apparition` `Magic-Circle`
`Fireplace` `Bookcase` `Cupboard` `Table` `Rack` `Weapons-Rack`
`Stairs_and_Chest` `Maiden` `Witch` `Man-at-Arms` `Rogue` `Wandering-Monsters`
`Nothing` `None`.

Note `Stairs_and_Chest` is spelled with underscores; every other multi-word
keyword uses hyphens.

The usual pattern is a single line handing off to other tables:

```
Normal
(   1D12
    1-12    Room-Furnish() & Hidden-Treasure()
)

Lair
(   1D12
    1-12    Chest & Lairs-Matrix() & Treasure-Chest() & Hidden-Treasure()
)

Quest
(   1D12
    1-4     Stairs_and_Chest & "Stairs Down" & Quest-Rooms-Matrix() & Treasure-Chest() & Hidden-Treasure()
    5-12    Chest & Quest-Rooms-Matrix() & Treasure-Chest() & Hidden-Treasure()
)
```

There are also corridor content tables, all optional, taking only `Nothing` and
`None`: `Passage` `Dead-End` `Corner` `T-Junction` `Right-Turn` `Left-Turn`
`Stairs-Down` `Stairs-Out`.

---

## 6. Tables campaigns define for themselves

These names mean nothing to the program. They are a convention every shipped
campaign follows, and they are where your campaign's character actually lives.

| Name | Usually holds | Usually in |
| --- | --- | --- |
| `Wandering-Monsters` | what you meet in corridors | your monster file |
| `Lairs-Matrix` | what occupies a lair, with gold | your monster file |
| `Quest-Rooms-Matrix` | what guards a quest room — tougher, richer | your monster file |
| `Specialist-<something>` | a rare champion rolled on top of a group | your monster file |
| `Hidden-Treasure` | loose treasure | `standard\treasure.tab` |
| `Treasure-Chest` | chest contents | `standard\treasure.tab` |
| `Room-Furnish` | furniture | `standard\furnish.tab` |
| `Hazards-Matrix` | hazard room contents | `standard\hazard.tab` |
| `Room-Trap` | traps | `standard\trap.tab` |

**A monster file is the shortest route to a new campaign.** Copy one, rewrite
the creature lists, and you have a new adventure using proven layout rules:

```
Wandering-Monsters
(   1D12
    1-2     "2 Skaven Warriors (20 Gold Crowns)"
    3-4     "1 Skaven Sentry (20 Gold Crowns)"
   ...
)

Lairs-Matrix
(   1D12
    1-1     "4 Skaven Warriors (40 Gold Crowns)"
   ...
    9-9     "6 Skaven Warriors, 1 Champion (80 Gold Crowns)" & Specialist-Skaven()
   ...
)

Quest-Rooms-Matrix
(   1D12
    1-1     "2 Skaven Champions, 1 Skaven Warlord (100 Gold Crowns)"
   ...
)
```

If your `Room-Type` can roll `Lair` or `Quest`, your monster file **must**
define `Lairs-Matrix` and `Quest-Rooms-Matrix`, because the room tables call
them. Forgetting is the commonest way to end up with a campaign that will not
load.

---

## 7. A worked example

A small campaign with one kind of enemy. Three files.

**`tables\crypt.tab`**

```
#The Crypt of Grey Hands
#-----------------------
#

define ROOM-FURNISH

include crypt\room.tab
include standard\furnish.tab
include standard\hazard.tab
include standard\treasure.tab
include standard\trap.tab
include crypt\undead.tab
```

**`tables\crypt\room.tab`** — copy `tables\standard\room.tab` and adjust. It
already provides all seven required tables plus `Normal`, `Hazard`, `Lair` and
`Quest`. To make the crypt more dangerous, give it more monsters and more lairs:

```
Passage-Feature
(   1D15
    1-1     Door1
    2-10    Wandering-Monsters
   11-15    Nothing
)

Room-Type
(   1D12
    1-5     Normal
    6-7     Hazard
    8-10    Lair
   11-12    Quest
)
```

**`tables\crypt\undead.tab`** — the campaign's character:

```
Wandering-Monsters
(   1D12
    1-3     "3 Skeletons (20 Gold Crowns)"
    4-6     "2 Zombies (20 Gold Crowns)"
    7-9     "4 Skeletons (30 Gold Crowns)"
   10-11    "1 Wight (40 Gold Crowns)"
   12-12    "2 Ghouls (40 Gold Crowns)"
)

Lairs-Matrix
(   1D12
    1-4     "6 Skeletons (60 Gold Crowns)"
    5-8     "4 Zombies (60 Gold Crowns)"
    9-10    "2 Ghouls, 2 Skeletons (80 Gold Crowns)"
   11-12    "1 Wight, 3 Skeletons (90 Gold Crowns)"
)

Quest-Rooms-Matrix
(   1D12
    1-6     "1 Wight, 6 Skeletons (120 Gold Crowns)"
    7-10    "2 Wights, 4 Zombies (140 Gold Crowns)"
   11-12    "1 Vampire, 4 Ghouls (200 Gold Crowns)"
)
```

Check it, then load it:

```
python tools\tabcheck.py
```

---

## 8. Checking your work

From the folder HQ-Map was built in:

```
python tools\tabcheck.py
```

It walks every campaign, follows the includes, and reports

- includes that do not resolve
- any of the seven required tables missing
- `Name()` calls with no table behind them

then lists files inside campaign folders that no campaign includes. Those are
usually harmless, but a whole folder appearing there means that content is
unreachable.

It follows `if`/`else`/`endif` the way the program does, so a reference in a
branch that is never taken is not reported.

The program also writes an **`errors.txt`** into the campaign folder whenever a
load fails, listing each problem with its file and line.

---

## 9. Error messages, and what they mean

| Message | Cause |
| --- | --- |
| `Reference table not available` | One of the seven required tables is missing. Usually you loaded a file from inside a campaign folder rather than the campaign itself, or forgot to include your `room.tab`. |
| `Table not available` | Something calls `Name()` and no table by that name is defined. Check the spelling, and check the file defining it is included. |
| `Table contains a key name that is not allowed!` | A keyword used where it is not permitted — see section 4. |
| `Table incomplete (last dice value too small)` | The rows do not reach the highest possible roll. `2D12` must reach 24. |
| `Dice value too small` / `too large` | A row's range lies outside what the dice can roll, or the rows are out of order. |
| `Table name already exists` | Two tables with the same name, often from including two files that both define it. |
| `Table probably contains recursion!` | A table reaches itself, directly or through others, with no way out. |
| `Missing dice type` / `Invalid number of dice` / `Missing 'D'` | The line after the opening bracket is not of the form `1D12`. |
| `Missing end of table (end of file)` | A closing bracket is missing. |
| `Missing '-' character` | A range needs the form `1-2`, even for a single value: `12-12`. |
| `Cannot open include file` | An `include` path is wrong. Paths are relative to `tables\`. |
| `Too many nested 'Include' directives` | Includes nested too deeply, or including one another in a loop. |

---

## 10. Things that catch people out

**Load the campaign, not its parts.** Choosing `room1.tab` or `skaven.tab` from
inside a campaign folder cannot work. This is why the menu lists only whole
campaigns.

**Single values still need a range.** Write `12-12`, not `12`.

**`2D12` starts at 2.** Any table on two dice must cover 2 to 24, not 1 to 24.
Two dice cannot roll 1.

**Define `Lairs-Matrix` and `Quest-Rooms-Matrix`** if your `Room-Type` can roll
`Lair` or `Quest`.

**Do not define a table twice.** If you include `standard\treasure.tab`, do not
also write your own `Hidden-Treasure`.

**Give stairs down a decent chance.** If they are too rare, map generation
spends a long time hunting for a usable dungeon.

**Two campaigns may share a title.** The menu then shows the file name after it
to tell them apart, as it does for `sonne1` and `sonne2`.

**Keep a working copy.** Loading a broken campaign does not lose the one you
had — the program puts it back — but a copy of the file you are editing costs
nothing.
