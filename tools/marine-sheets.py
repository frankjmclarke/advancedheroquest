"""Build Advanced HeroQuest character sheets for the Space Crusade Marines.

    python tools/marine-sheets.py

Writes tables/sentinel/space-marine-sheets.html - ten cards, two to an A4
landscape page, laid out like the class cards in the Advanced HeroQuest
character sheet set (blank Current ovals, pre-printed Start characteristics,
and the two combat rows already filled in).

The combat rows are computed rather than typed, from the tables on page 43 of
the rulebook:

    hand-to-hand   hit roll = 7 + defender WS - attacker WS, clamped to 2..10
    ranged         hit roll = 12 - Bow Skill + range band, minimum 3,
                   a miss if it comes out above 12

Both were checked cell by cell against the printed tables: 144 of 144 and
60 of 60. The ranged row uses the Current Bow Skill, ie after armour, which
is what the worked example on the shipped sheets does.
"""
import io
import os

# --- characteristics -------------------------------------------------------
#
# One profile for every Space Marine, because that is how the box works: the
# figures are identical and the weapon sprue is what differs. The Commander is
# the separate casting and the separate profile.
#
# Toughness is split into base and armour the way a character sheet wants it,
# rather than given as the single effective number a monster reference table
# would carry. Power armour is Toughness +4 with no penalty, which sits just
# under Enchanted Armour, the best in the treasure tables.

MARINE = dict(ws=9, bs=8, s=5, t=8, sp=6, br=12, intl=8, w=3, fate=1)
COMMANDER = dict(ws=11, bs=9, s=6, t=8, sp=6, br=12, intl=9, w=5, fate=3)

ARMOUR = ('Power Armour', 0, 4, 0)          # name, Bow Skill, Toughness, Speed
ARTIFICER = ('Artificer Power Armour', 0, 4, 0)

HEAVY_NOTE = 'Heavy weapon: Speed -2 while it is carried ready.'

# name, range in squares, damage dice, fumble, critical
BOLTER      = ('Bolter',            '36',   '4',   '1',   '12')
BOLT_PISTOL = ('Bolt Pistol',       '12',   '4',   '1',   '12')
KNIFE       = ('Combat Knife',      'N/A', '3',  '1',   '12')

CARDS = [
    dict(title='Space Marine Commander', set='Space Crusade', prof=COMMANDER,
         armour=ARTIFICER, heavy=False,
         weapons=[('Power Glove', 'N/A', '6', '1', '11-12'),
                  ('Power Sword', 'N/A', '5', '1', '11-12'),
                  BOLT_PISTOL],
         notes=['Carries a Power Glove or a Power Sword, and a Bolt Pistol.',
                'Leads the squad: while he is on his feet, any Marine within '
                '6 squares may re-roll one failed Bravery test per combat.']),

    dict(title='Space Marine, Bolter', set='Space Crusade', prof=MARINE,
         armour=ARMOUR, heavy=False,
         weapons=[BOLTER, KNIFE],
         notes=['The standard trooper. Four of the five in a squad carry this.']),

    dict(title='Space Marine, Heavy Bolter', set='Space Crusade', prof=MARINE,
         armour=ARMOUR, heavy=True,
         weapons=[('Heavy Bolter', '48', '6', '1-2', '12'), BOLT_PISTOL, KNIFE],
         notes=[HEAVY_NOTE,
                'May fire at up to 3 targets in one turn, all within 2 squares '
                'of each other, rolling damage separately for each.']),

    dict(title='Space Marine, Assault Cannon', set='Space Crusade', prof=MARINE,
         armour=ARMOUR, heavy=True,
         weapons=[('Assault Cannon', '24', '6', '1-2', '11-12'), BOLT_PISTOL, KNIFE],
         notes=[HEAVY_NOTE,
                'May fire at up to 4 targets in one turn, all within 2 squares '
                'of each other.',
                'On a fumble the cannon jams: it may not be fired again until '
                'a whole turn is spent clearing it.']),

    dict(title='Space Marine, Missile Launcher', set='Space Crusade', prof=MARINE,
         armour=ARMOUR, heavy=True,
         weapons=[('Missile Launcher', '48', '7', '1-2', '12'), BOLT_PISTOL, KNIFE],
         notes=[HEAVY_NOTE,
                'Blast: every model adjacent to the target, friend or enemy, '
                'takes 3 damage dice.',
                'May not be fired at a target in the firer’s own death zone.']),

    dict(title='Space Marine, Plasma Gun', set='Space Crusade', prof=MARINE,
         armour=ARMOUR, heavy=True,
         weapons=[('Plasma Gun', '36', '7', '1-2', '11-12'), BOLT_PISTOL, KNIFE],
         notes=[HEAVY_NOTE,
                'Overheats on a fumble: the firer takes 3 damage dice against '
                'his own Toughness and may not fire next turn.']),

    dict(title='Space Marine, Las-Cannon', set='Mission Dreadnought', prof=MARINE,
         armour=ARMOUR, heavy=True,
         weapons=[('Las-Cannon', '48', '9', '1-2', '12'), BOLT_PISTOL, KNIFE],
         notes=[HEAVY_NOTE,
                'One shot per turn. Ignores the Toughness bonus of any armour '
                'the target is wearing.']),

    dict(title='Space Marine, Conversion Beamer', set='Mission Dreadnought',
         prof=MARINE, armour=ARMOUR, heavy=True,
         weapons=[('Conversion Beam (near)', '1-12', '5', '1-2', '12'),
                  ('Conversion Beam (far)', '13-48', '9', '1-2', '12'),
                  BOLT_PISTOL],
         notes=[HEAVY_NOTE,
                'The beam gathers as it travels: 5 damage dice out to 12 '
                'squares, 9 beyond that.']),

    dict(title='Space Marine, Fusion Gun', set='Mission Dreadnought', prof=MARINE,
         armour=ARMOUR, heavy=True,
         weapons=[('Fusion Gun', '12', '10', '1-2', '12'), BOLT_PISTOL, KNIFE],
         notes=[HEAVY_NOTE,
                'Short ranged and devastating. Against a Dreadnought, a '
                'bulkhead or a sealed door, roll double damage dice.']),

    dict(title='Space Marine', set='blank — fill in the loadout', prof=MARINE,
         armour=ARMOUR, heavy=False, weapons=[('', '', '', '', ''), KNIFE],
         notes=['Spare card. Combat rows are for the standard profile: if the '
                'weapon changes Bow Skill or Speed, redo the Ranged row.']),
]

BANDS = ['1-3', '4-12', '13-24', '25-36', '37+']


def hth_row(attacker_ws):
    """Rulebook p43, Hand-to-Hand Hit Rolls Table."""
    return [max(2, min(10, 7 + d - attacker_ws)) for d in range(1, 13)]


def ranged_row(bow_skill):
    """Rulebook p43, Ranged Hit Rolls Table. Blank cells mean an automatic miss."""
    out = []
    for band in range(5):
        v = 12 - bow_skill + band
        out.append('Miss' if v > 12 else str(max(3, v)))
    return out


def oval(value=''):
    return '<span class="ov">%s</span>' % value


def stat(label, start, current=''):
    return ('<div class="st"><div class="lbl">%s</div>'
            '<div class="ovs">%s%s</div></div>' % (label, oval(start), oval(current)))


def card(c):
    p = c['prof']
    aname, abs_, at, asp = c['armour']
    speed_now = p['sp'] + asp - (2 if c['heavy'] else 0)
    bs_now = p['bs'] + abs_
    t_now = p['t'] + at

    left = ''.join([stat('WEAPON SKILL', p['ws']),
                    stat('BOW SKILL', p['bs'], bs_now if abs_ else ''),
                    stat('STRENGTH', p['s']),
                    stat('TOUGHNESS', p['t'], t_now)])
    right = ''.join([stat('SPEED', p['sp'], speed_now if speed_now != p['sp'] else ''),
                     stat('BRAVERY', p['br']),
                     stat('INTELLIGENCE', p['intl']),
                     stat('FATE', p['fate']),
                     stat('WOUNDS', p['w'])])

    hth = hth_row(p['ws'])
    rng = ranged_row(bs_now)

    hdr = ''.join('<td>%d</td>' % i for i in range(1, 13))
    hits = ''.join('<td class="fill">%d</td>' % v for v in hth)
    bandhdr = ''.join('<td>%s</td>' % b for b in BANDS)
    bandhit = ''.join('<td class="fill">%s</td>' % v for v in rng)

    wrows = ''
    for i in range(3):
        n, r, dd, f, cr = c['weapons'][i] if i < len(c['weapons']) else ('', '', '', '', '')
        wrows += ('<tr><td class="wn">%s</td><td>%s</td><td>%s</td>'
                  '<td>%s</td><td>%s</td></tr>' % (n, r, dd, f, cr))

    def sgn(v):
        return '0' if v == 0 else '%+d' % v

    notes = '<br>'.join(c['notes'])

    hdrow = ('<div class="hdrow"><span class="sc">START</span>'
             '<span class="sc">CURRENT</span></div>')

    return f'''<div class="card">
  <div class="top">
    <div class="col">{hdrow}{left}</div>
    <div class="art"><span>Chapter badge</span></div>
    <div class="col">{hdrow}{right}</div>
  </div>
  <div class="nameline">
    <div class="nm">Name: <span class="rule"></span></div>
    <div class="ch">Chapter: <span class="rule"></span></div>
  </div>
  <div class="cls">{c['title']}<span class="src">{c['set']}</span></div>

  <table class="cmb">
    <tr><td class="rh" rowspan="2">Hand to Hand<br>Combat</td>
        <td class="sh">Target<br>Wpn Skill</td>{hdr}</tr>
    <tr><td class="sh">Hit Roll</td>{hits}</tr>
  </table>

  <table class="cmb rng">
    <tr><td class="rh" rowspan="2">Ranged<br>Combat</td>
        <td class="sh">Range</td>{bandhdr}</tr>
    <tr><td class="sh">Hit Roll</td>{bandhit}</tr>
  </table>

  <table class="cmb wpn">
    <tr><td class="rh" rowspan="4">Weapons</td>
        <td class="wn"></td><td>Range</td><td>Dam. Dice</td><td>Fumble</td><td>Critical</td></tr>
    {wrows}
  </table>

  <table class="cmb arm">
    <tr><td class="rh" rowspan="3">Armour</td>
        <td class="wn"></td><td>Bow Skill</td><td>Toughness</td><td>Speed</td></tr>
    <tr><td class="wn">{aname}</td><td class="fill">{sgn(abs_)}</td>
        <td class="fill">{sgn(at)}</td><td class="fill">{sgn(asp)}</td></tr>
    <tr><td class="wn tot">TOTAL</td><td class="fill">{sgn(abs_)}</td>
        <td class="fill">{sgn(at)}</td><td class="fill">{sgn(asp)}</td></tr>
  </table>

  <table class="cmb nt">
    <tr><td class="rh">Notes</td><td class="notes">{notes}</td></tr>
  </table>
</div>'''


CSS = '''
@page { size: A4 landscape; margin: 8mm; }
* { box-sizing: border-box; }
body { font-family: "Book Antiqua", Palatino, Georgia, serif; margin: 0;
       background: #ffffff; color: #000; }
.page { display: flex; gap: 5mm; page-break-after: always;
        width: 281mm; height: 194mm; }
.page:last-child { page-break-after: auto; }
.card { flex: 1 1 0; border: 2.5px solid #000; padding: 2.5mm;
        display: flex; flex-direction: column; }
.top { display: flex; align-items: flex-start; }
.col { width: 38%; }
.art { flex: 1; align-self: stretch; min-height: 37mm; margin: 3mm 1.5mm 0;
       border: 1px dashed #bbb; display: flex; align-items: center;
       justify-content: center; }
.art span { font-size: 7pt; color: #bbb; letter-spacing: .08em; }
.st { margin-bottom: .6mm; }
.lbl { font-variant: small-caps; font-size: 8.5pt; letter-spacing: .04em;
       text-align: center; background: #e2e2e2; }
.ovs { display: flex; justify-content: center; gap: 2mm; }
.ov { display: inline-block; width: 11mm; height: 6mm; line-height: 6mm;
      border: 1.2px solid #000; border-radius: 50%; text-align: center;
      font-size: 10pt; font-weight: bold; }
.hdrow { display: flex; justify-content: center; gap: 3.5mm; margin-bottom: .4mm; }
.sc { font-size: 6.2pt; letter-spacing: .1em; color: #555; width: 11mm;
      text-align: center; }
.nameline { display: flex; gap: 3mm; font-size: 9pt; margin-bottom: .8mm; }
.nm { flex: 2; display: flex; align-items: flex-end; }
.ch { flex: 1; display: flex; align-items: flex-end; }
.rule { flex: 1; border-bottom: 1px solid #000; margin-left: 1.5mm; height: 4mm; }
.cls { font-size: 11.5pt; font-weight: bold; border: 1.5px solid #000;
       padding: 1mm 2mm; margin-bottom: 1.2mm; display: flex;
       justify-content: space-between; align-items: baseline; }
.src { font-size: 7.5pt; font-weight: normal; font-style: italic; color: #555; }
table.cmb { width: 100%; border-collapse: collapse; margin-bottom: 1.2mm;
            table-layout: fixed; }
table.cmb td { border: 1px solid #000; text-align: center; font-size: 8.5pt;
               height: 5.2mm; padding: 0; }
td.rh { width: 19mm; font-size: 9pt; font-variant: small-caps; }
td.sh { width: 13mm; font-size: 6.5pt; background: #eee; }
td.fill { font-weight: bold; }
table.rng td, table.wpn td, table.arm td { height: 5.2mm; }
td.wn { text-align: left; padding-left: 1.5mm; font-size: 8pt; }
td.tot { font-weight: bold; font-variant: small-caps; }
table.nt { flex: 1; }
table.nt td.rh { vertical-align: middle; }
td.notes { text-align: left; padding: 1mm 1.5mm; font-size: 7.6pt;
           line-height: 1.35; vertical-align: top; }
'''


def main():
    out = ['<!doctype html><html><head><meta charset="utf-8">',
           '<title>Space Crusade Space Marines — Advanced HeroQuest '
           'character sheets</title><style>%s</style></head><body>' % CSS]
    for i in range(0, len(CARDS), 2):
        out.append('<div class="page">')
        out.append(card(CARDS[i]))
        if i + 1 < len(CARDS):
            out.append(card(CARDS[i + 1]))
        out.append('</div>')
    out.append('</body></html>')

    dest = os.path.join('tables', 'sentinel', 'space-marine-sheets.html')
    io.open(dest, 'w', encoding='utf-8', newline='\n').write('\n'.join(out))
    print('%s  (%d cards, %d pages)'
          % (dest, len(CARDS), (len(CARDS) + 1) // 2))


if __name__ == '__main__':
    main()
