"""Generate/check the checked-in C reference table; Python is not needed to build or run HQ-Map."""
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def c_header():
    pack = json.loads((ROOT / 'data/packs/fantasy.json').read_text(encoding='utf-8'))
    lines = ['/* Generated from data/packs/fantasy.json. */',
             'typedef struct { const char *aliases; const char *text; } AHQ_REFERENCE;',
             'static const AHQ_REFERENCE ahq_references[] = {']
    for monster in pack['monsters']:
        lines.append('  { ' + json.dumps('|'.join(monster['aliases'])) + ',')
        lines.append('    ' + json.dumps(monster['text']) + ' },')
    lines += ['};', '#define AHQ_REFERENCE_COUNT (sizeof(ahq_references) / sizeof(ahq_references[0]))', '']
    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='Fail if the generated header is stale')
    args = parser.parse_args()
    dest = ROOT / 'ahq-reference-data.h'
    text = c_header()
    if args.check:
        if not dest.exists() or dest.read_text(encoding='ascii') != text:
            parser.exit(1, 'AHQ reference data is stale. Run python tools/generate-ahq-reference.py\n')
        print('AHQ reference data is up to date.')
    else:
        dest.write_text(text, encoding='ascii', newline='\n')
        print(dest)


if __name__ == '__main__':
    main()
