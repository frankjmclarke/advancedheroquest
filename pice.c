#include <pice.h>
#include <defs.h>
#include <move.h>
#include <map.h>
#include <ro_mem.h>

#define N_QUEUES 2


GLOBAL _WORD MAX_PICE;
GLOBAL PICE *Pice;

GLOBAL CONST SIZE Section = {2, 5};
GLOBAL CONST SIZE End = {2, 2};
GLOBAL CONST SIZE Ultra_Room = {15, 15};
GLOBAL CONST SIZE Big_Room = {10, 10};
GLOBAL CONST SIZE Large_Room = {5, 10};
GLOBAL CONST SIZE Small_Room = {5, 5};

GLOBAL CONST SIZE Border = {1, 1};
GLOBAL CONST SIZE Door = {1, 1};
GLOBAL CONST SIZE Void = {0, 0};

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL _BOOL init_pice(PICE *ptr, TYPE type, _WORD x, _WORD y, DIRECTION pos)
{
	CONST SIZE *size = &Void;
	
	ptr->type = type;
	ptr->x = x;
	ptr->y = y;
	ptr->pos = pos;
	
	ptr->feature = Nothing;
	ptr->text = NULL;
	
	switch (type)
	{
	case PASSAGE:
	case DEAD_END:
		size = &Section;
		break;
	case LEFT_TURN:
	case RIGHT_TURN:
	case T_JUNCTION:
	case CORNER:
	case STAIRS_OUT:
	case STAIRS_DOWN:
		size = &End;
		break;
	case TEST:
	case SECRET:
	case DOOR:
		size = &Door;
		break;
	case SMALL_ROOM:
	case NORMAL_ROOM:
	case HAZARD:
		size = &Small_Room;
		break;
	case LARGE_ROOM:
	case LAIR:
	case QUEST:
		size = &Large_Room;
		break;
	case BIG:
		size = &Big_Room;
		break;
	case MERSCHA:
		size = &Ultra_Room;
		break;
	default:
		if (!error_abort("Invalid section | (init_pice)"))
			return FALSE;
		break;
	}

	switch (pos)
	{
	case North:
	case South:
		ptr->w = size->w;
		ptr->h = size->h;
		break;
	case East:
	case West:
		ptr->w = size->h;
		ptr->h = size->w;
		break;
	case Illegal_Dir:
		return FALSE;
	default:
		if (!error_abort("Invalid direction | (init_pice)"))
			return FALSE;
		break;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL init_at(PICE *new, TYPE type, PICE *from)
{
	return init_pice(new, type, from->x, from->y, from->pos);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL init_straight(PICE *new, TYPE type, PICE *from)
{
	if (!init_pice(new, type, 0, 0, from->pos))
		return FALSE;
	if (!pos_straight(new, from))
		return FALSE;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL init_back(PICE *new, TYPE type, PICE *from)
{
	if (!init_pice(new, type, 0, 0, back(from)))
		return FALSE;
	if (!pos_back(new, from))
		return FALSE;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL init_left(PICE *new, TYPE type, PICE *from)
{
	if (!init_pice(new, type, 0, 0, left(from)))
		return FALSE;
	if (!pos_left(new, from))
		return FALSE;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL init_right(PICE *new, TYPE type, PICE *from)
{
	if (!init_pice(new, type, 0, 0, right(from)))
		return FALSE;
	if (!pos_right(new, from))
		return FALSE;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL set_pice(PICE *ptr)
{
	_WORD i, j;
	
	for (i = 0; i < ptr->w; i++)
	{
		for (j = 0; j < ptr->h; j++)
		{
			if (!set_square(ptr->x+i, ptr->y+j, ptr))
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL clr_pice(PICE *ptr)
{
	_WORD i, j;
	
	for (i = 0; i < ptr->w; i++)
	{
		for (j = 0; j < ptr->h; j++)
		{
			if (!clr_square(ptr->x+i, ptr->y+j))
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL delete_pice(PICE *ptr)
{
	ptr->type = EMPTY;
	return clr_pice(ptr);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL PICE *alloc_pice(_VOID)
{
	PICE *ptr = Pice;
	_WORD i;
	
	for (i = 0; i < MAX_PICE; i++)
	{
		if (ptr->type == EMPTY)
			return ptr;
		ptr++;
	}
	return NULL;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL insert_pice(PICE *dat)
{
	PICE *ptr;
	
	ptr = alloc_pice();
	if (ptr == NULL)
	{
		return FALSE;
	}
	copy_pice(ptr, dat);
	return set_pice(ptr);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL test_pice(PICE *ptr)
{
	PICE *foo;
	_WORD i, j;
	
	if (ptr->x < 0 || ptr->y < 0 || ptr->x >= Xsize || ptr->y >= Ysize)
	{
		return FALSE;
	}
	
	for (i = 0; i < ptr->w; i++)
	{
		for (j = 0; j < ptr->h; j++)
		{
			if (!get_square(ptr->x + i, ptr->y + j, &foo) || foo != NULL)
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID pice_empty(_VOID)
{
	_WORD i;
	
	for (i = 0; i < MAX_PICE; i++)
	{
		(Pice+i)->type = EMPTY;
	}
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID pice_exit(_VOID)
{
	if (Pice != NULL)
	{
		SFREE(Pice);
		Pice = NULL;
	}
	MAX_PICE = 0;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL pice_init(_WORD max_pice)
{
	if ((Pice = MALLOC(sizeof(*Pice) * (size_t)max_pice * (N_QUEUES+1), "mem_alloc: pice")) == NULL)
		return FALSE;
	MAX_PICE = max_pice;
	return TRUE;
}
