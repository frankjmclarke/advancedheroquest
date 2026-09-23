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
version-6 format reads earlier version-1 through version-5 saves.
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


## Orthogonal hand-to-hand combat

In **Party → Heroes and Reserve**, select a hero and choose **Weapon / combat**.
New heroes have editable equipment presets from the supplied hero cards, with
labelled suggestions for other classes. Completely blank saved profiles receive
the same defaults on load. See [Hero weapon presets](hero-weapons.md) for the
source values and invented suggestions. You can edit the weapon name, number
of damage D12s, and the twelve required-hit values against target Weapon Skill
1–12 from the hero's sheet. Zero means unconfigured.
Changing WS does not infer or recalculate the hit table. Weapon bonuses, armour
and other adjustments must already be reflected in the profile you enter.

Newly revealed, recognized monsters receive their exact printed melee hit row
and damage dice automatically. **Party → Monsters → Melee profile** also lets
you edit their WS, Toughness and melee profile, or load **Use monster reference**.
Unknown or ambiguous references require manual entry. The generic label
“Printed melee profile” identifies the reference's damage pool; it does not
infer a particular equipped weapon. Old saves can load the reference into the
combat preview without changing the saved monster until Apply.

Drag a hero onto a living monster in the square immediately above, below, left
or right to open combat. Dragging a monster onto an adjacent hero works too.
Both tokens stay in their original squares. Diagonal and distant targets now
open ranged combat instead. Unrevealed targets and attacks through walls or
closed doors are rejected; opened connecting doors are supported.
Dragging to an empty square still moves the token. Both GM and Player View use
the same combat workflow and retain the selected view.

1. Check the attacker, weapon, required hit and target's WS/T/Wounds. Profile
   buttons allow corrections in the draft before a roll is calculated.
2. Choose **Roll attack**. It rolls the hit die, all required damage dice and
   bonus dice from twelves, then calculates the result in one action. The dice
   and result are displayed for review. A natural 1 fumbles; a natural 12
   grants a free attack.
3. **Take free attack** continues
   with the attacker after a critical, or the defender after a fumble, provided
   both remain alive. These attacks share the same preview and Undo operation.
   Players adjudicate free-attack eligibility and special rules; the supplied
   rule extract does not include the complete Free Attacks section.
4. **Apply** commits the previewed results. **Cancel** discards the entire draft,
   including profile edits. Starting an optional free attack and then choosing
   Apply commits only the attacks already calculated. The log shows every
   calculated roll and wound change.

A killed monster or Henchman is removed; unique monster deaths are recorded for
the continuing adventure. Heroes at zero Wounds retain the existing defeated
marker for manual adjudication. **Undo Token Change** reverses the
whole applied exchange, profiles and character death included.

Combat does not enforce attack allowances per turn, class weapon restrictions,
long-reach exceptions, ammunition or special abilities. Ranged eligibility and
normal orthogonal death zones are handled as described below. WS outside the
printed table's 1–12 range requires manual adjudication. A preview is bounded to
32 attacks and 512 damage dice per attack; further attacks can be adjudicated
separately. Profiles persist through save/load, automatic recovery and the next
dungeon. Version-5 saves require the updated executable.

`tests/combat-check.py` checks printed lookup, native dialogs and drag routing in
both views, adjacency and walls/doors, manual and generated dice, damage
explosions, criticals/fumbles, draft isolation, Apply/Undo, deaths and profiles in
saved sessions. Existing save tests also construct actual version-1/2/3 files.


## Ranged combat and automatic targeting

Click a hero, then an enemy, or drag the hero onto the enemy. Selecting a monster
and then a hero also works. **Orthogonal neighbours use melee; every other
position uses ranged combat**, including diagonally adjacent targets. An attack
does not move either figure. Right-click empty map space to cancel target
selection before selecting an opposing figure for ordinary movement.

Use **Ranged weapon** in the hero or monster editor to equip a bow, crossbow,
thrown weapon or other ranged weapon. Record its maximum range, damage D12s and
hit requirements for distance bands 1–3, 4–12, 13–24, 25–36 and 37+. **None
equipped** disables ranged attacks. A missing weapon opens a blocked preview
with an attacker-profile button, so it can be configured there too.

Source defaults are Torallion's longbow for Elf (48 squares, 4 damage dice,
hits 3/4/5/6/7) and Telor's thrown dagger for Wizard (4 squares, 1 damage die,
hits 6/7/8/9/10). Other hero classes start without ranged equipment; this does
not restrict what the user may equip. Unambiguous monster reference rows are
loaded when available. Ambiguous or unsupported special profiles require manual
entry. **Other** weapon types leave movement eligibility to the user.

Range is the sum of horizontal and vertical square distances. A gold firing line
appears during the ranged preview. Clearly intersected friendly and hostile
figures block it, including defeated figures still on the board. Walls, closed
doors and unexplored space block shots. Exact corner grazes are treated
generously: side figures touched only at a corner are ignored, and either open
route around a wall corner is accepted. Cancel any shot you disallow.

Doors now retain an individual open state: click them in Player View to open
them. Revealing one room does not open every other door into it. When importing
pre-version-5 saves, old openings are inferred where both sides were already
explored, since those saves did not record individual door states.

**Roll attack** rolls the hit die, damage and all bonus dice, and calculates a
preview in one action. A natural 12 halves the target's Toughness for wound rolls
(fractions round down); it does not grant a melee free attack. Review the displayed
result, then Apply or Cancel.

A natural 1 shows a **Friendly fire** message only after the fumble occurs:
“Fumble! You may have hit a friendly model. Resolve friendly fire manually.”
There is no friendly-fire selector or question in the combat form beforehand.
The intended target is unharmed, and the program does not choose or damage a
friendly model. Resolve any friendly fire manually using the rulebook: a friendly
model within two squares of the intended target is struck; if more than one is
present, the player controlling the intended target chooses. With none, it misses.
Apply commits the preview; Cancel discards it. Undo reverses applied changes.

## Movement, turns and death-zone focus

**Party → Next Turn (reset movement)** clears the moved flag for all heroes and
monsters. It preserves death-zone focus. Moving an already placed model sets its
moved flag; bows and crossbows cannot fire while it is set. Thrown weapons can.
The ranged profile's **Moved this turn** checkbox permits manual correction.
Reserve placement is treated as setup rather than movement.

With living opponents on the map, movement follows a shortest open orthogonal
path through explored, unoccupied squares. It stops at the first newly entered
enemy death zone, even if you dropped the token farther away. Move in shorter
segments if you want a different route. When no opponents are placed, the
existing creative repositioning behaviour remains available.

A normal death zone covers the four orthogonal neighbours reachable without a
wall or closed door. Its owner focuses on the first opponent entering it. Other
opponents can then pass that owner's zone without being stopped by it, though
other enemies may still stop them. The focus follows its opponent while it
remains in the zone, and clears when that opponent leaves or either model is
removed. A removed model's focus clears immediately; Undo restores it. Defeated
heroes retained on the map are still models until explicitly removed.

A shooter in an enemy's active death zone cannot fire. Diagonal proximity and
long-reach exceptions do not prohibit ranged attacks in this implementation,
following the user's generous diagonal ruling. The user retains the final say.
Movement limits, attack counts, initiative and ammunition remain manual.

Version 5 persists ranged profiles, moved flags, focused opponents and individual
opened doors through saves and automatic recovery. New dungeons clear movement
and focus while retaining equipment. `tests/ranged-check.py` covers native
click/drag routing in both views, line of sight, movement/focus, criticals,
fumbles, Cancel/Apply/Undo, profiles and save migration.

Heroes who have applied a ranged attack see an already-fired explanation and a
**Next Turn** button in the ranged dialog. Applied misses and fumbles also use
the shot; cancelled previews do not. Next Turn immediately resets all movement
and hero ranged attacks, even if the dialog is later cancelled; Undo can restore
the prior turn. The fired flag is saved and recovered; older saves default to
not fired. Melee free attacks and their chains are unaffected.
