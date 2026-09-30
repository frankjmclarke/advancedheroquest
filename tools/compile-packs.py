"""Compile editable JSON character packs to bounded portable HQPACK6 resources.

No Python is needed to play. Run after editing data/packs/*.json; --check
verifies the shipped resources and the built-in fantasy fallback.
"""
import argparse
import json
import re
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def compile_pack(pack):
    out = bytearray(b'HQPACK6\n')

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
        critical = m.get('critical', 12)
        fumble = m.get('fumble', 1)
        if type(critical) is not int or type(fumble) is not int or not 1 <= fumble < critical <= 12:
            raise ValueError(f'Invalid melee thresholds for {p["id"]}')
        number(critical, 1, 12)
        number(fumble, 1, 12)
        reach = m.get('reach', 2 if m.get('diagonal', 0) else 1)
        number(reach, 1, 2)
        string(r['weapon'], 39)
        number(r['kind'], 0, 4)
        number(r['range'], 0, 480)
        number(r['dice'])
        for v in r['hit']:
            number(v, 0, 12)
        critical = r.get('critical', 12)
        fumble = r.get('fumble', 1)
        if type(critical) is not int or type(fumble) is not int or not 1 <= fumble < critical <= 12:
            raise ValueError(f'Invalid ranged thresholds for {p["id"]}')
        number(critical, 1, 12)
        number(fumble, 1, 12)
    # Spell definitions belong to this same pack; IDs are never matched by name.
    components, spells, books = (pack.get(k, []) for k in ('components', 'spells', 'spellbooks'))
    def catalogue(items, limit, kind):
        if len(items) > limit:
            raise ValueError(f'Too many {kind}')
        result = {}
        for i, item in enumerate(items):
            ident = item['id']
            if ident in ids or not ident.startswith(pack['id'] + ':') or not item['name']:
                raise ValueError(f'Invalid or duplicate {kind} ID: {ident}')
            ids.add(ident); result[ident] = i
        return result
    ci = catalogue(components, 32, 'component')
    si = catalogue(spells, 64, 'spell')
    bi = catalogue(books, 16, 'spellbook')
    def reference(mapping, ident, owner):
        if ident not in mapping:
            raise ValueError(f'{owner}: missing reference {ident}')
        return mapping[ident]
    def stock(mapping, owner):
        values = [0] * len(components)
        for ident, quantity in mapping.items():
            index = reference(ci, ident, owner)
            if type(quantity) is not int or not 1 <= quantity <= 9999:
                raise ValueError(f'{owner}: invalid component quantity')
            values[index] = quantity
        return values
    number(len(components), 0, 32); number(len(spells), 0, 64); number(len(books), 0, 16)
    for c in components:
        string(c['id'], 63); string(c['name'], 63); number(c.get('price', 0), 0, 100000)
    effects = {'manual': 0, 'damage': 1, 'heal': 2, 'armour': 3, 'spy': 4, 'door': 5, 'hand': 6, 'flight': 7, 'swift': 8, 'resurrect': 9, 'still': 10, 'courage': 11, 'life':12, 'strength':13, 'cloak':14, 'blind':15, 'regen':16, 'fear':17, 'sleep':18, 'restore':19, 'learning':20, 'banish':21, 'escape':22, 'venom':23}
    targets = {'manual': 0, 'model': 1, 'template': 2, 'healing': 3, 'touch': 4, 'section': 5, 'hidden_section': 6, 'wall': 7, 'self': 8, 'corpse': 9, 'group': 10, 'closed_door':11}
    for sp in spells:
        string(sp['id'], 63); string(sp['name'], 63); string(sp['text'], 2047)
        if sp['effect'] not in effects or sp['target'] not in targets:
            raise ValueError(f'{sp["id"]}: unsupported spell effect or target')
        number(effects[sp['effect']], 0, 23); number(targets[sp['target']], 0, 11)
        number(sp.get('range', 0), 0, 480); number(sp.get('dice', 0), 0, 99)
        number(sp.get('intelligence_test', 0), 0, 1); number(sp.get('failed_dice', 0), 0, 99)
        costs = stock(sp.get('components', {}), sp['id'])
        if sum(costs) >= 2 and not sp.get('stationary', 1):
            raise ValueError(f'{sp["id"]}: multiple components require stationary casting')
        number(sp.get('stationary', int(sum(costs) >= 2)), 0, 1)
        number(sp.get('learning_cost', 0), 0, 100000)
        if sp['effect'] == 'damage' and (not sp.get('dice') or sp['target'] not in ('model', 'template')):
            raise ValueError(f'{sp["id"]}: damage needs dice and model/template targeting')
        if sp['effect'] == 'heal' and sp.get('intelligence_test', 0):
            raise ValueError(f'{sp["id"]}: Intelligence-tested healing is not supported yet')
        if sp['effect'] == 'heal' and sp['target'] != 'healing':
            raise ValueError(f'{sp["id"]}: healing needs healing targeting')
        required = {'armour':'touch','spy':'hidden_section','door':'wall','hand':'self','flight':'model','swift':'group','resurrect':'corpse','still':'section','courage':'touch','life':'healing','strength':'touch','cloak':'self','blind':'self','regen':'model','fear':'self','sleep':'model','restore':'corpse','learning':'closed_door','banish':'model','escape':'self','venom':'self'}
        if sp['effect'] in required and sp['target'] != required[sp['effect']]:
            raise ValueError(f'{sp["id"]}: effect requires {required[sp["effect"]]} targeting')
        if sp['effect'] in ('resurrect','restore') and not sp.get('intelligence_test'):
            raise ValueError(f'{sp["id"]}: resurrection requires an Intelligence test')
        for qty in costs: number(qty, 0, 9999)
    for book in books:
        string(book['id'], 63); string(book['name'], 63)
        available = {reference(si, x, book['id']) for x in book['spells']}
        starting = {reference(si, x, book['id']) for x in book.get('starting_spells', [])}
        if len(available) != len(book['spells']) or len(starting) != len(book.get('starting_spells', [])) or not starting <= available:
            raise ValueError(f'{book["id"]}: duplicate spells or starting spell outside book')
        for i in range(len(spells)): number(int(i in available), 0, 1)
        for i in range(len(spells)): number(int(i in starting), 0, 1)
    for profile in pack['heroes'] + pack['monsters']:
        caster = profile.get('caster', {})
        membership = {reference(bi, x, profile['id']) for x in caster.get('books', [])}
        if len(membership) != len(caster.get('books', [])):
            raise ValueError(f'{profile["id"]}: duplicate book')
        for i in range(len(books)): number(int(i in membership), 0, 1)
        number(caster.get('starting_components', 0), 0, 9999)
        quantities = stock(caster.get('components', {}), profile['id'])
        if not membership and (any(quantities) or caster.get('starting_components', 0)):
            raise ValueError(f'{profile["id"]}: ingredients without a spellbook')
        for qty in quantities: number(qty, 0, 9999)
    for sp in spells:
        template = sp.get('template', {})
        if not isinstance(template, dict) or set(template) - {'width', 'height'}:
            raise ValueError(f'{sp["id"]}: template must contain width and height only')
        if sp['target'] == 'template':
            number(template.get('width', 2), 1, 12)
            number(template.get('height', 2), 1, 12)
        else:
            if template:
                raise ValueError(f'{sp["id"]}: template geometry requires template targeting')
            number(0, 0, 0); number(0, 0, 0)
    for profile in pack['heroes'] + pack['monsters']:
        traits = profile.get('traits', [])
        flags = {'undead':1, 'lesser_daemon':2, 'greater_daemon':4}
        if any(t not in flags for t in traits) or len(set(traits)) != len(traits) or len(traits)>1:
            raise ValueError('Invalid creature traits: ' + profile['id'])
        number(sum(flags[t] for t in traits), 0, 4)
    return bytes(out)


def outputs():
    for source in sorted((ROOT / 'data/packs').glob('*.json')):
        pack = json.loads(source.read_text(encoding='utf-8'))
        data = compile_pack(pack)
        yield ROOT / 'tables' / pack['id'] / 'characters.hqp', data
        if pack['id'] == 'fantasy':
            lines = ['/* Generated by tools/compile-packs.py. */',
                     'static const unsigned char fantasy_pack_bytes[] = {']
            lines += [','.join(str(v) for v in data[i:i+32]) + ','
                      for i in range(0, len(data), 32)]
            lines += ['};', 'const char *hero_classes[HERO_CLASS_COUNT] = {',
                      ','.join(json.dumps(h['name']) for h in pack['heroes'][:9]), '};', '']
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
