"""Generate printable fantasy monster reference cards from the fantasy pack."""
import argparse
import html
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def render():
    pack = json.loads((ROOT / 'data/packs/fantasy.json').read_text(encoding='utf-8'))
    css = """@page{size:A4 portrait;margin:7mm}*{box-sizing:border-box}body{font-family:Georgia,serif}
.page{width:196mm;height:283mm;display:grid;gap:3mm;grid-template-columns:repeat(2,1fr);grid-template-rows:repeat(4,1fr);break-after:page}
.card{border:1.5px solid;padding:2mm;overflow:hidden}h2{margin:0 0 2mm;font-size:11pt}pre{white-space:pre-wrap;font:7pt/1.15 monospace;margin:0}"""
    out = ['<!doctype html><meta charset="utf-8"><title>Fantasy monster reference sheets</title>',
           f'<style>{css}</style>']
    monsters = pack['monsters']
    for start in range(0, len(monsters), 8):
        out.append('<section class="page">')
        for monster in monsters[start:start + 8]:
            reference = '\n'.join(line.rstrip() for line in monster['text'].splitlines())
            out.append('<article class="card"><h2>' + html.escape(monster['name']) +
                       '</h2><pre>' + html.escape(reference) + '</pre></article>')
        out.append('</section>')
    return '\n'.join(out) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'tables/ahq-monster-sheets.html')
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render(), encoding='utf-8', newline='\n')
    print(f'{args.output} (64 cards)')


if __name__ == '__main__':
    main()
