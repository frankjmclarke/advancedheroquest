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

For a template spell, enter its centre using the 1-based board X/Y coordinates.
The centre must be within range and the existing ranged line of sight. Place
the physical template and select every covered model, including friendly figures
(Ctrl-click to add/remove targets). Template coverage is adjudicated by the GM;
the application does not invent a template shape. Damage spells automatically
hit: no weapon hit roll, critical threshold or fumble is used. Each target has
its own damage dice against Toughness, including bonus rolls for twelves.

Healing chooses one living model, including a KO'd hero. The caster may heal
itself or a model in its death zone; another model in that zone blocks the spell.
Healing restores maximum Wounds and removes KO. It does not resurrect the dead.

**Preview cast / roll** displays the outcome without spending resources. Spells
with an Intelligence test roll D12 against Intelligence; **Spend 1 Fate to succeed** is offered after a failed test. Choose it or
**Continue with failure**; the original Intelligence roll is retained and Fate
is spent only on Apply. Inferno
of Doom uses seven damage dice on success and five on failure.

**Apply cast** commits the effect, spends components and any Fate, and records
casting/movement restrictions together. Cancel before Apply spends nothing.
Undo restores the complete committed change. Manual stock edits made through
Components are separately saved actions, even if the casting window is cancelled.

The casting footer shows component costs, movement consequences, friendly-fire
warnings and the reason Preview or Apply is unavailable. After applying, the
combat panel names the spell, reports remaining ingredient stock, and identifies
any GM-resolved effect.

## Implemented effects

Automatic effects: Flames of Death, Flames of the Phoenix, Inferno of Doom,
and Skaven Fireball. Their names are data: other books can use the same effect
handlers with different ingredients, ranges and damage pools.

Other Bright and Skaven spells are explicitly **GM-resolved**. Check the GM
resolution box before previewing. Apply consumes components and records the
casting action, but does not modify the dungeon, temporary characteristics,
movement of targets, resurrection, fear, or timed effects. In particular, Open
Window does not reveal a room or spawn its monsters automatically. Its private
inspection and surprise bonus are handled by the GM.

## Packs and saves

Each theme JSON is the source of its components, spells, spellbooks and caster
profile defaults. Compile with `python tools/compile-packs.py`. The compiler
checks references and the runtime decoder bounds and validates the compiled
HQPACK4 resource. HQPACK1-3 remain readable. Campaign Builder preserves the
complete pack, so its spell definitions travel with the campaign.

Version 14 saves embed the pack and personal casting state, including stock,
learned spells, books and used/locked flags. Old saves remain readable. Older
fantasy pack snapshots gain the supplied spell catalogue while preserving their
hero/monster definitions. Existing heroes require component setup; remaining
stock is not guessed. Legacy spellcasting monsters start with zero stock for
GM correction. New Skaven sorcerers use their printed starting stock.
