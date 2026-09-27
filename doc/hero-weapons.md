# Hero weapon presets

Source: the user-provided `heroesStats.png`, transcribed visually on
23 September 2026. The user explicitly authorized invented values for classes
not represented. These are editable starting equipment packages; they do not
replace a hero's existing attributes or calculate a hit row from current WS.

## Defaults used by the application

| Hero type | Weapon | Damage D12 | Hit row | Basis |
| --- | --- | ---: | --- | --- |
| Warrior | Sword | 4 | A | Heinrich Löwen |
| Dwarf | Warhammer | 4 | A | Sven Hammerhelm |
| Elf | Sword | 3 | B | Torallion Leafstar |
| Wizard | Dagger | 1 | C | Magnus the Bright / Telor |
| Henchman | Short sword | 2 | D | Invented suggestion |
| Warrior Priest | Warhammer | 3 | A | Invented suggestion using row A |
| Rogue | Short sword | 2 | D | Invented suggestion |
| Fighter | Sword | 4 | A | Suggested reuse of Heinrich's equipment |
| Mage | Staff | 2 | C | Invented suggestion using the wizard row |

The weapon field names the source character or includes **(suggested)** for an
invented class package. Every value remains editable in **Weapon / combat**.
All these selected profiles use the current standard fumble on 1 and critical
on 12; the damage dice count already represents the selected weapon package.
Pack files provide these editable settings per character profile.

| Target WS | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| A: Heinrich / Sven | 2 | 2 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 10 |
| B: Torallion | 2 | 2 | 2 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
| C: Magnus / Telor | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 10 | 10 | 10 | 10 |
| D: invented support profile | 3 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 10 | 11 | 12 |

New heroes receive their class package immediately. Loading a save or recovery
fills only completely blank hero melee profiles (empty weapon name, zero damage
and all-zero hit entries). Any partially or fully entered profile is retained.
Names, WS, Strength, Toughness, armour adjustments, Wounds, Fate and positions
are not changed by this equipment upgrade. Ranged combat originally extended
the save format to version 5; later versions preserve expanded weapon profiles.

## Other weapon information visible in the image

These entries are retained here as reference, not silently substituted for the
selected default packages:

| Character | Weapon | Printed range | Damage D12 | Fumble | Critical |
| --- | --- | --- | ---: | --- | --- |
| Magnus the Bright | Dagger | N/A | 1 | 1 | 12 |
| Heinrich Löwen | Sword | N/A | 4 | 1 | 12 |
| Torallion Leafstar | Sword | N/A | 3 | 1 | 12 |
| Torallion Leafstar | Long bow | 48 | 4 | 1 | 12 |
| Sven Hammerhelm | Warhammer | N/A | 4 | 1 | 12 |
| Rogar | Double-handed sword | N/A | 6 | 1–2 | 11–12 |
| Ladril | Sword | N/A | 3 | 1 | 12 |
| Telor | Dagger | 4 | 1 | 1 | 12 |
| Durgin | Double-handed axe | N/A | 5 | 1–2 | 11–12 |

Melee and ranged profiles store weapon-specific critical and fumble
thresholds. Melee reach is also stored with each profile: normal reach covers
orthogonal neighbors; long reach covers diagonal neighbors and their death
zones when the connecting floor routes are open. Large weapons such as Rogar's
double-handed sword and Durgin's double-handed axe use fumble 1–2 and critical
11–12 when entered in their profiles. Ranged fumbles strike a friendly model
within two squares of the intended target, selected by that target's controller;
with no eligible model, the shot misses. No new character classes are added;
each model has one active melee profile and one active ranged profile.

Opening a hero's Weapon / combat editor fills missing (zero) entries with class
defaults while keeping entered values. Choose **Save profile** to keep them,
or **Cancel** to discard the preview. **Use class defaults** replaces the weapon,
damage dice and full hit row in the editor; it leaves WS and Toughness alone.
