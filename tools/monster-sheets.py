"""Build monster reference cards for the Sentinel V bestiary.

    python tools/monster-sheets.py

Writes tables/sentinel/monster-sheets.html - one card for every creature in
tables/sentinel/monster.tab, eight to an A4 portrait page, in the layout of
the Advanced HeroQuest monster reference sheets: the WS BS S T Sp Br Int W PV
strip across the top, the two combat blocks with their hit rolls and damage
dice filled in, and an equipment and notes box.

Pages are grouped so each one covers a stretch of the campaign:

    1   the Ork mob and the brood, decks 1 to 3
    2   the Silent Hand and the machines, decks 2 to 4
    3   the Sentinel Host, deck 5 - Eldar only, so this one page is
        the whole bestiary for the last deck

Hit rolls are computed from the tables on page 43 of the rulebook, the same
two formulas the character sheets use:

    hand-to-hand   7 + defender's WS - attacker's WS, clamped to 2..10
    ranged         12 - Bow Skill + range band, minimum 3, a miss above 12

Checked against the printed tables cell by cell, 144 of 144 and 60 of 60, and
against the shipped Skaven cards: a Warrior on Weapon Skill 6 gives
2 3 4 5 6 7 8 9 10 10 10 10, and a Globadier on Bow Skill 6 gives 6 7 8 9 10.
Both come out right.
"""
import io
import os

# ws bs s t sp br int w  - a dash where the characteristic does not apply.
# pv is the Points band the matching table in monster.tab can roll.
# hth  = damage dice in hand-to-hand.
# rng  = (max range in squares, damage dice) or None for no ranged attack.


def M(name, set_, ws, bs, s, t, sp, br, intl, w, pv, hth, rng, notes):
    return dict(name=name, set=set_, ws=ws, bs=bs, s=s, t=t, sp=sp, br=br,
                intl=intl, w=w, pv=pv, hth=hth, rng=rng, notes=notes)


SC, EA, MD = 'Space Crusade', 'Eldar Attack', 'Mission Dreadnought'

PAGE1 = [
    M('Gretchin', SC, 4, 4, 2, 4, 6, 3, 4, 1, '5', 2, (24, 2),
      'Autogun, knife. Runs for the nearest exit if no Ork is left in '
      'the section.'),
    M('Ork', SC, 7, 5, 4, 8, 6, 8, 5, 1, '10', 4, (36, 4),
      'Bolter, choppa.'),
    M('Ork with Heavy Weapon', SC, 7, 6, 4, 8, 4, 8, 5, 1, '15-30', 4, (48, 6),
      'A looted Heavy Bolter, Missile Launcher, Assault Cannon or Plasma Gun. '
      'Speed already includes the -2 for carrying it.'),
    M('Ork Nob', SC, 9, 6, 5, 10, 6, 10, 6, 2, '20-50', 5, (36, 4),
      'Power claw, bolter. While a Nob is on his feet every Ork and Gretchin '
      'in the section passes its Bravery tests automatically.'),
    M('Chaos Android', SC + ' / ' + MD, 8, 7, 5, 12, 4, 12, 4, 2, '15', 5,
      (36, 5),
      'Integral bolter, power claw. Never takes a Bravery test. Unaffected by '
      'toxins, vacuum, gas and psychic attack.'),
    M('Genestealer', SC, 11, '—', 5, 10, 12, 12, 6, 2, '25', 6, None,
      'Rending claws. No ranged attack. Moves its full 12 squares and still '
      'attacks in the same turn. Its claws ignore the Toughness bonus of '
      'armour.'),
    None, None,
]

PAGE2 = [
    M('Chaos Space Marine', SC, 9, 8, 5, 12, 6, 11, 8, 2, '25', 5, (36, 4),
      'Bolter, chainsword. Power armour is already in the Toughness.'),
    M('Chaos Space Marine with Heavy Weapon', SC, 9, 9, 5, 12, 4, 11, 8, 2,
      '30-45', 5, (48, 7),
      'Heavy Bolter or Missile Launcher, chainsword. Speed already includes '
      'the -2 for the heavy weapon.'),
    M('Chaos Space Marine Commander', SC, 11, 9, 6, 14, 6, 12, 9, 4, '50-100',
      6, (12, 4),
      'Power weapon, bolt pistol. Toughness 14: wounded only on a damage dice '
      'of 12, which is then rolled again.'),
    M('Tarantula Turret', MD, 2, 8, '—', 10, '—', '—',
      '—', 3, '20-50', '—', (48, 6),
      'A station turret slaved to the enemy. Cannot move and cannot attack in '
      'hand-to-hand; the Weapon Skill of 2 is only for working out hits '
      'against it. Fires at the nearest model it can see.'),
    M('Dreadnought', SC + ' / ' + MD, 9, 8, 8, 18, 4, 12, 6, 6, '40-70', 8,
      (24, 8),
      'Assault cannon, power fist. Toughness 18: only a damage dice of 12 '
      'wounds it, and that dice is rolled again. A Fusion Gun rolls double '
      'damage dice against it.'),
    M('Advanced Dreadnought', MD, 10, 9, 9, 20, 4, 12, 6, 8, '60-120', 10,
      (48, 10),
      'Las-cannon and conversion beam, power fist. Toughness 20: wounded only '
      'on a damage dice of 12, rolled again. Fusion Guns roll double dice.'),
    None, None,
]

PAGE3 = [
    M('Eldar Guardian', EA, 7, 8, 3, 6, 8, 8, 9, 1, '20', 3, (24, 4),
      'Shuriken catapult. The rank and file of the Sentinel Host.'),
    M('Eldar Guardian with Shuriken Cannon', EA, 7, 9, 3, 6, 6, 8, 9, 1, '30',
      3, (36, 6),
      'May fire at up to 3 targets in a turn, all within 2 squares of each '
      'other.'),
    M('Eldar Guardian with Missile Launcher', EA, 7, 9, 3, 6, 6, 8, 9, 1, '35',
      3, (48, 7),
      'Blast: every model adjacent to the target takes 3 damage dice.'),
    M('Eldar Guardian with Las-Cannon', EA, 7, 9, 3, 6, 6, 8, 9, 1, '40', 3,
      (48, 8),
      'One shot a turn. Ignores the Toughness bonus of armour.'),
    M('Eldar Leader', EA, 12, 10, 4, 8, 10, 11, 11, 3, '60-120', 6, (16, 4),
      'Power sword, shuriken pistol. One miniature, six characters: roll on '
      'Eldar-Leader in monster.tab for which. Farseer Ulvaneth is the one the '
      'strike force came down for.'),
    M('Eldar Wraithlord', 'proxy — Dreadnought model', 10, 9, 8, 16, 6,
      12, 8, 6, '70-120', 9, (48, 8),
      'Wraithcannon, wraithbone fists. Toughness 16: wounded only on a damage '
      'dice of 12, rolled again. Optional; see README.md.'),
    None, None,
]

PAGES = [PAGE1, PAGE2, PAGE3]
BANDS = ['1-3', '4-12', '13-24', '25-36', '37+']


def hth_row(ws):
    """Rulebook p43, Hand-to-Hand Hit Rolls Table."""
    if not isinstance(ws, int):
        return ['—'] * 12
    return [str(max(2, min(10, 7 + d - ws))) for d in range(1, 13)]


def ranged_row(bs):
    """Rulebook p43, Ranged Hit Rolls Table."""
    if not isinstance(bs, int):
        return [''] * 5
    out = []
    for band in range(5):
        v = 12 - bs + band
        out.append('—' if v > 12 else str(max(3, v)))
    return out


def card(m):
    if m is None:
        m = M('', '', '', '', '', '', '', '', '', '', '', '', ('', ''), '')
        blank = True
    else:
        blank = False

    stats = ''.join('<td>%s</td>' % v for v in
                    (m['ws'], m['bs'], m['s'], m['t'], m['sp'], m['br'],
                     m['intl'], m['w'], m['pv']))

    if blank:
        hits = ''.join('<td></td>' for _ in range(12))
        rng = ['' for _ in range(5)]
        maxr, rdd = '', ''
    else:
        if m['hth'] == '—':
            hits = ''.join('<td></td>' for _ in range(12))
        else:
            hits = ''.join('<td class="f">%s</td>' % v for v in hth_row(m['ws']))
        rng = ranged_row(m['bs'])
        if m['rng'] is None:
            rng = [''] * 5
            maxr, rdd = '—', '—'
        else:
            maxr, rdd = m['rng']

    rngcells = ''.join('<td class="f">%s</td>' % v for v in rng)
    hdr = ''.join('<td>%d</td>' % i for i in range(1, 13))
    bandhdr = ''.join('<td>%s</td>' % b for b in BANDS)

    return f'''<div class="mc">
  <div class="hd">
    <div class="nmbox">
      <div class="nm">{m['name'] or '&nbsp;'}</div>
      <div class="set">{m['set']}</div>
    </div>
    <table class="ch">
      <tr class="k"><td>WS</td><td>BS</td><td>S</td><td>T</td><td>Sp</td>
          <td>Br</td><td>Int</td><td>W</td><td>PV</td></tr>
      <tr class="v">{stats}</tr>
    </table>
  </div>
  <div class="body">
    <div class="pic"></div>
    <div class="tables">
      <div class="bar">Hand to Hand Combat</div>
      <table class="t">
        <tr><td class="lh">Target<br>WS</td>{hdr}<td class="dh">Dam.<br>Dice</td></tr>
        <tr><td class="lh">Hit<br>Roll</td>{hits}<td class="dd">{m['hth']}</td></tr>
      </table>
      <div class="bar">Ranged Combat</div>
      <table class="t r">
        <tr><td class="lh">Range</td>{bandhdr}<td class="dh">Max<br>Range</td>
            <td class="dh">Dam.<br>Dice</td></tr>
        <tr><td class="lh">Hit<br>Roll</td>{rngcells}
            <td class="dd">{maxr}</td><td class="dd">{rdd}</td></tr>
      </table>
      <div class="bar">Equipment / Notes</div>
      <div class="notes">{m['notes']}</div>
    </div>
  </div>
</div>'''


CSS = '''
@page { size: A4 portrait; margin: 7mm; }
* { box-sizing: border-box; }
body { font-family: "Book Antiqua", Palatino, Georgia, serif; margin: 0;
       color: #000; }
.page { display: grid; grid-template-columns: 1fr 1fr;
        grid-template-rows: repeat(4, 1fr); gap: 3mm;
        width: 196mm; height: 283mm; page-break-after: always; }
.page:last-child { page-break-after: auto; }
.mc { border: 2px solid #000; padding: 1.4mm; display: flex;
      flex-direction: column; overflow: hidden; }
.hd { display: flex; gap: 1.2mm; align-items: stretch; margin-bottom: 1.2mm; }
.nmbox { flex: 1; border: 1px solid #000; padding: .6mm 1mm;
         display: flex; flex-direction: column; justify-content: center; }
.nm { font-size: 9.5pt; font-weight: bold; font-variant: small-caps;
      line-height: 1.05; letter-spacing: .01em; }
.set { font-size: 6pt; font-style: italic; color: #555; margin-top: .3mm; }
table.ch { border-collapse: collapse; table-layout: fixed; width: 53mm; }
table.ch td { border: 1px solid #000; width: 5.6mm; text-align: center; }
table.ch td:last-child { width: 8.2mm; }
table.ch tr.k td { background: #2a2a2a; color: #fff; font-size: 6.4pt;
                   font-weight: bold; height: 4mm; }
table.ch tr.v td { font-size: 9pt; font-weight: bold; height: 6.2mm; }
.body { display: flex; gap: 1.2mm; flex: 1; min-height: 0; }
.pic { width: 25mm; border: 1px solid #000; }
.tables { flex: 1; display: flex; flex-direction: column; min-width: 0; }
.bar { background: #2a2a2a; color: #fff; font-size: 6.6pt; font-weight: bold;
       text-align: center; letter-spacing: .06em; height: 4mm;
       line-height: 4mm; font-variant: small-caps; }
table.t { border-collapse: collapse; table-layout: fixed; width: 100%; }
table.t td { border: 1px solid #000; text-align: center; font-size: 6.8pt;
             height: 4.8mm; padding: 0; }
td.lh { width: 9mm; font-size: 5.6pt; background: #eee; line-height: 1.05; }
td.dh { width: 8mm; font-size: 5.6pt; background: #eee; line-height: 1.05; }
td.dd { font-size: 8.5pt; font-weight: bold; }
td.f { font-weight: bold; font-size: 7.4pt; }
tr.ghost { display: none; }
.notes { border: 1px solid #000; border-top: 0; flex: 1; padding: 1mm 1.2mm;
         font-size: 6.6pt; line-height: 1.32; }
'''


def main():
    out = ['<!doctype html><html><head><meta charset="utf-8">',
           '<title>Sentinel V — monster reference</title>',
           '<style>%s</style></head><body>' % CSS]
    for pg in PAGES:
        out.append('<div class="page">')
        for m in pg:
            out.append(card(m))
        out.append('</div>')
    out.append('</body></html>')

    dest = os.path.join('tables', 'sentinel', 'monster-sheets.html')
    io.open(dest, 'w', encoding='utf-8', newline='\n').write('\n'.join(out))
    n = sum(1 for pg in PAGES for m in pg if m is not None)
    print('%s  (%d creatures, %d pages)' % (dest, n, len(PAGES)))


if __name__ == '__main__':
    main()
