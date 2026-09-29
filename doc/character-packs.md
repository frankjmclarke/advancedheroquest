# Campaign character packs

Campaigns select the heroes, enemy references and combat presets used for a new
dungeon. They all use the existing AHQ ranged and hand-to-hand combat rules.

Load `sentinel1.tab` through `sentinel5.tab` under **Options > Load Tables**, then
generate a dungeon. **Party > Heroes and Reserve** offers a Space Marine
Commander and eight equipped Marine presets. Reveal rooms in Player View to
place their enemies. Existing party members keep their names, equipment,
characteristics and class notes when continuing into another campaign.

The original nine fantasy presets, literal monster references, ambiguous source
variants and combat defaults are preserved. Quests without a pack declaration
use the built-in Old World pack.

## Authoring a pack

The editable sources are `data/packs/fantasy.json` and
`data/packs/sentinel.json`. Each file contains that theme's heroes, enemies,
aliases, statistics, combat presets and reference text. Generated runtime
resources, references and printable sheets all use the fantasy pack directly.

Copy a pack JSON file to start another theme, for example `wwii.json`. Give the
pack and its profiles new stable IDs, edit the presets and aliases, then run:

```text
python tools/compile-packs.py
python tools/compile-packs.py --check
```

This produces `tables/<pack-id>/characters.hqp`. The application reads this
bounded, versioned data resource at runtime; players do not need Python or a
new executable for additional packs. The compiler also produces the embedded
fantasy fallback in `pack-fantasy-data.h`; rebuilding is necessary when changing
that fallback.

Add a declaration to each quest entry, after its title:

```text
#My WWII adventure
;character-pack wwii/characters.hqp
```

Paths are relative to the quest entry, cannot be absolute or contain `..`, and
must identify one self-contained resource. Missing, malformed or unsupported
resources fail explicitly rather than substituting fantasy characters. The
comment syntax keeps the existing table language intact.

Each pack has `version: 1`, `id`, `name`, a `heroes` array, a `monsters` array,
and optionally `nonmonsters`: exact names of salvage, equipment or other text
that could otherwise resemble a counted encounter. Profiles contain:

| Field | Meaning |
| --- | --- |
| `id` | Stable, unique `<pack-id>:<profile-id>`; at most 63 characters |
| `name` | Display name; at most 63 characters |
| `aliases` | Explicit lowercase singular/plural encounter names; no fuzzy species matching |
| `stats` | Nine values in WS, BS, S, T, Sp, Br, Int, W, PV order |
| `text` | Reference text or hero class notes; does not execute rules |
| `melee` | Weapon name, damage dice, twelve hit values indexed by target WS, and optional `critical`, `fumble`, `reach` properties |
| `ranged` | Weapon name, kind, maximum range, damage dice, five hit values, and optional `critical`, `fumble` properties |
| `fate` | Hero starting Fate |
| `kind` | Existing marker/preset code 0–8; defaults to 0. Code 4 retains Henchman death removal |
| `unique` | `1` for a named enemy whose identity and death persist; otherwise `0` |

`critical` is the hit roll that grants a free attack (default 12); `fumble` is
the hit roll or lower that grants the defender a free attack (default 1). The
fumble threshold must be below the critical threshold. `reach: 2` is a long-
reach weapon: it can attack a diagonally adjacent square when both connecting
floor routes are open, and its death zone includes diagonal adjacent squares.
`reach: 1` is normal reach (the default). Ranged weapons default to critical 12
and fumble 1; a ranged critical halves target Toughness, while a fumble strikes
a friendly model within two squares of the intended target if one is there.
These settings are editable and persist in saved games. Older packs and saves
retain their former 12/1 thresholds and orthogonal reach.

Ranged kinds are 0 (none), 1 (bow), 2 (crossbow), 3 (thrown), and 4 (other,
manual movement). Ranges use squares; the five bands are 1–3, 4–12, 13–24,
25–36 and 37+. A zero hit value means unconfigured/unavailable. Characteristics
and damage dice are 0–99; monster PV can reach 9999. Hero reference notes are
limited to 2047 bytes, monster reference text to 4095. Resources use Windows
CP1252, matching the native application. A pack supports up to 256 hero presets
and 256 monster profiles; the existing live-party and token limits are unchanged.

Monster references with the same alias remain separate profiles when the
printed source contains variants. Ambiguous references are displayed together;
combat values are populated only where the printed values have an unambiguous
structured interpretation.

Sentinel's heavy-weapon variants have explicit profiles. Its six named Eldar
leaders share the Eldar Leader characteristics but have separate IDs and point
values. Encounter descriptions spell out each group, including heavy-weapon
Guardians and Chaos Marines. Repeated reveals do not create duplicate tokens;
repeated appearances of a living named leader do not create a second copy.

No science-fiction combat rules have been added. Weapons use the existing
damage and turn behaviour, with pack-defined melee and ranged critical/fumble
thresholds and normal or long melee reach where needed. Multi-target fire, blast templates,
jams and similar abilities on the original cards are not applied. Conversion
Beam presets use the printed near damage (5 dice) with range 48; damage remains
editable like other existing equipment. Generic heavy-weapon groups retain
their original generic reference profile until the player chooses equipment.

## Saves and Campaign Builder

Version 8 `.hqg` files embed the complete active pack, including the definitions
needed for unrevealed rooms, plus each party member's identity and class notes.
They also store KO/death state and melee weapon settings. Editing or removing the
original resource does not change an adventure loaded from that save. Named enemy
deaths use stable IDs even if their display names are edited. Manual characters
retain the existing name-based identity behaviour.

Versions 1–7 remain readable. They contain no KO/death state or weapon thresholds,
so zero-Wound Heroes load as KO'd and legacy melee profiles keep 12/1 thresholds.
Versions 1–6 also contain no pack identity, so when loading
one the application uses the campaign pack currently selected in **Load Tables**;
without a selected campaign it falls back to the original fantasy pack. This
lets existing Sentinel saves load with Sentinel profiles. As before, generating
the next dungeon requires the selected campaign tables; a save contains the
current dungeon, not all quest-generation tables.
Older executables cannot read version 7 saves.

Campaign Builder copies the starting campaign's complete resource into the new
campaign directory and rewrites the declaration. The copy can be copied again,
and works after the original campaign is removed. Selecting monster tables from
a different known pack is rejected with an explanation, rather than producing
unresolved encounters. To author a mixed-theme adventure, provide a combined
pack with explicit, unambiguous profile names.

## Verification

Build with `build-msvc.bat`. The native regressions can be run with:

```text
tools\test-msvc.bat pack campaign-builder game monster combat ranged monster-reference room-contents game-render fog view
```

The pack checks cover every listed Sentinel encounter, multiple generated maps
on each of the five decks, actual Heroes & Reserve controls, stable named deaths,
repeated reveals, corrupt/truncated resources, loading without the source pack,
and old-save migration while another theme is active. The remaining checks cover
existing fantasy combat, save/recovery, fog and both map rendering modes.

## Spellbooks (HQPACK4)

The same theme file optionally defines `components`, `spells` and `spellbooks`.
Every ID shares the pack namespace and must be unique. Spell component maps
reference component IDs with positive quantities; book lists reference spell
IDs. A book's `starting_spells` must be a subset of its `spells`. Profiles can
supply `caster.books`, `caster.starting_components` (a player-chosen allowance)
and `caster.components` (explicit initial stock).

Spell fields: `effect` (`manual`, `damage`, `heal`), `target` (`manual`, `model`,
`template`, `healing`), `range`, `dice`, `intelligence_test`, `failed_dice`,
`stationary`, `learning_cost`, `text` and `components`. Healing uses `healing`
targeting and currently cannot require an Intelligence test. Two or more
components require stationary casting. Unsupported effect types are rejected;
use an explicitly described `manual` effect for GM resolution.

Limits are 16 books, 64 spells and 32 components per pack, with stock quantities
from 0 to 9999. Runtime resources resolve IDs into bounded slots; saves retain
the immutable complete pack snapshot, so slot references preserve their meaning.
Older pack formats remain readable. Version 14 saves add personal casting state;
older saves require explicit component setup rather than reconstructed stock.
See [Spellbooks and casting](spellcasting.md) for the interface and current scope.

## Spellbooks (HQPACK4)

The same theme file optionally defines `components`, `spells` and `spellbooks`.
Every ID shares the pack namespace and must be unique. Spell component maps
reference component IDs with positive quantities; book lists reference spell
IDs. A book's `starting_spells` must be a subset of its `spells`. Profiles can
supply `caster.books`, `caster.starting_components` (a player-chosen allowance)
and `caster.components` (explicit initial stock).

Spell fields: `effect` (`manual`, `damage`, `heal`), `target` (`manual`, `model`,
`template`, `healing`), `range`, `dice`, `intelligence_test`, `failed_dice`,
`stationary`, `learning_cost`, `text` and `components`. Healing uses `healing`
targeting and currently cannot require an Intelligence test. Two or more
components require stationary casting. Unsupported effect types are rejected;
use an explicitly described `manual` effect for GM resolution.

Limits are 16 books, 64 spells and 32 components per pack, with stock quantities
from 0 to 9999. Runtime resources resolve IDs into bounded slots; saves retain
the immutable complete pack snapshot, so slot references preserve their meaning.
Older pack formats remain readable. Version 14 saves add personal casting state;
older saves require explicit component setup rather than reconstructed stock.
See [Spellbooks and casting](spellcasting.md) for the interface and current scope.
