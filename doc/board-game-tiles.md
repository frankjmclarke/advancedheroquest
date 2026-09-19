# Board-game tile display

Choose **View → Enhanced board-game tiles** to switch both the Graphic Map
and Player View to scanned board-game artwork. Uncheck it to return
to the original bitmap view. Enhanced tiles are the default; your saved choice is
remembered with the program options.

The enhanced view covers straight corridors, dead ends, turns and junctions,
plus plain 5×5 and 5×10 rooms. Small rooms have four floor designs; large rooms
have two. Each room keeps the same design when you scroll, zoom, or reveal it.
Room artwork rotates to fit horizontal and vertical layouts.

Stairs, pits, chasms, bridges, grates, trapdoors, thrones and chests use matching
scanned artwork. Magic-circle rooms and quest rooms use the decorated room
floor. Rats, bats, slime, mould and mushrooms use their tokens from
`Terror in the Dark - Items - 3.jpg`. Other special features (including furniture, pools and
tombs) retain their familiar symbols on a parchment badge over the enhanced
floor. The 10×10 and 15×15 rooms assemble floor panels without internal walls.

The pit, rope bridge, throne, grate, trapdoor and treasure chest artwork comes
from `Advanced Heroquest - Tiles - 1.jpg`. Chasm rooms use the pit artwork;
bridge rooms place the rope bridge across it. Chests embedded in rockfall and
slime rooms also use the scanned chest while retaining their hazard symbols.

Portcullises are traps: a generated `Portcullis (Spot 6, Disarm 11)` entry
from `mixed.tab`, `trap.tab`, or another table adds a portcullis marker to that
piece in the enhanced GM map. The table does not specify an exact square, so
this is a contents marker inside the piece, not a newly placed door. It stays
hidden in Player View even after the room is revealed, because entering a
room does not automatically discover its traps. The earlier portcullis door
style option has been removed. Ordinary and secret doors retain their normal
appearance and click behaviour.

Numbers keep their original positions, and doors remain on top. Changing the display keeps the current scroll
position, zoom and revealed rooms; hidden rooms stay hidden in Player View.
Saved images and printouts still use the original style.

The executable embeds the shared artwork in `rsh/board/atlas.bmp`. Ordinary builds
(`build-msvc.bat` or `package.bat`) need no image-processing tools. To regenerate
the images, install Pillow and ImageMagick (`magick` on PATH), then run
`python tools/prepare-board-tiles.py`.
The script samples complete floor squares from
`Tiles/Advanced Heroquest - Floors - 3.jpg`, avoiding scan backgrounds and
cardboard tabs, and preserves the original bitmap exit boundaries. Artwork is
scaled directly to the display size rather than reduced to the old 8-pixel
grid first. Room floors are cropped and straightened from the two rooms on
each of `Advanced Heroquest - Rooms - 2.jpg` and `- 3.jpg`, and the large rooms
on `- 4.jpg` and `- 6.jpg`. The original room wall footprint is retained.

Special features use `Advanced Heroquest - Tiles - 1.jpg`, the decorated
`Advanced Heroquest - Rooms - 5.jpg`, and the supplied stair images. Monster
counters and extra scenic images are not assigned to unrelated map features.

The generator shares identical 16×16 pixel patches, including rotated copies,
across floors, features and walls. The executable contains one full-colour
atlas and small assembly recipes in `rsh/board-atlas.h`, rather than 185
separate colour bitmaps. Large rooms reuse the same floor patches; features
reuse the floor behind them. Each distinct patch is stored only once.

Images are reconstructed with exact integer rotations and cached in memory.
Special-feature images are assembled only when needed. Normal rendering,
numbers, doors and fog-of-war behaviour are unchanged. `rsh/board-atlas.json`
records sizes and lossless image hashes for validation; it is not embedded or
required at runtime. Complete reference images are generated under
`obj/board-reference` for development only, and are not shipped.

All board artwork remains at 32 pixels per board square, prepared at 64 pixels
before ImageMagick downsampling. The atlas retains full RGB colour; there is
no further palette reduction. The original scans are preserved. Ordinary
application builds use the prepared atlas and do not require ImageMagick.
