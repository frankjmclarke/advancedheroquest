#include <icon.h>
#include <pice.h>
#include <map.h>
#include <grafic.rh>
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
	if (!error_abort("Ungültige Sektion\n(pice_image)"))
		return NULL;
	return NULL;
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
		
		copy_icon(image, icon->mfdb[dir], x, (Ysize + 2) * ICON_SCALE - y - h, w, h);
		
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
					if (!error_abort("Ungültige Richtungsangabe\n(draw_pice)"))
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

GLOBAL _BOOL draw_img_map(MFDB *image)
{
	_WORD i;
	PICE *pice;
	
	if (mfdb_start_paint(image))
	{
		draw_border(image);
	
		pice = Pice;
		for (i = 0; i < MAX_PICE; i++)
		{
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
				if (!error_abort("Ungültiger Type\n(draw_img_map)"))
					return FALSE;
				break;
			}
			pice++;
		}
		
		pice = Pice;
		for (i = 0; i < MAX_PICE; i++)
		{
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
	if (!error_abort("Ungültige Richtungsangabe\n(top_nr)"))
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
	if (!error_abort("Ungültige Richtungsangabe\n(bottom_nr)"))
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
		if (!error_abort("Ungültige Richtungsangabe\n(room_pos)"))
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
		if (!error_abort("Ungültige Richtungsangabe\n(door_pos)"))
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

LOCAL _BOOL init_mfdb(MFDB **mfdb, _WORD formular, _WORD images)
{
	_WORD i;
	
	for (i = 0; i < images; i++)
	{
		mfdb[i] = get_mfdb_from_bitmap(formular + i);
		if (mfdb[i] == NULL)
		{
			ende("RSC error (init_mfdb)");
			return FALSE;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL init_icons(_VOID)
{
	ICON *icon;
	
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
	
	free_mfdbs(MFDB_digit, 10);
	
	for (icon = Icons; icon->ptr != NULL; icon++)
	{
		free_mfdbs(icon->mfdb, 4);
	}
}
