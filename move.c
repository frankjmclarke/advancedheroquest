#include <move.h>
#include <defs.h>
#include <boxf.h>

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL DIRECTION straight(PICE *from)
{
	switch (from->pos)
	{
	case North:
		return North;
	case South:
		return South;
	case East:
		return East;
	case West:
		return West;
	case Illegal_Dir:
		return FALSE;
	default:
		if (!error_abort("Invalid direction\n(straight)"))
			return Illegal_Dir;
		break;
	}
	return North;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL DIRECTION left(PICE *from)
{
	switch (from->pos)
	{
	case North:
		return West;
	case South:
		return East;
	case East:
		return North;
	case West:
		return South;
	case Illegal_Dir:
		return FALSE;
	default:
		if (!error_abort("Invalid direction\n(left)"))
			return Illegal_Dir;
		break;
	}
	return North;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL DIRECTION right(PICE *from)
{
	switch (from->pos)
	{
	case North:
		return East;
	case South:
		return West;
	case East:
		return South;
	case West:
		return North;
	case Illegal_Dir:
		return FALSE;
	default:
 		if (!error_abort("Invalid direction\n(right)"))
 			return Illegal_Dir;
		break;
	}
	return North;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL DIRECTION back(PICE *from)
{
	switch (from->pos)
	{
	case North:
		return South;
	case South:
		return North;
	case East:
		return West;
	case West:
		return East;
	case Illegal_Dir:
		return FALSE;
	default:
		if (!error_abort("Invalid direction\n(back)"))
			return Illegal_Dir;
		break;
	}
	return North;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL pos_north(PICE *new, PICE *from)
{
	new->pos = North;
	new->x = from->x;
	new->y = from->y;
	new->y += from->h;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL pos_south(PICE *new, PICE *from)
{
	new->pos = South;
	new->x = from->x;
	new->y = from->y;
	new->y -= new->h;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL pos_east(PICE *new, PICE *from)
{
	new->pos = East;
	new->x = from->x;
	new->x += from->w;
	new->y = from->y;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL pos_west(PICE *new, PICE *from)
{
	new->pos = West;
	new->x = from->x;
	new->x -= new->w;
	new->y = from->y;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL pos_at(PICE *new, PICE *from)
{
	new->pos = from->pos;
	new->x = from->x;
	new->y = from->y;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL pos_straight(PICE *new, PICE *from)
{
	switch (from->pos)
	{
	case North:
		if (!pos_north(new, from))
			return FALSE;
		break;
	case South:
		if (!pos_south(new, from))
			return FALSE;
		break;
	case East:
		if (!pos_east(new, from))
			return FALSE;
		break;
	case West:
		if (!pos_west(new, from))
			return FALSE;
		break;
	case Illegal_Dir:
		return FALSE;
	default:
		if (!error_abort("Invalid direction\n(pos_straight)"))
			return FALSE;
		break;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL pos_back(PICE *new, PICE *from)
{
	switch (from->pos)
	{
	case North:
		if (!pos_south(new, from))
			return FALSE;
		break;
	case South:
		if (!pos_north(new, from))
			return FALSE;
		break;
	case East:
		if (!pos_west(new, from))
			return FALSE;
		break;
	case West:
		if (!pos_east(new, from))
			return FALSE;
		break;
	case Illegal_Dir:
		return FALSE;
	default:
		if (!error_abort("Invalid direction\n(pos_back)"))
			return FALSE;
		break;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL pos_left(PICE *new, PICE *from)
{
	switch (from->pos)
	{
	case North:
		if (!pos_west(new, from))
			return FALSE;
		break;
	case South:
		if (!pos_east(new, from))
			return FALSE;
		break;
	case East:
		if (!pos_north(new, from))
			return FALSE;
		break;
	case West:
		if (!pos_south(new, from))
			return FALSE;
		break;
	case Illegal_Dir:
		return FALSE;
	default:
		if (!error_abort("Invalid direction\n(pos_left)"))
			return FALSE;
		break;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL pos_right(PICE *new, PICE *from)
{
	switch (from->pos)
	{
	case North:
		if (!pos_east(new, from))
			return FALSE;
		break;
	case South:
		if (!pos_west(new, from))
			return FALSE;
		break;
	case East:
		if (!pos_south(new, from))
			return FALSE;
		break;
	case West:
		if (!pos_north(new, from))
			return FALSE;
		break;
	case Illegal_Dir:
		return FALSE;
	default:
		if (!error_abort("Invalid direction\n(pos_right)"))
			return FALSE;
		break;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL xy_north(PICE *new, PICE *from)
{
	new->x = from->x;
	new->y = from->y;
	new->y += from->h;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL xy_south(PICE *new, PICE *from)
{
	new->x = from->x;
	new->y = from->y;
	new->y -= new->h;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL xy_east(PICE *new, PICE *from)
{
	new->x = from->x;
	new->x += from->w;
	new->y = from->y;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL xy_west(PICE *new, PICE *from)
{
	new->pos = West;
	new->x = from->x;
	new->x -= new->w;
	new->y = from->y;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL xy_at(PICE *new, PICE *from)
{
	new->x = from->x;
	new->y = from->y;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL xy_straight(PICE *new, PICE *from)
{
	switch (from->pos)
	{
	case North:
		if (!xy_north(new, from))
			return FALSE;
		break;
	case South:
		if (!xy_south(new, from))
			return FALSE;
		break;
	case East:
		if (!xy_east(new, from))
			return FALSE;
		break;
	case West:
		if (!xy_west(new, from))
			return FALSE;
		break;
	case Illegal_Dir:
		return FALSE;
	default:
		if (!error_abort("Invalid direction\n(xy_straight)"))
			return FALSE;
		break;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL xy_back(PICE *new, PICE *from)
{
	switch (from->pos)
	{
	case North:
		if (!xy_south(new, from))
			return FALSE;
		break;
	case South:
		if (!xy_north(new, from))
			return FALSE;
		break;
	case East:
		if (!xy_west(new, from))
			return FALSE;
		break;
	case West:
		if (!xy_east(new, from))
			return FALSE;
		break;
	case Illegal_Dir:
		return FALSE;
	default:
		if (!error_abort("Invalid direction\n(xy_back)"))
			return FALSE;
		break;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL xy_left(PICE *new, PICE *from)
{
	switch (from->pos)
	{
	case North:
		if (!xy_west(new, from))
			return FALSE;
		break;
	case South:
		if (!xy_east(new, from))
			return FALSE;
		break;
	case East:
		if (!xy_north(new, from))
			return FALSE;
		break;
	case West:
		if (!xy_south(new, from))
			return FALSE;
		break;
	case Illegal_Dir:
		return FALSE;
	default:
		if (!error_abort("Invalid direction\n(xy_left)"))
			return FALSE;
		break;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL xy_right(PICE *new, PICE *from)
{
	switch (from->pos)
	{
	case North:
		if (!xy_east(new, from))
			return FALSE;
		break;
	case South:
		if (!xy_west(new, from))
			return FALSE;
		break;
	case East:
		if (!xy_south(new, from))
			return FALSE;
		break;
	case West:
		if (!xy_north(new, from))
			return FALSE;
		break;
	case Illegal_Dir:
		return FALSE;
	default:
		if (!error_abort("Invalid direction\n(xy_right)"))
			return FALSE;
		break;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _WORD length(PICE *from)
{
	switch (from->pos)
	{
	case North:
	case South:
		return from->h;
	case East:
	case West:
		return from->w;
	case Illegal_Dir:
		return 0;
	}
	if (!error_abort("Invalid direction\n(length)"))
		return 0;
	return from->h;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _WORD wide(PICE *from)
{
	switch (from->pos)
	{
	case North:
	case South:
		return from->w;
	case East:
	case West:
		return from->h;
	case Illegal_Dir:
		return 0;
	}
	if (!error_abort("Invalid direction\n(wide)"))
		return 0;
	return from->w;
}
