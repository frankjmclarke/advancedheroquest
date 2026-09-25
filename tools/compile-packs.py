"""Compile editable JSON character packs to bounded portable HQPACK1 resources.

No Python is needed to play. Run after editing data/packs/*.json; --check
verifies the shipped resources and the built-in fantasy fallback.
"""
import argparse
import json
import re
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fantasy_monsters():
    """Adapt the existing single-source printed catalogue without changing it."""
    from ahq_monsters import PAGES, reference
    result = []
    for page, cards in enumerate(PAGES, 1):
        for slot, card in enumerate(cards, 1):
            text = reference(card, page, slot)
            slug = re.sub('[^a-z0-9]+', '-', card['name'].lower()).strip('-')
            melee = dict(weapon='', dice=0, hit=[0]*12)
            ranged = dict(weapon='', kind=0, range=0, dice=0, hit=[0]*5)
            if all(v.isdecimal() and 1 <= int(v) <= 12 for v in card['hits']) and card['damage'].isdecimal() and 1 <= int(card['damage']) <= 99:
                melee = dict(weapon='Printed melee profile', dice=int(card['damage']), hit=list(map(int, card['hits'])))
            rr = card['ranged']
            if rr[5].isdecimal() and rr[6].isdecimal() and 1 <= int(rr[5]) <= 480 and 1 <= int(rr[6]) <= 99:
                ran = int(rr[5])
                hits = [int(v.rstrip('*')) for start, v in zip([1,4,13,25,37], rr[:5])
                        if re.fullmatch(r'\d+\*?', v) and 1 <= int(v.rstrip('*')) <= 12 and not ('*' in v and start <= ran)]
                if len(hits) == 5:
                    ranged = dict(weapon='Printed ranged profile', kind=2 if 'crossbow' in text else 1 if 'bow' in text else 4,
                                  range=ran, dice=int(rr[6]), hit=hits)
            result.append(dict(id=f'fantasy:{slug}-{page}-{slot}', name=card['name'],
                               aliases=[a.lower() for a in card['aliases']], text=text,
                               stats=[int(v) if v.isdecimal() else 0 for v in card['stats']],
                               melee=melee, ranged=ranged))
    return result


def compile_pack(pack):
    out = bytearray(b'HQPACK1\n')

    def number(n, low=0, high=99):
        if type(n) is not int or not low <= n <= high:
            raise ValueError(f'Invalid integer: {n}')
        out.extend(struct.pack('<I', n))

    def string(s, limit):
        b = s.encode('cp1252')
        if len(b) > limit or b'\0' in b:
            raise ValueError(f'Invalid string: {s[:80]}')
        out.extend(struct.pack('<I', len(b)))
        out.extend(b)

    if pack['version'] != 1:
        raise ValueError('Unsupported pack version')
    if not re.fullmatch(r'[a-z0-9][a-z0-9-]*', pack['id']) or not pack['name']:
        raise ValueError('A pack needs a simple lowercase ID and a display name')
    string(pack['id'], 63)
    string(pack['name'], 95)
    string('|'.join(pack.get('nonmonsters', [])), 8191)
    number(pack.get('legacy_references', 0), 0, 1)
    number(len(pack['heroes']), 1, 256)
    number(len(pack['monsters']), 0, 256)
    ids = set()
    for index, p in enumerate(pack['heroes'] + pack['monsters']):
        hero = index < len(pack['heroes'])
        if p['id'] in ids or not p['id'].startswith(pack['id'] + ':'):
            raise ValueError('Duplicate or unqualified profile ID: ' + p['id'])
        ids.add(p['id'])
        string(p['id'], 63)
        if not p['name']:
            raise ValueError('A profile needs a display name')
        aliases = p.get('aliases', [p['name'].lower()])
        if not aliases or any(not a or '|' in a or a != a.lower() for a in aliases) or len(aliases[0].encode('cp1252')) > 63:
            raise ValueError('Use explicit lowercase aliases with a short canonical first alias')
        string(p['name'], 63)
        string('|'.join(aliases), 1023)
        string(p['text'], 2047 if hero else 4095)
        if len(p['stats']) != 9:
            raise ValueError('Expected nine stats')
        for i, v in enumerate(p['stats']):
            number(v, 0, 9999 if i == 8 and not hero else 99)
        if (hero or not pack.get('legacy_references')) and not p['stats'][7]:
            raise ValueError('A character needs positive maximum Wounds')
        number(p.get('kind', 0), 0, 8)
        number(p.get('fate', 0))
        number(p.get('unique', 0), 0, 1)
        m, r = p['melee'], p['ranged']
        string(m['weapon'], 39)
        number(m['dice'])
        if len(m['hit']) != 12 or len(r['hit']) != 5:
            raise ValueError('Incorrect combat row length')
        for v in m['hit']:
            number(v, 0, 12)
        string(r['weapon'], 39)
        number(r['kind'], 0, 4)
        number(r['range'], 0, 480)
        number(r['dice'])
        for v in r['hit']:
            number(v, 0, 12)
    return bytes(out)


def outputs():
    for source in sorted((ROOT / 'data/packs').glob('*.json')):
        pack = json.loads(source.read_text(encoding='utf-8'))
        if pack.get('monster_source') == 'ahq-monsters':
            pack['monsters'] = fantasy_monsters()
        data = compile_pack(pack)
        yield ROOT / 'tables' / pack['id'] / 'characters.hqp', data
        if pack['id'] == 'fantasy':
            lines = ['/* Generated by tools/compile-packs.py. */',
                     'static const unsigned char fantasy_pack_bytes[] = {']
            lines += [','.join(str(v) for v in data[i:i+32]) + ','
                      for i in range(0, len(data), 32)]
            lines += ['};', 'const char *hero_classes[HERO_CLASS_COUNT] = {',
                      ','.join(json.dumps(h['name']) for h in pack['heroes']), '};', '']
            yield ROOT / 'pack-fantasy-data.h', '\n'.join(lines).encode('ascii')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, data in list(outputs()):
        if args.check:
            if not path.exists() or path.read_bytes() != data:
                raise SystemExit(f'Stale pack resource: {path}')
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        print(path.relative_to(ROOT))
