#include <map.h>
#include <ro_mem.h>
#include <defs.h>

GLOBAL _WORD Xsize, Ysize;
GLOBAL _WORD MAX_X = 0;
GLOBAL _WORD MAX_Y = 0;

LOCAL PICE **Map;

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL _BOOL get_square(_WORD x, _WORD y, PICE **ptr)
{
	if (x < 0 || y < 0 || x >= Xsize || y >= Ysize)
	{
		*ptr = NULL;
		return FALSE;
	}
	*ptr = *(Map + (size_t)x + (size_t)y * (size_t)MAX_X);
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL set_square(_WORD x, _WORD y, PICE *ptr)
{
	if (x < 0 || y < 0 || x >= Xsize || y >= Ysize)
	{
		return FALSE;
	}
	*(Map + (size_t)x + (size_t)y * (size_t)MAX_X) = ptr;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL init_map(_WORD x, _WORD y)
{
	_WORD i, j;
	
	if (x > MAX_X || y > MAX_Y)
	{
		return FALSE;
	}
	
	Xsize = x;
	Ysize = y;
	
	for (i = 0; i < Xsize; i++)
	{
		for (j = 0; j < Ysize; j++)
		{
			if (!clr_square(i, j))
			{
				return FALSE;
			}
		}
	}
	pice_empty();
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID map_exit(_VOID)
{
	if (Map != NULL)
	{
		SFREE(Map);
		Map = NULL;
	}
	MAX_X = 0;
	MAX_Y = 0;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL map_init(_WORD max_x, _WORD max_y)
{
	if ((Map = MALLOC(sizeof(*Map) * (size_t)max_x * (size_t)max_y, "map_init: map")) == NULL)
		return FALSE;
	MAX_X = max_x;
	MAX_Y = max_y;
	return TRUE;
}

/* Fog geometry uses piece bounds, not Map cells: doors replace the cell's
 * room pointer, but both sides of their wall must still be found. */
LOCAL _BOOL fog_room(TYPE type)
{
 switch (type) {
 case SMALL_ROOM: case NORMAL_ROOM: case HAZARD: case LARGE_ROOM:
 case LAIR: case QUEST: case BIG: case MERSCHA: return TRUE;
 default: return FALSE;
 }
}

LOCAL _BOOL fog_corridor(TYPE type)
{
 switch (type) {
 case PASSAGE: case DEAD_END: case LEFT_TURN: case RIGHT_TURN:
 case T_JUNCTION: case CORNER: case STAIRS_OUT: case STAIRS_DOWN: return TRUE;
 default: return FALSE;
 }
}

LOCAL _BOOL fog_door_side(CONST PICE *door, CONST PICE *piece)
{
 _WORD x = door->x, y = door->y;
 if (x >= piece->x && x < piece->x + piece->w &&
     y >= piece->y && y < piece->y + piece->h) return TRUE;
 switch (door->pos) {
 case North: y++; break;
 case South: y--; break;
 case East: x++; break;
 case West: x--; break;
 default: return FALSE;
 }
 return x >= piece->x && x < piece->x + piece->w &&
        y >= piece->y && y < piece->y + piece->h;
}

GLOBAL _VOID fog_init(_UBYTE *visible)
{
 _WORD i;
 for (i = 0; i < MAX_PICE; i++) visible[i] = fog_corridor(Pice[i].type);
}

GLOBAL _BOOL fog_visible(CONST _UBYTE *visible, _WORD index)
{
 _WORD i;
 CONST PICE *door = &Pice[index];
 if (door->type != DOOR && door->type != SECRET) return visible[index] != 0;
 for (i = 0; i < MAX_PICE; i++)
  if (visible[i] && (fog_room(Pice[i].type) || fog_corridor(Pice[i].type)) &&
      fog_door_side(door, &Pice[i])) return TRUE;
 return FALSE;
}

/* Pixel coordinates are unzoomed bitmap coordinates. Match door_pos()'s
 * half-cell offset exactly, so either half of a visible doorway is clickable. */
GLOBAL _BOOL fog_open_door(_UBYTE *visible, _WORD pixel_x, _WORD pixel_y)
{
 _WORD i, j, x, y;
 _BOOL changed = FALSE;
 PICE *door;
 for (i = MAX_PICE - 1; i >= 0; i--) {
  door = &Pice[i];
  if ((door->type != DOOR && door->type != SECRET) || !fog_visible(visible, i)) continue;
  x = (door->x + 1) * ICON_SCALE;
  y = (Ysize - door->y) * ICON_SCALE;
  switch (door->pos) {
  case North: y -= ICON_SCALE / 2; break;
  case South: y += ICON_SCALE / 2; break;
  case East: x += ICON_SCALE / 2; break;
  case West: x -= ICON_SCALE / 2; break;
  default: continue;
  }
  if (pixel_x < x || pixel_x >= x + ICON_SCALE ||
      pixel_y < y || pixel_y >= y + ICON_SCALE) continue;
  for (j = 0; j < MAX_PICE; j++)
   if (!visible[j] && fog_room(Pice[j].type) && fog_door_side(door, &Pice[j])) {
    visible[j] = TRUE;
    changed = TRUE;
   }
  return changed;
 }
 return FALSE;
}
