"""Generate printable AHQ monster cards from AHQ - Monsters 1.pdf.

    python tools/ahq-monster-sheet.py
    python tools/ahq-monster-sheet.py --output path/to/cards.html

The source is an eight-page image-only scan. The 64 cards in data/ahq-monsters.json were visually
transcribed, in page order and then left-to-right/top-to-bottom. Generation
requires only Python's standard library, not the PDF, OCR, or a PDF library.

The JSON is also used to generate the application reference table.
Values are the printed card values, NOT recalculated game rules. This includes
duplicate names with different profiles, blank cells, asterisks, expressions
such as 8+2, and anomalous hit rows. No miniature images are copied.
"""

import argparse
from html import escape
from pathlib import Path


from ahq_monsters import SOURCE, PAGES, STAT_LABELS, BANDS, validate


CSS = """
@page { size: A4 portrait; margin: 7mm; }
* { box-sizing: border-box; }
body { margin: 0; color: #111; font-family: Georgia, 'Times New Roman', serif; }
.intro { max-width: 196mm; margin: 15px auto; font: 14px/1.5 sans-serif; }
.page { width: 196mm; height: 283mm; display: grid; gap: 3mm;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  grid-template-rows: repeat(4, minmax(0, 1fr)); break-after: page; }
.page:last-child { break-after: auto; }
.card { border: 1.5px solid #111; padding: 1.5mm; min-width: 0;
  display: flex; flex-direction: column; break-inside: avoid; }
.heading { height: 9mm; flex-shrink: 0; display: flex; flex-direction: column;
  justify-content: center; }
h2 { margin: 0; font-size: 10pt; line-height: 1.05; }
.source { font: 6.5pt/1.25 sans-serif; color: #555; margin-top: .5mm; }
table { border-collapse: collapse; table-layout: fixed; width: 100%; }
td, th { border: 1px solid #444; text-align: center; padding: 0; height: 4mm;
  font-size: 8pt; font-weight: normal; }
th { font-size: 6.7pt; background: #eee; }
.stats td { font-weight: bold; font-size: 9pt; }
.bar { background: #252525; color: white; text-align: center; font-weight: bold;
  font-size: 7pt; line-height: 3.5mm; height: 3.5mm; margin-top: 1mm; }
.label { width: 10mm; font-size: 6pt; line-height: 1; }
.damage { width: 10mm; }
.combat td { font-weight: bold; }
.notes { font-size: 7.5pt; line-height: 1.15; padding: 1mm .4mm 0; }
@media screen {
 body { background: #ececec; padding: 12px; }
 .page { margin: 0 auto 20px; background: white; box-shadow: 0 2px 10px #bbb; }
}
@media print {
 .intro { display: none; }
 * { print-color-adjust: exact; -webkit-print-color-adjust: exact; }
}
"""


def cells(values, tag="td"):
    return "".join(f"<{tag}>{escape(str(v))}</{tag}>" for v in values)


def card(m, page, slot):
    ranged = m["ranged"]
    return f'''<article class="card" id="p{page}-c{slot}">
<header class="heading"><h2>{escape(m['name'])}</h2>
<div class="source">{SOURCE} · page {page}, card {slot}</div></header>
<table class="stats" aria-label="Characteristics"><tr>{cells(STAT_LABELS, 'th')}</tr>
<tr>{cells(m['stats'])}</tr></table>
<div class="bar">Hand-to-Hand Combat</div>
<table class="combat" aria-label="Hand-to-hand hit rolls">
<tr><th class="label">Target WS</th>{cells(range(1, 13), 'th')}<th class="damage">Dam. dice</th></tr>
<tr><th class="label">Hit roll</th>{cells(m['hits'])}<td>{escape(m['damage'])}</td></tr></table>
<div class="bar">Ranged Combat</div>
<table class="combat" aria-label="Ranged hit rolls">
<tr><th class="label">Range</th>{cells(BANDS, 'th')}<th>Max range</th><th>Dam. dice</th></tr>
<tr><th class="label">Hit roll</th>{cells(ranged)}</tr></table>
<div class="bar">Equipment / Notes</div><div class="notes">{escape(m['notes'])}</div>
</article>'''


def render():
    validate()
    out = ['<!doctype html><html lang="en"><head><meta charset="utf-8">',
           '<meta name="viewport" content="width=device-width, initial-scale=1">',
           '<title>Advanced HeroQuest - monster reference sheets</title>',
           f'<style>{CSS}</style></head><body>',
           '<div class="intro"><strong>Advanced HeroQuest monster reference sheets</strong><br>',
           '64 cards transcribed from AHQ - Monsters 1.pdf. Print at 100% on A4 portrait ',
           'with browser headers/footers off. Source variants and printed combat rows are ',
           'preserved; blanks are not inferred, and * refers to the source special rules. ',
           'The White Skaven Sorcerer damage cell is blank in the scan. ',
           'Card numbers run left-to-right, then top-to-bottom on each source page.</div>']
    for page_number, page in enumerate(PAGES, 1):
        out.append(f'<section class="page" aria-label="Source page {page_number}">')
        out.extend(card(m, page_number, slot) for slot, m in enumerate(page, 1))
        out.append('</section>')
    out.append('</body></html>')
    return '\n'.join(out) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path,
                        default=Path(__file__).resolve().parents[1] / 'tables' / 'ahq-monster-sheets.html')
    args = parser.parse_args()
    html = render()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(html, encoding='utf-8', newline='\n')
    print(f'{args.output} (64 cards, 8 A4 pages)')


if __name__ == '__main__':
    main()
