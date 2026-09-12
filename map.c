#include <map.h>
#include <ro_mem.h>

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
