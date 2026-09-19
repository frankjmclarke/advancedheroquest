"""Prepare embedded corridor and room sprites from board scans (Pillow).

Run from any directory. Normal builds use the checked-in BMPs and need no
Python or external Tiles directory. Regeneration needs Pillow and ImageMagick
(`magick` on PATH). Corridor coordinates refer to Floors 3.
Only complete interior squares are sampled, avoiding cardboard tabs and the
white scanner background. Original bitmap perimeter pixels preserve exits.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageChops, ImageOps
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
CELL = 64
OUTPUT_CELL = 32
KINDS = ('passage', 'deadend', 'lturn', 'rturn', 'tjunct', 'corner')

# Floor quadrilaterals in original scan pixels, ordered UL, LL, LR, UR.
# Excluding the printed border keeps each floor square on the 64-pixel grid.
# Small scan skew is corrected before the four orientations are prepared.
ROOMS = (
    (2, (150, 154, 150, 1644, 1655, 1644, 1660, 154), 5),
    (2, (150, 1770, 150, 3240, 1647, 3240, 1655, 1770), 5),
    (3, (174, 123, 181, 1596, 1673, 1584, 1670, 114), 5),
    (3, (181, 1734, 193, 3194, 1682, 3188, 1680, 1730), 5),
    (4, (130, 146, 124, 3210, 1638, 3210, 1645, 146), 10),
    (6, (124, 182, 145, 3228, 1650, 3220, 1639, 175), 10),
)


def preserve_walls(tile, old):
    """Keep the same outer wall/exit footprint as the original bitmap."""
    w, h = old.size
    draw = ImageDraw.Draw(tile)
    for y in range(h):
        for x in range(w):
            if (x in (0, w - 1) or y in (0, h - 1)) and old.getpixel((x, y)) < 128:
                draw.rectangle((x * 8, y * 8, x * 8 + 7, y * 8 + 7), fill=(48, 35, 29))


def generate_rooms(out):
    rotations = (None, Image.Transpose.ROTATE_180,
                 Image.Transpose.ROTATE_270, Image.Transpose.ROTATE_90)
    for index, (sheet, quad, rows) in enumerate(ROOMS):
        scan = Image.open(ROOT / f'Tiles/Advanced Heroquest - Rooms - {sheet}.jpg').convert('RGB')
        # Rectify at double resolution, then downsample for clean scan detail.
        floor = scan.transform((5 * CELL * 2, rows * CELL * 2), Image.Transform.QUAD,
                               quad, Image.Resampling.BICUBIC).resize(
                                   (5 * CELL, rows * CELL), Image.Resampling.LANCZOS)
        for direction, rotation in zip('nsew', rotations):
            tile = floor.copy() if rotation is None else floor.transpose(rotation)
            kind = 'hazard' if rows == 5 else 'large'
            old = Image.open(ROOT / f'rsh/{kind}{direction}.bmp').convert('L')
            assert tile.size == (old.width * 8, old.height * 8)
            preserve_walls(tile, old)
            tile.save(out / f'room{index}{direction}.bmp')


def generate_features(out):
    """Bake features onto board floors, using original symbols as fallback.

    Resource IDs, rather than filename suffixes, preserve several unusual
    direction orderings in the original resource set. Some old BMPs include
    extra padding, so crop to the logical piece size just as draw_pice does.
    """
    rh = (ROOT / 'rsh/grafic.rh').read_text()
    ids = {name: int(value) for name, value in re.findall(r'#define\s+(\w+)\s+(\d+)', rh)}
    rc = (ROOT / 'rsh/grafic.rc').read_text()
    originals = {ids[name]: ROOT / 'rsh' / filename for name, filename in
                 re.findall(r'(\w+)\s+BITMAP\s+"([^"\n]+)"', rc) if name in ids}
    # (first original resource, width and height in map squares)
    groups = [(13, 5, 10), (17, 5, 10), (45, 2, 2), (49, 2, 2), (65, 15, 15)]
    groups += [(n, 5, 5) for n in range(69, 89, 4)]
    groups += [(n, 5, 5) for n in range(93, 177, 4)]
    groups += [(177, 10, 10), (181, 5, 5), (185, 5, 10)]
    rotations = (None, Image.Transpose.ROTATE_180,
                 Image.Transpose.ROTATE_270, Image.Transpose.ROTATE_90)
    tokens = Image.open(ROOT / 'Tiles/Advanced Heroquest - Tiles - 1.jpg').convert('RGB')
    terror = Image.open(ROOT / 'Tiles/Terror in the Dark - Items - 3.jpg').convert('RGB')
    pit = tokens.crop((1112, 1535, 1424, 2170))
    rope = tokens.crop((2160, 2272, 2684, 2924)).convert('RGBA')
    # The rope bridge is a shaped cardboard cutout on the blue sheet.
    rope.putdata([(r, g, b, 0 if b > r * 1.15 and b > g * 1.1 else 255)
                  for r, g, b, a in rope.getdata()])
    art = {
        'grate': tokens.crop((1755, 1535, 2070, 1848)),
        'trap': tokens.crop((1755, 1858, 2070, 2170)),
        'chest': tokens.crop((1755, 2265, 2070, 2580)),
        'throne': tokens.crop((2080, 1538, 2720, 2170)),
        'bridge': rope,
        'pit': pit,
        'chasm': pit.transpose(Image.Transpose.ROTATE_90),
        'portcullis': tokens.crop((1150, 2265, 1430, 2915)),
        'rats': terror.crop((790, 100, 1080, 380)),
        'bats': terror.crop((455, 115, 740, 393)),
        'slime': terror.crop((1868, 94, 2147, 366)),
        'mould': terror.crop((450, 486, 744, 769)),
        'fungi': terror.crop((94, 487, 375, 765)),
    }
    for name in ('up', 'down'):
        # Keep the landing as well as the steps, so the stair direction is
        # still readable when fitted to the application's 2x2 footprint.
        art[name] = Image.open(ROOT / f'Tiles/escalier-{name}-01-4293e{"5e" if name == "up" else "68"}.png').convert('RGB').transpose(Image.Transpose.ROTATE_270)
    decorated = Image.open(ROOT / 'Tiles/Advanced Heroquest - Rooms - 5.jpg').convert('RGB')
    decorated = decorated.transform((320, 640), Image.Transform.QUAD,
        (100, 157, 120, 3199, 1603, 3199, 1600, 161), Image.Resampling.BICUBIC)

    def floor(w, h):
        if (w, h) == (5, 10):
            return Image.open(out / 'room4n.bmp').convert('RGB')
        tile = Image.new('RGB', (w * CELL, h * CELL))
        # Remove prepared wall strips before assembling larger rooms. Cropping
        # and extending their nearest floor pixels avoids internal black walls.
        for yy in range(0, h * CELL, 5 * CELL):
            for xx in range(0, w * CELL, 5 * CELL):
                patch = Image.open(out / f'room{(xx // CELL + yy // CELL) % 4}n.bmp').convert('RGB')
                patch = patch.crop((8, 8, 312, 312))
                patch = ImageOps.expand(patch, 8)
                patch.paste(patch.crop((8, 8, 9, 312)).resize((8, 304)), (0, 8))
                patch.paste(patch.crop((311, 8, 312, 312)).resize((8, 304)), (312, 8))
                patch.paste(patch.crop((0, 8, 320, 9)).resize((320, 8)), (0, 0))
                patch.paste(patch.crop((0, 311, 320, 312)).resize((320, 8)), (0, 312))
                tile.paste(patch, (xx, yy))
        return tile

    # Token positions use the original North-facing symbol footprints, in
    # the old 8-pixel-per-square coordinates. Number placement stays in C.
    placements = {
        13: [('chest', (17, 17, 23, 23)), ('down', (15, 55, 25, 65))],
        17: [('chest', (17, 17, 23, 23))],
        69: [('chasm', (2, 16, 38, 32)), ('bridge', (16, 16, 24, 32))],
        73: [('chasm', (2, 16, 38, 32)), ('chest', (32, 2, 38, 8))],
        81: [('grate', (15, 15, 25, 25))],
        93: [('trap', (15, 15, 25, 25))],
        97: [('throne', (19, 2, 29, 11))],
        101: [('fungi', (14, 14, 26, 26))],
        105: [('mould', (14, 14, 26, 26))],
        109: [('rats', (14, 14, 26, 26))],
        113: [('bats', (14, 14, 26, 26))],
        125: [('slime', (14, 18, 26, 30))],
        129: [('pit', (12, 10, 28, 30))],
        145: [('chest', (32, 2, 38, 8))],
        149: [('down', (15, 7, 25, 17)), ('chest', (17, 25, 23, 31))],
    }
    mapping, resources = [], []
    chest_glyph = Image.open(originals[17]).convert('L').crop((17, 17, 23, 23))
    for first, cols, rows in groups:
        for direction, rotation in enumerate(rotations):
            ow, oh = (cols * 8, rows * 8) if direction < 2 else (rows * 8, cols * 8)
            old_id = first + direction
            old = Image.open(originals[old_id]).convert('L').crop((0, 0, ow, oh))
            if first in (45, 49):
                tile = art['up' if first == 45 else 'down'].resize((128, 128), Image.Resampling.LANCZOS)
            elif first == 85:
                tile = decorated.crop((0, 128, 320, 448))
            elif first == 13:
                tile = decorated.copy()
            else:
                tile = floor(cols, rows)
            if rotation is not None:
                tile = tile.transpose(rotation)

            if first not in (45, 49, 65, 85, 177) and first not in placements:
                baseline_id = (9 if rows == 10 else 25) + direction
                baseline = Image.open(originals[baseline_id]).convert('L').crop((0, 0, ow, oh))
                bbox = ImageChops.difference(old, baseline).getbbox()
                if bbox:
                    # A parchment badge preserves every bit of an unmatched
                    # symbol, including white interior details and lettering.
                    symbol = ImageOps.colorize(old.crop(bbox), '#46321e', '#edce99')
                    symbol = symbol.resize((symbol.width * 8, symbol.height * 8), Image.Resampling.NEAREST)
                    tile.paste(symbol, (bbox[0] * 8, bbox[1] * 8))

            # Rockfall and slime also contain a treasure chest. Preserve
            # their hazard badge but replace the chest in its actual
            # bitmap position (legacy direction ordering is irregular).
            if first in (121, 125):
                found = False
                for turn in rotations:
                    glyph = chest_glyph if turn is None else chest_glyph.transpose(turn)
                    for gy in range(oh - 5):
                        for gx in range(ow - 5):
                            if ImageChops.difference(old.crop((gx, gy, gx + 6, gy + 6)), glyph).getbbox() is None:
                                patch = art['chest'].resize((48, 48), Image.Resampling.LANCZOS)
                                if turn is not None:
                                    patch = patch.transpose(turn)
                                tile.paste(patch, (gx * 8, gy * 8))
                                found = True
                if not found:
                    raise RuntimeError(f'Chest symbol missing from legacy feature {old_id}')

            for name, rect in placements.get(first, []):
                x1, y1, x2, y2 = [v * 8 for v in rect]
                patch = art[name].resize((x2 - x1, y2 - y1), Image.Resampling.LANCZOS)
                if rotation is not None:
                    patch = patch.transpose(rotation)
                W, H = cols * CELL, rows * CELL
                dest = ((x1, y1), (W - x2, H - y2), (H - y2, x1), (y1, W - x2))[direction]
                if patch.mode == 'RGBA':
                    tile.paste(patch, dest, patch.getchannel('A'))
                else:
                    tile.paste(patch, dest)
            preserve_walls(tile, old)
            assert tile.size == (ow * 8, oh * 8)
            # Local palettes keep the embedded artwork compact without
            # changing its pixel dimensions or requiring a PNG decoder.
            tile.quantize(colors=256, dither=Image.Dither.NONE).save(out / f'feature{old_id}.bmp')
            resource_id = 500 + len(mapping)
            mapping.append(f'    {{{old_id}, {resource_id}}}')
            resources.append(f'{resource_id} BITMAP "board/feature{old_id}.bmp"')
    # A trap annotation, not an ordinary door replacement. Table output has
    # no exact trap square; the GM renderer places this badge within its piece.
    tile = art['portcullis'].transpose(Image.Transpose.ROTATE_90).resize((CELL * 2, CELL), Image.Resampling.LANCZOS)
    tile.save(out / 'portcullis-trap.bmp')
    resources.append('700 BITMAP "board/portcullis-trap.bmp"')
    for direction in range(4):
        (out / f'portcullis{direction}.bmp').unlink(missing_ok=True)
    (ROOT / 'rsh/board-features.rh').write_text(
        '/* Generated by tools/prepare-board-tiles.py. */\n'
        '#define BOARD_FEATURE_RESOURCES { \\\n' + ', \\\n'.join(mapping) + ' \\\n}\n', encoding='ascii')
    (ROOT / 'rsh/board-features.rc').write_text(
        '/* Generated by tools/prepare-board-tiles.py. */\n' + '\n'.join(resources) + '\n', encoding='ascii')


def generate():
    magick = shutil.which('magick')
    if magick is None:
        raise SystemExit('ImageMagick is required to prepare compact assets: add magick to PATH.')
    scan = Image.open(ROOT / 'Tiles/Advanced Heroquest - Floors - 3.jpg').convert('RGB')
    # Three complete columns in the second horizontal strip; straighten each
    # square independently to line up the board grid with the map grid.
    xs = (1135, 1455, 1777, 2099)
    ys = (839, 1157, 1471)
    cells = [scan.crop((xs[x], ys[y], xs[x + 1], ys[y + 1])).resize(
        (CELL, CELL), Image.Resampling.LANCZOS) for y in range(2) for x in range(3)]
    out = ROOT / 'rsh/board'
    out.mkdir(exist_ok=True)
    for kind_index, kind in enumerate(KINDS):
        for direction_index, direction in enumerate('nsew'):
            old = Image.open(ROOT / f'rsh/{kind}{direction}.bmp').convert('L')
            w, h = old.size
            tile = Image.new('RGB', (w * 8, h * 8))
            for y in range(h // 8):
                for x in range(w // 8):
                    tile.paste(cells[(x + y * 3 + kind_index + direction_index) % len(cells)],
                               (x * CELL, y * CELL))
            preserve_walls(tile, old)
            tile.save(out / f'{kind}{direction}.bmp')
    generate_rooms(out)
    generate_features(out)
    # Always rebuild from the scans at full working resolution first, then
    # compact once. Re-running this script never progressively shrinks assets.
    subprocess.run([magick, 'mogrify', '-filter', 'Lanczos', '-resize',
                    f'{OUTPUT_CELL * 100 // CELL}%', '-colors', '256',
                    '-type', 'Palette', '-depth', '8', '-compress', 'None',
                    '-define', 'bmp:format=bmp3',
                    *[str(p) for p in sorted(out.glob('*.bmp'))]], check=True)
    for path in out.glob('*.bmp'):
        with Image.open(path) as image:
            if image.mode != 'P' or image.width % OUTPUT_CELL or image.height % OUTPUT_CELL:
                raise RuntimeError(f'Invalid compact board asset: {path}')
    print(f'Prepared compact board artwork at {OUTPUT_CELL} pixels per square.')


if __name__ == '__main__':
    generate()
