#include <icon.h>
#include <pice.h>
#include <map.h>
#include <grafic.rh>
#include <board-features.rh>
#include <board-atlas.h>
#include <defs.h>
#include <boxf.h>
#include <ctype.h>
#include <debug.h>

#define RAND 3

typedef struct icon
{
	struct icon **ptr;
	_WORD rsc;
	DIRECTION (*position)(DIRECTION dir, _WORD *x, _WORD *y, _WORD w, _WORD h);
	_WORD (*number)(DIRECTION dir, _WORD *x, _WORD *y, _WORD w, _WORD h, _WORD wx, _WORD hx, _WORD wy, _WORD hy);
#define DIR_NONE 0
#define DIR_X    1
#define DIR_Y    2
	MFDB *mfdb[4];
} ICON;

LOCAL MFDB *MFDB_digit[10];

LOCAL ICON *border;
LOCAL ICON *normal;
LOCAL ICON *hazard;
LOCAL ICON *large;
LOCAL ICON *lair;
LOCAL ICON *quest;
LOCAL ICON *bigtomb;
LOCAL ICON *big;
LOCAL ICON *merscha;

LOCAL ICON *passage;

LOCAL ICON *deadend;
LOCAL ICON *left_turn;
LOCAL ICON *right_turn;
LOCAL ICON *t_junction;
LOCAL ICON *corner;
LOCAL ICON *stairs_out;
LOCAL ICON *stairs_down;

LOCAL ICON *door;
LOCAL ICON *secret;
LOCAL ICON *test;

LOCAL ICON *bridge;
LOCAL ICON *chasm;
LOCAL ICON *grate;
LOCAL ICON *trap;
LOCAL ICON *throne;
LOCAL ICON *statue;
LOCAL ICON *pool;
LOCAL ICON *cess_pit;
LOCAL ICON *well;
LOCAL ICON *circle;
LOCAL ICON *rats;
LOCAL ICON *bats;
LOCAL ICON *mould;
LOCAL ICON *mushrooms;
LOCAL ICON *apparition;
LOCAL ICON *rockfall;
LOCAL ICON *slime;
LOCAL ICON *moths;
LOCAL ICON *chest;
LOCAL ICON *stairs_and_chest;
LOCAL ICON *tomb;
LOCAL ICON *weapon;
LOCAL ICON *cupboard;
LOCAL ICON *table;
LOCAL ICON *fireplace;
LOCAL ICON *bookcase;
LOCAL ICON *rack;


/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL ICON *pice_icon(PICE *pice)
{
	switch (pice->type)
	{
	case SMALL_ROOM:
	case NORMAL_ROOM:
	case HAZARD:
		switch (pice->feature)
		{
		case Nothing:
		case Wandering_Monsters:
		case Maiden:
		case Witch:
		case Man_at_Arms:
		case Rogue:
		case Wight:
			return hazard;
		case Statue:
			return statue;
		case Rats:
			return rats;
		case Bats:
			return bats;
		case Mould:
			return mould;
		case Mushrooms:
			return mushrooms;
		case Chasm:
			return chasm;
		case Grate:
			return grate;
		case Pool:
			return pool;
		case Magic_Circle:
			return circle;
		case Trapdoor:
			return trap;
		case Throne:
			return throne;
		case Rockfall:
			return rockfall;
		case Slime:
			return slime;
		case Cess_Pit:
			return cess_pit;
		case Bridge:
			return bridge;
		case Apparition:
			return apparition;
		case Well:
			return well;
		case Moths:
			return moths;
		case Chest:
			return chest;
		case Stairs_and_Chest:
			return stairs_and_chest;
		case Tomb:
			return tomb;
		case Weapons_Rack:
			return weapon;
		case Cupboard:
			return cupboard;
		case Table:
			return table;
		case Fireplace:
			return fireplace;
		case Bookcase:
			return bookcase;
		case Rack:
			return rack;
		default:
			break;
		}
		if (!error_abort("Unknown small feature\n(pice_icon)"))
			return NULL;
		return hazard;
	case LARGE_ROOM:
	case LAIR:
	case QUEST:
		switch (pice->feature)
		{
		case Nothing:
			return large;
		case Chest:
			return lair;
		case Stairs_and_Chest:
			return quest;
		case Tomb:
			return bigtomb;
		default:
			break;
		}
		if (!error_abort("Unknown large feature\n(pice_icon)"))
			return NULL;
		return large;
	case BIG:
		return big;
	case MERSCHA:
		return merscha;
	case PASSAGE:
		return passage;
	case DEAD_END:
		return deadend;
	case LEFT_TURN:
		return left_turn;
	case RIGHT_TURN:
		return right_turn;
	case T_JUNCTION:
		return t_junction;
	case CORNER:
		return corner;
	case STAIRS_OUT:
		return stairs_out;
	case STAIRS_DOWN:
		return stairs_down;
	case DOOR:
		return door;
	case SECRET:
		return secret;
	case TEST:
		return test;
	default:
		break;
	}
	if (!error_abort("Invalid section\n(pice_image)"))
		return NULL;
	return NULL;
}

/*** ---------------------------------------------------------------------- ***/

/*
 * Ink and paper for a map piece.
 *
 * The tiles are 1 bit deep, so nothing here changes the artwork: GDI expands
 * the bitmap while blitting, giving the clear bits the ink colour and the set
 * bits the paper colour. Feature wins over type, because the feature is what
 * a reader is usually looking for; type only decides the colour of plain
 * structure such as corridors and stairs.
 *
 * Paper stays very light so the hatched floor reads as a tint rather than a
 * block of colour, and so the room numbers drawn on top stay legible.
 */
LOCAL _VOID pice_colors(PICE *pice, _LONG *ink, _LONG *paper)
{
	*ink = W_RGB( 60,  64,  74);		/* dark slate, the default */
	*paper = W_RGB(255, 255, 255);

	switch (pice->feature)
	{
	case Pool:
	case Well:
	case Bridge:
	case Chasm:
		*ink = W_RGB( 30,  90, 170);	/* water and voids: blue */
		*paper = W_RGB(232, 241, 250);
		return;
	case Fireplace:
		*ink = W_RGB(190,  60,  20);	/* fire: orange red */
		*paper = W_RGB(253, 237, 228);
		return;
	case Slime:
	case Mould:
	case Mushrooms:
	case Cess_Pit:
		*ink = W_RGB( 40, 120,  50);	/* growth and filth: green */
		*paper = W_RGB(233, 244, 233);
		return;
	case Chest:
	case Stairs_and_Chest:
		*ink = W_RGB(168, 128,  20);	/* treasure: gold */
		*paper = W_RGB(252, 246, 226);
		return;
	case Tomb:
	case Apparition:
	case Wight:
		*ink = W_RGB(110,  60, 160);	/* undead: violet */
		*paper = W_RGB(243, 237, 250);
		return;
	case Magic_Circle:
		*ink = W_RGB(160,  40, 140);	/* magic: magenta */
		*paper = W_RGB(250, 235, 247);
		return;
	case Rats:
	case Bats:
	case Moths:
		*ink = W_RGB(120,  80,  40);	/* vermin: brown */
		*paper = W_RGB(248, 243, 235);
		return;
	case Wandering_Monsters:
	case Maiden:
	case Witch:
	case Man_at_Arms:
	case Rogue:
		*ink = W_RGB(170,  30,  50);	/* occupants: crimson */
		*paper = W_RGB(252, 234, 237);
		return;
	case Throne:
	case Statue:
	case Weapons_Rack:
	case Rack:
	case Table:
	case Cupboard:
	case Bookcase:
		*ink = W_RGB(130,  90,  50);	/* furnishings: wood */
		*paper = W_RGB(250, 246, 239);
		return;
	case Trapdoor:
	case Rockfall:
	case Grate:
		*ink = W_RGB( 90,  92,  98);	/* hazards: grey */
		*paper = W_RGB(242, 242, 244);
		return;
	default:
		break;
	}

	switch (pice->type)
	{
	case PASSAGE:
	case DEAD_END:
	case LEFT_TURN:
	case RIGHT_TURN:
	case T_JUNCTION:
	case CORNER:
		*ink = W_RGB( 70,  80,  95);	/* corridors: cool grey */
		*paper = W_RGB(246, 247, 249);
		break;
	case STAIRS_DOWN:
	case STAIRS_OUT:
		*ink = W_RGB(140,  90,  30);	/* stairs: brown */
		*paper = W_RGB(251, 245, 234);
		break;
	case DOOR:
		*ink = W_RGB(110,  70,  35);
		break;
	case SECRET:
		*ink = W_RGB(150,  40,  40);	/* secret doors: red */
		break;
	case LAIR:
		*ink = W_RGB( 45, 110,  55);
		*paper = W_RGB(235, 245, 236);
		break;
	case QUEST:
		*ink = W_RGB(120,  50, 150);
		*paper = W_RGB(245, 238, 251);
		break;
	case HAZARD:
		*ink = W_RGB(175, 110,  20);
		*paper = W_RGB(253, 247, 232);
		break;
	default:
		break;
	}
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID copy_icon(MFDB *dest, MFDB *icon, _WORD x, _WORD y, _WORD w, _WORD h)
{
	draw_mfdb(icon, 0, 0, w, h, dest, x, y, MD_REPLACE);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID draw_border(MFDB *image)
{
	_WORD i;

	for (i = 0; i < Xsize + 2; i++)
	{
		copy_icon(image, border->mfdb[0], i * ICON_SCALE, 0, Border.w * ICON_SCALE, Border.h * ICON_SCALE);
		copy_icon(image, border->mfdb[0], i * ICON_SCALE, (Ysize + 1) * ICON_SCALE, Border.w * ICON_SCALE, Border.h * ICON_SCALE);
	}
	for (i = 0; i < Ysize+2; i++)
	{
		copy_icon(image, border->mfdb[0], 0, i*ICON_SCALE, Border.w*ICON_SCALE, Border.h*ICON_SCALE);
		copy_icon(image, border->mfdb[0], (Xsize + 1) * ICON_SCALE, i * ICON_SCALE, Border.w * ICON_SCALE, Border.h * ICON_SCALE);
	}
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL draw_pice(MFDB *image, PICE *pice)
{
	_WORD i, xx, yy;
	_UBYTE *d;
	MFDB *mfdb;
	ICON *icon;
	DIRECTION dir;
	_WORD x, y, w, h;
	_WORD dw, dh;
	_WORD xh, yw;
	
	if ((icon = pice_icon(pice)) != NULL)
	{
		dir = pice->pos;
		x = (pice->x + 1) * ICON_SCALE;
		y = (pice->y + 1) * ICON_SCALE;
		w = pice->w * ICON_SCALE;
		h = pice->h * ICON_SCALE;
		
		if (icon->position != FUNK_NULL)
		{
			dir = icon->position(dir, &x, &y, w, h);
			if (dir == Illegal_Dir)
				return FALSE;
		}
		
		{
			_LONG ink, paper;

			pice_colors(pice, &ink, &paper);
			set_mfdb_colors(ink, paper);
			copy_icon(image, icon->mfdb[dir], x, (Ysize + 2) * ICON_SCALE - y - h, w, h);
			reset_mfdb_colors();		/* room numbers stay black */
		}
		
		if (pice->text != NULL && icon->number != FUNK_NULL)
		{
			xx = 0;
			yy = 0;
			d = (*(pice->text));
			SPEC_DEBUG_OUT((EO_DEBUG, "draw_pice: x = %d, y = %d, w = %d, h = %d, d = %s", x, y, w, h, d));
			xh = 0;
			yw = 0;
			for (i = 1; i <= N_DIGIT && d[i] != '\0' && isdigit(d[i]); i++)
			{
				get_mfdb_info(MFDB_digit[d[i]-'0'], &dw, &dh, NULL);
				if (dw > yw)
					yw = dw;
				if (dh > xh)
					xh = dh;
				xx += dw;
				yy += dh;
			}
			if (xx > 0 &&
				xh > 0 &&
				yw > 0 &&
				yy > 0)
			{
				switch (icon->number(dir, &x, &y, w, h, xx, xh, yw, yy))
				{
				case DIR_X:
					y = (Ysize + 2) * ICON_SCALE - y - xh;
					for (i = 1; i <= N_DIGIT && d[i] != '\0' && isdigit(d[i]); i++)
					{
						mfdb = MFDB_digit[d[i] - '0'];
						get_mfdb_info(mfdb, &dw, &dh, NULL);
						draw_mfdb(mfdb, 0, 0, dw, dh, image, x, y, MD_REPLACE);
						x += dw;
					}
					break;
				case DIR_Y:
					y = (Ysize + 2) * ICON_SCALE - y - yy;
					for (i = 1; i <= N_DIGIT && d[i] != '\0' && isdigit(d[i]); i++)
					{
						mfdb = MFDB_digit[d[i] - '0'];
						get_mfdb_info(mfdb, &dw, &dh, NULL);
						draw_mfdb(mfdb, 0, 0, dw, dh, image, x, y, MD_REPLACE);
						y += dh;
					}
					break;
				case DIR_NONE:
					return FALSE;
				default:
					if (!error_abort("Invalid direction\n(draw_pice)"))
						return FALSE;
					break;
				}
			}
		}
		return TRUE;
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL draw_map_visible(MFDB *image, CONST _UBYTE *visible)
{
	_WORD i;
	PICE *pice;
	
	if (mfdb_start_paint(image))
	{
		draw_border(image);
	
		pice = Pice;
		for (i = 0; i < MAX_PICE; i++)
		{
			if (visible != NULL && !fog_visible(visible, i)) { pice++; continue; }
			switch (pice->type)
			{
			case SMALL_ROOM:
			case NORMAL_ROOM:
			case HAZARD:
			case LARGE_ROOM:
			case LAIR:
			case QUEST:
			case BIG:
			case MERSCHA:
			case PASSAGE:
			case DEAD_END:
			case LEFT_TURN:
			case RIGHT_TURN:
			case T_JUNCTION:
			case CORNER:
			case STAIRS_OUT:
			case STAIRS_DOWN:
				if (!draw_pice(image, pice))
					return FALSE;
				break;
			case TEST:
			case DOOR:
			case SECRET:
			case EMPTY:
				break;
			default:
				if (!error_abort("Invalid type\n(draw_img_map)"))
					return FALSE;
				break;
			}
			pice++;
		}
		
		pice = Pice;
		for (i = 0; i < MAX_PICE; i++)
		{
			if (visible != NULL && !fog_visible(visible, i)) { pice++; continue; }
			switch (pice->type)
			{
			case DOOR:
			case SECRET:
			case TEST:
				if (!draw_pice(image, pice))
					return FALSE;
				break;
			default:
				break;
			}
			pice++;
		}
		mfdb_end_paint(image);
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL draw_img_map(MFDB *image)
{
 return draw_map_visible(image, NULL);
}

GLOBAL _BOOL draw_player_map(MFDB *image, CONST _UBYTE *visible)
{
 return draw_map_visible(image, visible);
}

LOCAL _VOID center_mfdb(_WORD *x, _WORD *y, _WORD w, _WORD h, _WORD mw, _WORD mh)
{
	*x += (w - mw) / 2;
	*y += (h - mh) / 2;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _WORD west_mfdb(_WORD *x, _WORD *y, _WORD w, _WORD h, _WORD mw, _WORD mh)
{
	UNUSED(w);
	UNUSED(mw);
	*x += RAND;
	*y += (h - mh) / 2;
	return DIR_Y;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _WORD east_mfdb(_WORD *x, _WORD *y, _WORD w, _WORD h, _WORD mw, _WORD mh)
{
	*x += w - mw - RAND;
	*y += (h - mh) / 2;
	return DIR_Y;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _WORD north_mfdb(_WORD *x, _WORD *y, _WORD w, _WORD h, _WORD mw, _WORD mh)
{
	*x += (w - mw) / 2;
	*y += h - mh - RAND;
	return DIR_X;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _WORD south_mfdb(_WORD *x, _WORD *y, _WORD w, _WORD h, _WORD mw, _WORD mh)
{
	UNUSED(h);
	UNUSED(mh);
	*x += (w - mw) / 2;
	*y += RAND;
	return DIR_X;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _WORD center_nr(DIRECTION dir, _WORD *x, _WORD *y, _WORD w, _WORD h, _WORD wx, _WORD hx, _WORD wy, _WORD hy)
{
	UNUSED(dir);
	if (wx + 2 * RAND <= w)
	{
		center_mfdb(x, y, w, h, wx, hx);
		return DIR_X;
	}
	center_mfdb(x, y, w, h, wy, hy);
	return DIR_Y;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _WORD south_nr(DIRECTION dir, _WORD *x, _WORD *y, _WORD w, _WORD h, _WORD wx, _WORD hx, _WORD wy, _WORD hy)
{
	UNUSED(dir);
	UNUSED(wy);
	UNUSED(hy);
	return south_mfdb(x, y, w, h, wx, hx);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _WORD top_nr(DIRECTION dir, _WORD *x, _WORD *y, _WORD w, _WORD h, _WORD wx, _WORD hx, _WORD wy, _WORD hy)
{
	switch (dir)
	{
	case North:
		return north_mfdb(x, y, w, h, wx, hx);
	case South:
		return south_mfdb(x, y, w, h, wx, hx);
	case East:
		return east_mfdb(x, y, w, h, wy, hy);
	case West:
		return west_mfdb(x, y, w, h, wy, hy);
	default:
		break;
	}
	if (!error_abort("Invalid direction\n(top_nr)"))
		return DIR_NONE;
	center_mfdb(x, y, w, h, wx, hx);
	return DIR_X;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _WORD bottom_nr(DIRECTION dir, _WORD *x, _WORD *y, _WORD w, _WORD h, _WORD wx, _WORD hx, _WORD wy, _WORD hy)
{
	switch (dir)
	{
	case North:
		return south_mfdb(x, y, w, h, wx, hx);
	case South:
		return north_mfdb(x, y, w, h, wx, hx);
	case East:
		return west_mfdb(x, y, w, h, wy, hy);
	case West:
		return east_mfdb(x, y, w, h, wy, hy);
	default:
		break;
	}
	if (!error_abort("Invalid direction\n(bottom_nr)"))
		return DIR_NONE;
	center_mfdb(x, y, w, h, wx, hx);
	return DIR_X;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL DIRECTION room_pos(DIRECTION dir, _WORD *x, _WORD *y, _WORD w, _WORD h)
{
	UNUSED(x);
	UNUSED(y);
	switch (dir)
	{
	case North:
		if (w > h)
			dir = East;
		break;
	case South:
		if (w > h)
			dir = West;
		break;
	case East:
		if (w < h)
			dir = South;
		break;
	case West:
		if (w < h)
			dir = North;
		break;
	case Illegal_Dir:
		break;
	default:
		if (!error_abort("Invalid direction\n(room_pos)"))
			return Illegal_Dir;
		break;
	}
	return dir;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL DIRECTION door_pos(DIRECTION dir, _WORD *x, _WORD *y, _WORD w, _WORD h)
{
	UNUSED(w);
	UNUSED(h);
	switch (dir)
	{
	case North:
		*y += ICON_SCALE / 2;
		break;
	case South:
		*y -= ICON_SCALE / 2;
		break;
	case East:
		*x += ICON_SCALE / 2;
		break;
	case West:
		*x -= ICON_SCALE / 2;
		break;
	case Illegal_Dir:
		break;
	default:
		if (!error_abort("Invalid direction\n(door_pos)"))
			return Illegal_Dir;
		break;
	}
	return dir;
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

RLOCAL ICON Icons[] =
{
	{ &border, BBLACK, NULL, NULL },
	
	{ &normal, NORMALN, NULL, center_nr },
	{ &hazard, HAZARDN, NULL, center_nr },
	{ &large, LARGEN, room_pos, center_nr },
	{ &lair, LAIRN, room_pos, center_nr },
	{ &quest, QUESTN, room_pos, center_nr },
	{ &bigtomb, BIGTOMBN, room_pos, center_nr },
	{ &big, BIGN, room_pos, center_nr },
	{ &merscha, MERSCHAN, NULL, center_nr },
	
	{ &passage, PASSAGEN, NULL, center_nr },
	
	{ &deadend, DEADENDN, NULL, center_nr },
	{ &left_turn, LTURNN, NULL, center_nr },
	{ &right_turn, RTURNN, NULL, center_nr },
	{ &t_junction, TJUNCTN, NULL, center_nr },
	{ &corner, CORNERN, NULL, center_nr },
	{ &stairs_out, STOUTN, NULL, center_nr },
	{ &stairs_down, STDOWNN, NULL, center_nr },

 	{ &door, DOORN, door_pos, NULL },
	{ &secret, SECRETN, door_pos, NULL },
	{ &test, TESTN, door_pos, NULL },
	
	{ &bridge, BRIDGEN, NULL, top_nr },
	{ &chasm, CHASMN, NULL, top_nr },
	{ &throne, THRONEN, NULL, bottom_nr },
	{ &statue, STATUEN, NULL, bottom_nr },
	{ &cess_pit, CESSPITN, NULL, south_nr },
	{ &well, WELLN, NULL, south_nr },
	{ &pool, POOLN, NULL, south_nr },
	{ &circle, CIRCLEN, NULL, south_nr },
	{ &grate, GRATEN, NULL, south_nr },
	{ &trap, TRAPN, NULL, south_nr },
	{ &rats, RATN, NULL, south_nr },
	{ &bats, BATN, NULL, south_nr },
	{ &mould, MOULDN, NULL, south_nr },
	{ &mushrooms, MUSHN, NULL, south_nr },
	{ &apparition, APPARIN, NULL, south_nr },
	{ &rockfall, ROCKN, NULL, bottom_nr },
	{ &slime, SLIMEN, NULL, bottom_nr },
	{ &moths, MOTHN, NULL, south_nr },
	{ &chest, CHESTN, NULL, center_nr },
	{ &stairs_and_chest, STCHESTN, NULL, center_nr },
	{ &weapon, WEAPONSN, NULL, bottom_nr },
	{ &cupboard, BOARDN, NULL, bottom_nr },
	{ &table, TABLEN, NULL, bottom_nr },
	{ &fireplace, FIREN, NULL, bottom_nr },
	{ &bookcase, CASEN, NULL, bottom_nr },
	{ &rack, RACKN, NULL, bottom_nr },
	{ &tomb, TOMBN, NULL, bottom_nr },
	{ NULL }
};

typedef struct board_image {
    _WORD id, w, h;
    _ULONG offset;
} BOARD_IMAGE;
LOCAL CONST BOARD_IMAGE board_images[] = BOARD_IMAGE_ENTRIES;
LOCAL MFDB *board_atlas;

LOCAL MFDB *get_board_bitmap(_WORD id)
{
    unsigned int i;
    for (i = 0; i < sizeof(board_images) / sizeof(board_images[0]); i++)
    {
        if (board_images[i].id != id) continue;
        if (board_atlas == NULL) board_atlas = get_mfdb_from_bitmap(BOARD_ATLAS_RESOURCE);
        if (board_atlas == NULL) return NULL;
        return assemble_mfdb(board_atlas, board_images[i].w, board_images[i].h,
            BOARD_ATLAS_BLOCK, BOARD_ATLAS_COLUMNS, board_patch_codes + board_images[i].offset);
    }
    return NULL;
}

LOCAL _BOOL init_mfdb(MFDB **mfdb, _WORD formular, _WORD images)
{
	_WORD i;
	
	for (i = 0; i < images; i++)
	{
		mfdb[i] = formular >= BOARD_FIRST ? get_board_bitmap(formular + i) : get_mfdb_from_bitmap(formular + i);
		if (mfdb[i] == NULL)
		{
			ende("RSC error (init_mfdb)");
			return FALSE;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL MFDB *board_tiles[24];
LOCAL MFDB *board_portcullis_trap;
LOCAL MFDB *board_rooms[BOARD_ROOMS_COUNT];
typedef struct board_feature {
    _WORD original, resource;
    MFDB *image;
} BOARD_FEATURE;
LOCAL BOARD_FEATURE board_features[] = BOARD_FEATURE_RESOURCES;
#define BOARD_FEATURE_COUNT (sizeof(board_features) / sizeof(board_features[0]))

GLOBAL _BOOL init_icons(_VOID)
{
	ICON *icon;
	
	if (!init_mfdb(board_tiles, BOARD_FIRST, 24))
		return FALSE;
	if (!init_mfdb(board_rooms, BOARD_ROOMS_FIRST, BOARD_ROOMS_COUNT))
		return FALSE;
	if (!init_mfdb(MFDB_digit, Z0, 10))
		return FALSE;
	
	for (icon = Icons; icon->ptr != NULL; icon++)
	{
		*(icon->ptr) = icon;
		if (!init_mfdb(icon->mfdb, icon->rsc, 4))
			return FALSE;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID free_mfdbs(MFDB **mfdb, _WORD images)
{
	_WORD i;
	
	for (i = 0; i < images; i++)
	{
		if (mfdb[i] != NULL)
		{
			free_mfdb(mfdb[i]);
			mfdb[i] = NULL;
		}
	}
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID free_icons(_VOID)
{
	ICON *icon;
	unsigned int i;
	
	for (i = 0; i < BOARD_FEATURE_COUNT; i++)
		free_mfdbs(&board_features[i].image, 1);
	free_mfdbs(board_tiles, 24);
	free_mfdbs(&board_atlas, 1);
	free_mfdbs(&board_portcullis_trap, 1);
	free_mfdbs(board_rooms, BOARD_ROOMS_COUNT);
	free_mfdbs(MFDB_digit, 10);
	
	for (icon = Icons; icon->ptr != NULL; icon++)
	{
		free_mfdbs(icon->mfdb, 4);
	}
}

/* Only replace plain rooms of matching dimensions. Special-feature artwork
 * remains intact until a corresponding board illustration is supported. */
LOCAL MFDB *board_room_image(PICE *p)
{
    _WORD variant, x = 0, y = 0;
    DIRECTION dir = p->pos;
    _ULONG seed = (_ULONG)p->x * 13 + (_ULONG)p->y * 7;
    if (dir < North || dir > West) return NULL;
    switch (p->type)
    {
    case SMALL_ROOM:
    case NORMAL_ROOM:
    case HAZARD:
        if (p->w != 5 || p->h != 5) return NULL;
        switch (p->feature)
        {
        case Nothing:
        case Wandering_Monsters:
        case Maiden:
        case Witch:
        case Man_at_Arms:
        case Rogue:
        case Wight:
            break;
        default: return NULL;
        }
        variant = (_WORD)(seed % 4);
        break;
    case LARGE_ROOM:
    case LAIR:
    case QUEST:
        if (p->feature != Nothing) return NULL;
        if (!((p->w == 5 && p->h == 10) || (p->w == 10 && p->h == 5))) return NULL;
        variant = 4 + (_WORD)(seed % 2);
        /* Use the same dimension-aware orientation as the old room bitmap. */
        dir = room_pos(dir, &x, &y, p->w, p->h);
        break;
    default: return NULL;
    }
    return board_rooms[variant * 4 + dir];
}

/* Feature tiles are loaded on demand: the original view and unrevealed
 * player rooms do not allocate a second set of large colour bitmaps. */
LOCAL MFDB *board_feature_image(PICE *p)
{
    ICON *icon = pice_icon(p);
    DIRECTION dir = p->pos;
    _WORD x = 0, y = 0, w, h, original;
    unsigned int i;
    if (icon == NULL || dir < North || dir > West) return NULL;
    if (icon->position != FUNK_NULL)
        dir = icon->position(dir, &x, &y, p->w, p->h);
    if (dir < North || dir > West) return NULL;
    original = icon->rsc + dir;
    for (i = 0; i < BOARD_FEATURE_COUNT; i++)
    {
        if (board_features[i].original != original) continue;
        if (board_features[i].image == NULL)
            board_features[i].image = get_board_bitmap(board_features[i].resource);
        if (board_features[i].image == NULL) return NULL;
        get_mfdb_info(board_features[i].image, &w, &h, NULL);
        if (w != p->w * BOARD_PIXELS_PER_SQUARE || h != p->h * BOARD_PIXELS_PER_SQUARE) return NULL;
        return board_features[i].image;
    }
    return NULL;
}

/* Trap tables emit text rather than a dedicated feature code. Recognize
 * the complete trap name at the start of a generated line, case-insensitively. */
LOCAL _BOOL has_portcullis_trap(PICE *p)
{
    _UBYTE **line;
    CONST _UBYTE *s, *q;
    CONST char *name;
    if (p->text == NULL) return FALSE;
    for (line = p->text; *line != NULL; line++)
    {
        s = *line;
        do
        {
            while (*s == ' ' || *s == '\t' || *s == '\r') s++;
            q = s; name = "portcullis";
            while (*name && *q && tolower((unsigned char)*q) == *name) { q++; name++; }
            if (*name == '\0' && (*q == '\0' || *q == '(' || isspace((unsigned char)*q)))
                return TRUE;
            while (*s && *s != '\n') s++;
            if (*s == '\n') s++;
        } while (*s);
    }
    return FALSE;
}

LOCAL _VOID draw_board_trap(_VOID *window, PICE *p, _WORD zoom, _WORD sx, _WORD sy)
{
    _WORD x, y, w = 2 * ICON_SCALE, h = ICON_SCALE;
    if (p->w < 2 || p->h < 2 || !has_portcullis_trap(p)) return;
    if (board_portcullis_trap == NULL)
        board_portcullis_trap = get_board_bitmap(BOARD_PORTCULLIS_TRAP);
    if (board_portcullis_trap == NULL) return;
    if (w > p->w * ICON_SCALE - 4) w = p->w * ICON_SCALE - 4;
    x = (p->x + 1) * ICON_SCALE + 2;
    y = (Ysize + 1 - p->y - p->h) * ICON_SCALE + 2;
    W_Draw_Tile(window, board_portcullis_trap, x * zoom - sx, y * zoom - sy, w * zoom, h * zoom);
}

/* Overlay supported board artwork. The base map remains the authority for
 * labels and doors, so fog, hit testing, saving and printing are unchanged. */
GLOBAL _VOID draw_board_map(_VOID *window, MFDB *base, CONST _UBYTE *visible,
                           _WORD zoom, _WORD scroll_x, _WORD scroll_y)
{
    _WORD i, kind, x, y, w, h, n, dw, dh, wx, hx, wy, hy, axis;
    _UBYTE *text;
    PICE *p;
    DIRECTION dir;
    ICON *icon;
    MFDB *tile;
    for (i = 0; i < MAX_PICE; i++)
    {
        if (visible != NULL && !fog_visible(visible, i)) continue;
        p = &Pice[i];
        switch (p->type)
        {
        case PASSAGE: kind = 0; break;
        case DEAD_END: kind = 1; break;
        case LEFT_TURN: kind = 2; break;
        case RIGHT_TURN: kind = 3; break;
        case T_JUNCTION: kind = 4; break;
        case CORNER: kind = 5; break;
        default: kind = -1; break;
        }
        if (p->pos < North || p->pos > West) continue;
        tile = kind >= 0 ? board_tiles[kind * 4 + p->pos] : board_room_image(p);
        if (tile == NULL && p->type != EMPTY && p->type != INVALID)
            tile = board_feature_image(p);
        if (tile == NULL) continue;
        x = (p->x + 1) * ICON_SCALE;
        y = (Ysize + 1 - p->y - p->h) * ICON_SCALE;
        w = p->w * ICON_SCALE; h = p->h * ICON_SCALE;
        W_Draw_Tile(window, tile,
                    x * zoom - scroll_x, y * zoom - scroll_y, w * zoom, h * zoom);
        if (visible == NULL) draw_board_trap(window, p, zoom, scroll_x, scroll_y);
        /* Preserve the original number as a legible small badge. */
        if (p->text == NULL || p->text[0] == NULL) continue;
        text = p->text[0]; wx = hx = wy = hy = 0;
        for (n = 1; n <= N_DIGIT && text[n] != '\0' && isdigit(text[n]); n++)
        {
            get_mfdb_info(MFDB_digit[text[n] - '0'], &dw, &dh, NULL);
            wx += dw; hy += dh;
            if (dh > hx) hx = dh;
            if (dw > wy) wy = dw;
        }
        if (wx == 0 || hy == 0) continue;
        y = (p->y + 1) * ICON_SCALE;
        dir = p->pos; icon = pice_icon(p);
        if (icon == NULL || icon->number == FUNK_NULL) continue;
        if (icon->position != FUNK_NULL) dir = icon->position(dir, &x, &y, w, h);
        if (dir == Illegal_Dir) continue;
        axis = icon->number(dir, &x, &y, w, h, wx, hx, wy, hy);
        if (axis == DIR_NONE) continue;
        w = axis == DIR_X ? wx : wy; h = axis == DIR_X ? hx : hy;
        y = (Ysize + 2) * ICON_SCALE - y - h;
        W_Draw_Bitmap(window, base, x, y, w, h,
                      x * zoom - scroll_x, y * zoom - scroll_y, zoom);
    }
    /* Doors straddle tile boundaries and must be the final layer. Copy the
     * existing coloured door pixels, respecting player visibility. */
    for (i = 0; i < MAX_PICE; i++)
    {
        if (visible != NULL && !fog_visible(visible, i)) continue;
        p = &Pice[i];
        if (p->type != DOOR && p->type != SECRET && p->type != TEST) continue;
        icon = pice_icon(p);
        if (icon == NULL) continue;
        x = (p->x + 1) * ICON_SCALE; y = (p->y + 1) * ICON_SCALE;
        w = p->w * ICON_SCALE; h = p->h * ICON_SCALE; dir = p->pos;
        if (icon->position != FUNK_NULL) dir = icon->position(dir, &x, &y, w, h);
        if (dir == Illegal_Dir) continue;
        y = (Ysize + 2) * ICON_SCALE - y - h;
        W_Draw_Bitmap(window, base, x, y, w, h,
                      x * zoom - scroll_x, y * zoom - scroll_y, zoom);
    }
}
