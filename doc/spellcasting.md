# Spellbooks and casting

Select a caster on the map. The combat panel and token context menu offer
**Cast spell...** and **Spellbook...**. Heroes & Reserve also has
**Spellbook & components...**, including for reserve heroes.

A new Bright Wizard knows Dragon Armour, Open Window, Flames of Death and
Flames of the Phoenix. Choose four starting components in any mix for those
spells, then click **Finish starting allocation**. Quantities are inventory,
not a separate number of charges attached to each spell: several spells can
spend the same ingredient. Starting setup only offers components for known spells.

The dialog shows the caster's name and has **Spells** and **Components** tabs.
The spell list shows learned status, the casts current stock could support,
and why a spell is unavailable. Select a spell to read its full rules; selection
does not change what the caster knows. Cast counts describe each spell alone:
shared ingredients cannot simultaneously fund every displayed count.

**GM adjustment...** opens a separate draft for book membership and learned
spells. **Use adjustments** returns these changes to the main draft. Cancel
leaves it untouched. Prices are reference information; no tuition is deducted.

On the Components tab, select an ingredient and choose **Adjust quantity...**.
Enter the new total. The list shows carried stock and price per component; no
gold is deducted. **Save changes** commits the whole draft; **Cancel** discards it.

A new Wizard must allocate exactly four starting components. The progress
message explains why Finish allocation and Save changes are disabled until
the total matches. Finish allocation marks setup complete within the draft;
Save changes commits it. Existing saves instead offer **Record remaining stock**,
with no four-component requirement and no invented replenishment.

## Casting

Choose a learned spell. Its description shows components, current stock,
movement restrictions and any reason it cannot be cast. Only an active, placed
caster can cast. Guided turns require the correct side's phase. A caster can
cast once per turn; **Next Turn** in free play or the next phase for that side
resets this allowance. Weapon attacks have a separate allowance. GM override
bypasses phase, casting-use and movement limits, but never creates ingredients.

A spell requiring two or more components forbids movement that turn. A prior
move prevents casting; committing the spell prevents subsequent movement.
Reserve placement is setup and does not count as a movement action.

The casting window stays open while you click the map. **Pick on map** temporarily
hides it if it covers the desired square; Escape brings it back. Click a model
for a single-target spell, or click characters to add/remove Swift Wind targets.
For Flight, click the model first, then an empty destination. Section spells use
a square in that section. Bright Key uses an inside wall square; the nearest wall
direction is selected automatically and can be changed in the window.

For a template spell, click its lower-left square, or enter its 1-based board
X/Y coordinates. The anchor must be within range and ranged line of sight.
The original fireball tiles use a 2-by-2 footprint, defined in the fantasy pack.
Gold outlines preview coverage and affected models; red outlines mean invalid
placement. Every revealed living figure in the footprint is included automatically,
including friendly figures and knocked-out heroes. The list cannot omit targets.
The footprint may overlap walls or empty space, but must fit inside the board;
only models on revealed floor are affected. Grid-aligned square coverage is the
explicit digital placement convention. An empty footprint can still be cast.
Damage spells automatically
hit: no weapon hit roll, critical threshold or fumble is used. Each target has
its own damage dice against Toughness, including bonus rolls for twelves.

Healing chooses one living model, including a KO'd hero. The caster may heal
itself or a model in its death zone; another model in that zone blocks the spell.
Healing restores maximum Wounds and removes KO. It does not resurrect the dead.

**Preview cast / roll** displays the outcome without spending resources. Spells
with an Intelligence test roll D12 against Intelligence; **Spend 1 Fate to succeed** is offered after a failed test. Choose it or
**Continue with failure**; the original Intelligence roll is retained even when targets change, and Fate
is spent only on Apply. Inferno
of Doom uses seven damage dice on success and five on failure.

**Apply cast** commits the effect, spends components and any Fate, and records
casting/movement restrictions together. Cancel before Apply spends nothing.
Undo restores the complete committed change. Manual stock edits made through
Components are separately saved actions, even if the casting window is cancelled.

The casting footer shows component costs, movement consequences, friendly-fire
warnings and the reason Preview or Apply is unavailable. After applying, the
casting window becomes **Spell cast - results** and stays open until **Close**.
It names the caster and targets and reports actual damage, remaining Wounds,
KO/death, healing, movement, effective characteristics and effect expiry.
It also lists ingredients consumed, remaining stock and Fate spent. Roll details
remain readable. Open Window includes its private inspection in these results.
The combat panel retains a short outcome message. Closing results cannot cast
the spell again or spend more resources.

## Bright spell effects

All twelve Bright spells have effect handlers. Their rules and component costs
come from the theme pack; names are not used to choose the runtime effect.

| Spell | Automatic effect and targeting |
| --- | --- |
| Dragon Armour | Select the caster or a model in its death zone. +1 effective Toughness (maximum 12) until the next exploration turn; repeated casts do not stack. Melee, ranged and spell damage use this bonus. |
| Open Window | Enter a square in an unexplored section. Apply opens a private contents message, leaves fog unchanged and spawns nothing. On discovery, the section gets a pending +3 Leader surprise bonus. |
| Flames of Death | Pack-defined 2-by-2 template within 12 squares and line of sight; automatically includes covered figures and friends, five damage dice each. |
| Flames of the Phoenix | Full Wounds and recovery from KO for the caster or a model in its death zone, with no other figure in that zone. Does not resurrect. |
| The Bright Key | Enter an inside wall square in the caster's section and select the outward direction. Creates a saved, closed door. Open it in Player View. Toward unexplored space, D12 1–4 permanently blocks the door with rock; 5–12 opens to the existing section or generates using the active campaign tables. |
| Flaming Hand of Destruction | Applies to the caster's successful melee hits in following combat turns. One D12 causes that many Wounds directly: no Toughness test and no exploding twelve. Critical/fumble free attacks retain their normal rules. Expires at exploration. |
| Flight | Select a model which has not moved, then enter its destination. Preview rolls a run and shows its actual endpoint. Apply moves along a legal route, stops at the first new enemy death zone, and uses movement and the normal attack. Walls, closed doors and occupied squares block the route. |
| Swift Wind | Preview rolls D12 once; half rounded up is the maximum number of selected characters, including the caster. Select fewer if needed and preview again with the same roll. Movement is 18 squares in exploration or double Speed in combat, minus movement already spent. Expires next full turn. |
| Power of the Phoenix | Select a recorded corpse in the turn immediately after death. The square must be visible and empty. Intelligence success (or Fate) restores full Wounds at that square; failure consumes the components and permanently marks the soul lost. KO is ineligible. |
| Still Air | Enter a square in a section within line of sight. Monsters in that section cannot move, run or attack, including free attacks and movement from Flight, until the next full turn. Monsters entering the section are also affected. GM override can bypass the restriction. |
| Inferno of Doom | Pack-defined 2-by-2 template; automatically includes covered figures and friends, seven damage dice on Intelligence success, five on failure. |
| Courage | Select the caster or a model in its death zone. Effective Bravery becomes 12 until exploration. The panel shows this value; Bravery tests remain GM adjudicated. |

The panel shows effective Toughness and Bravery, and movement remaining.
On the board, blue outlines and labels show active enchantments with their expiry:
**until exploration**, **until Turn N**, or **next combat turn** for a waiting
Flaming Hand. Still Air also labels its section. Corpse crosses show the character
and resurrection status: amber means next turn, green means this turn, grey means
expired, and red means the soul is lost. A blocked corpse square is identified.
Click the corpse when targeting Power of the Phoenix. Hidden corpses and effects
are not shown in Player View. Zoom in to read the full labels.
**Party > Start / Next exploration turn** starts an exploration turn with a
12-square movement budget and expires Dragon Armour, Courage and Flaming Hand.
**Next exploration turn** advances it. In guided combat, Hero-to-GM changes
phase within the same turn; GM-to-Hero advances the full turn. Starting guided
combat begins a new turn. In free play,
**Next Turn** advances the clock. Leaving guided combat alone does not start
exploration or expire those effects. The turn number is saved.

When entering an area inspected through Open Window, the panel announces its
bonus. **Party > Resolve spied-area surprise...** rolls the Leader's D12, adds
three and offers Apply/Cancel. Applying consumes that area's bonus once. Other
surprise modifiers and the opposing surprise roll remain GM adjudicated.

Maps are generated before play, so Open Window inspects the existing hidden
section rather than generating a second encounter. Bright Key generation needs
spare piece slots, enough free space inside the board and the selected campaign's
generation tables. If generation cannot fit, the door remains closed with its
successful rock test recorded; retries do not reroll that test. Generated
continuations stay hidden until discovered. Undo restores door geometry, cells,
fog, spawned monsters and spell state together.

Rules interpretations: Dragon Armour permits self-targeting, matching the other
touch bonuses. Flaming Hand applies to hand-to-hand combat, not ranged weapons.
Swift Wind's rolled allowance includes the caster. Flight uses the ordinary
run and death-zone rules and consumes the normal attack. An earlier attack does not prevent targeting; movement or a stationary-casting lock does. Partial visibility
uses existing centre-based line of sight; enable GM override during guided
combat to accept a target adjudicated as partly visible. Resurrection has no
extra range restriction because the spell supplies none; its corpse square
must be available. A resurrected character has already used movement and attack
for that turn, including in free play. Resurrection clears temporary enchantments and run bonuses. Still Air lasts through the current GM phase and expires at the
next full-turn boundary. These decisions are explicit rather than hidden rules.

Skaven Fireball is automatic. Other supplied Skaven effects remain explicitly
GM-resolved. Their Apply action records casting and spends ingredients, with
no claim that their manual effects have been applied.

## Packs and saves

Each theme JSON is the source of its components, spells, spellbooks and caster
profile defaults. Compile with `python tools/compile-packs.py`. The compiler
checks references and the runtime decoder bounds and validates the compiled
HQPACK5 resource. HQPACK1-4 remain readable; old template spells use the original
2-by-2 footprint. Campaign Builder preserves the
complete pack, so its spell definitions travel with the campaign.

Version 15 saves embed the pack, temporary effects, turn clock, corpse records,
spying/surprise and magic-door states, as well as personal casting state including stock,
learned spells, books and used/locked flags. Old saves remain readable. Older
fantasy pack snapshots gain the supplied spell catalogue while preserving their
hero/monster definitions. Version 14 saves retain their stock and known spells
and upgrade the previously manual built-in Bright handlers without changing slot
identities. Older deaths have no invented timing and cannot be resurrected by
this timed spell. Pre-spellcasting heroes require component setup; remaining
stock is not guessed. Legacy spellcasting monsters start with zero stock for
GM correction. New Skaven sorcerers use their printed starting stock.
