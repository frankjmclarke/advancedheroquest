#include <text.h>
#include <pice.h>
#include <map.h>
#include <defs.h>
#include <string.h>

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL _BOOL set_border(_UBYTE *Text_map)
{
	_WORD x, y;
	_UBYTE *ptr = Text_map;
	
	*ptr++ = '+';
	for (x = 0; x < Xsize; x++)
	{
		*ptr++ = '-';
	}
	*ptr++ = '+';
	*ptr++ = '\0';
	
	for (y = 0; y < Ysize; y++)
	{	
		*ptr++ = '|';
		for (x = 0; x < Xsize; x++)
		{
			*ptr++ = ' ';
		}
		*ptr++ = '|';
		*ptr++ = '\0';
	}
	
	*ptr++ = '+';
	for (x = 0; x < Xsize; x++)
	{
		*ptr++ = '-';
	}
	*ptr++ = '+';
	*ptr = '\0';
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL set_c(_UBYTE *Text_map, _WORD x, _WORD y, _UBYTE c)
{
	*(Text_map + (size_t)(x+1) + (size_t)(Ysize - y) * (size_t)(Xsize + 3)) = c;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL x_line(_UBYTE *Text_map, _WORD x, _WORD y, _WORD w, _UBYTE c)
{
	while (w-- > 0)
	{
		if (!set_c(Text_map, x++, y, c))
			return FALSE;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL y_line(_UBYTE *Text_map, _WORD x, _WORD y, _WORD h, _UBYTE c)
{
	while (h-- > 0)
	{
		if (!set_c(Text_map, x, y++, c))
			return FALSE;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL h_line(_UBYTE *Text_map, _WORD x, _WORD y, _WORD w)
{
	while (w-- > 0)
	{
		if (!set_c(Text_map, x++, y, '-'))
			return FALSE;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL v_line(_UBYTE *Text_map, _WORD x, _WORD y, _WORD h)
{
	while (h-- > 0)
	{
		if (!set_c(Text_map, x, y++, '|'))
			return FALSE;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL p_v_line(_UBYTE *Text_map, _WORD x, _WORD y, _WORD h)
{
	if (h-- > 0)
	{
		if (!set_c(Text_map, x, y, '+'))
			return FALSE;
	}
	if (h > 0)
	{
		if (!set_c(Text_map, x, y+h, '+'))
			return FALSE;
	}
	while (--h > 0)
	{
		if (!set_c(Text_map, x, ++y, '|'))
			return FALSE;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL p_h_line(_UBYTE *Text_map, _WORD x, _WORD y, _WORD w)
{
	if (w-- > 0)
	{
		if (!set_c(Text_map, x, y, '+'))
			return FALSE;
	}
	if (w > 0)
	{
		if (!set_c(Text_map, x+w, y, '+'))
			return FALSE;
	}
	while (--w > 0)
	{
		if (!set_c(Text_map, ++x, y, '-'))
			return FALSE;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL box(_UBYTE *Text_map, _WORD x, _WORD y, _WORD w, _WORD h, _UBYTE c)
{
	if (!x_line(Text_map, x, y, w, c))
		return FALSE;
	if (!x_line(Text_map, x, y + h - 1, w, c))
		return FALSE;
	if (!y_line(Text_map, x, y + 1, h - 2, c))
		return FALSE;
	if (!y_line(Text_map, x + w - 1, y + 1, h - 2, c))
		return FALSE;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL show_passage(_UBYTE *Text_map, PICE *ptr)
{
	switch (ptr->pos)
	{
	case North:
	case South:
		if (!v_line(Text_map, ptr->x, ptr->y, ptr->h))
			return FALSE;
		if (!v_line(Text_map, ptr->x + ptr->w - 1, ptr->y, ptr->h))
			return FALSE;
		break;
	case East:
	case West:
		if (!h_line(Text_map, ptr->x, ptr->y, ptr->w))
			return FALSE;
		if (!h_line(Text_map, ptr->x, ptr->y + ptr->h - 1, ptr->w))
			return FALSE;
		break;
	default:
		if (!error_abort("Ungültige Richtung\n(show_passage)"))
			return FALSE;
		break;
	}
	return TRUE;
} 

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL show_dead_end(_UBYTE *Text_map, PICE *ptr)
{
	switch (ptr->pos)
	{
	case North:
		if (!v_line(Text_map, ptr->x, ptr->y, ptr->h - 1))
			return FALSE;
		if (!v_line(Text_map, ptr->x + ptr->w-1, ptr->y, ptr->h - 1))
			return FALSE;
		if (!p_h_line(Text_map, ptr->x, ptr->y + ptr->h - 1, ptr->w))
			return FALSE;
		break;
	case South:
		if (!v_line(Text_map, ptr->x, ptr->y + 1, ptr->h - 1))
			return FALSE;
		if (!v_line(Text_map, ptr->x + ptr->w-1, ptr->y + 1, ptr->h - 1))
			return FALSE;
		if (!p_h_line(Text_map, ptr->x, ptr->y, ptr->w))
			return FALSE;
		break;
	case East:
		if (!h_line(Text_map, ptr->x, ptr->y, ptr->w-1))
			return FALSE;
		if (!h_line(Text_map, ptr->x, ptr->y + ptr->h - 1, ptr->w - 1))
			return FALSE;
		if (!p_v_line(Text_map, ptr->x + ptr->w - 1, ptr->y, ptr->h))
			return FALSE;
		break;
	case West:
		if (!h_line(Text_map, ptr->x + 1, ptr->y, ptr->w - 1))
			return FALSE;
		if (!h_line(Text_map, ptr->x + 1, ptr->y + ptr->h - 1, ptr->w - 1))
			return FALSE;
		if (!p_v_line(Text_map, ptr->x, ptr->y, ptr->h))
			return FALSE;
		break;
	default:
		if (!error_abort("Ungültige Richtung\n(show_dead_end)"))
			return FALSE;
		break;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL show_left_turn(_UBYTE *Text_map, PICE *ptr)
{
	switch (ptr->pos)
	{
	case North:
		if (!v_line(Text_map, ptr->x + ptr->w - 1, ptr->y, ptr->h - 1))
			return FALSE;
		if (!h_line(Text_map, ptr->x, ptr->y + ptr->h - 1, ptr->w - 1))
			return FALSE;
		if (!set_c(Text_map, ptr->x, ptr->y, '+'))
			return FALSE;
		if (!set_c(Text_map, ptr->x+ptr->w-1, ptr->y+ptr->h-1, '+'))
			return FALSE;
		break;
	case South:
		if (!v_line(Text_map, ptr->x, ptr->y + 1, ptr->h - 1))
			return FALSE;
		if (!h_line(Text_map, ptr->x + 1, ptr->y, ptr->w - 1))
			return FALSE;
		if (!set_c(Text_map, ptr->x, ptr->y, '+'))
			return FALSE;
		if (!set_c(Text_map, ptr->x + ptr->w - 1, ptr->y + ptr->h - 1, '+'))
			return FALSE;
		break;
	case East:
		if (!v_line(Text_map, ptr->x + ptr->w - 1, ptr->y + 1, ptr->h - 1))
			return FALSE;
		if (!h_line(Text_map, ptr->x, ptr->y, ptr->w-1))
			return FALSE;
		if (!set_c(Text_map, ptr->x, ptr->y + ptr->h - 1, '+'))
			return FALSE;
		if (!set_c(Text_map, ptr->x + ptr->w - 1, ptr->y, '+'))
			return FALSE;
		break;
	case West:
		if (!v_line(Text_map, ptr->x, ptr->y, ptr->h-1))
			return FALSE;
		if (!h_line(Text_map, ptr->x + 1, ptr->y + ptr->h - 1, ptr->w - 1))
			return FALSE;
		if (!set_c(Text_map, ptr->x, ptr->y+ptr->h-1, '+'))
			return FALSE;
		if (!set_c(Text_map, ptr->x + ptr->w - 1, ptr->y, '+'))
			return FALSE;
		break;
	default:
		if (!error_abort("Ungültige Richtung\n(show_left_turn)"))
			return FALSE;
		break;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL show_right_turn(_UBYTE *Text_map, PICE *ptr)
{
	switch (ptr->pos)
	{
	case North:
		if (!v_line(Text_map, ptr->x, ptr->y, ptr->h - 1))
			return FALSE;
		if (!h_line(Text_map, ptr->x + 1, ptr->y + ptr->h - 1, ptr->w - 1))
			return FALSE;
		if (!set_c(Text_map, ptr->x, ptr->y + ptr->h - 1, '+'))
			return FALSE;
		if (!set_c(Text_map, ptr->x + ptr->w - 1, ptr->y, '+'))
			return FALSE;
		break;
	case South:
		if (!v_line(Text_map, ptr->x + ptr->w - 1, ptr->y + 1, ptr->h - 1))
			return FALSE;
		if (!h_line(Text_map, ptr->x, ptr->y, ptr->w - 1))
			return FALSE;
		if (!set_c(Text_map, ptr->x, ptr->y + ptr->h - 1, '+'))
			return FALSE;
		if (!set_c(Text_map, ptr->x + ptr->w - 1, ptr->y, '+'))
			return FALSE;
		break;
	case East:
		if (!v_line(Text_map, ptr->x + ptr->w - 1, ptr->y, ptr->h - 1))
			return FALSE;
		if (!h_line(Text_map, ptr->x, ptr->y + ptr->h - 1, ptr->w - 1))
			return FALSE;
		if (!set_c(Text_map, ptr->x, ptr->y, '+'))
			return FALSE;
		if (!set_c(Text_map, ptr->x + ptr->w - 1, ptr->y + ptr->h - 1, '+'))
			return FALSE;
		break;
	case West:
		if (!v_line(Text_map, ptr->x, ptr->y + 1, ptr->h - 1))
			return FALSE;
		if (!h_line(Text_map, ptr->x + 1, ptr->y, ptr->w - 1))
			return FALSE;
		if (!set_c(Text_map, ptr->x, ptr->y, '+'))
			return FALSE;
		if (!set_c(Text_map, ptr->x + ptr->w - 1, ptr->y + ptr->h - 1, '+'))
			return FALSE;
		break;
	default:
		if (!error_abort("Ungültige Richtung\n(show_right_turn)"))
			return FALSE;
		break;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL show_t_junction(_UBYTE *Text_map, PICE *ptr)
{
	switch (ptr->pos)
	{
	case North:
		if (!h_line(Text_map, ptr->x, ptr->y + ptr->h - 1, ptr->w))
			return FALSE;
		if (!set_c(Text_map, ptr->x, ptr->y, '+'))
			return FALSE;
		if (!set_c(Text_map, ptr->x + ptr->w - 1, ptr->y, '+'))
			return FALSE;
		break;
	case South:
		if (!h_line(Text_map, ptr->x, ptr->y, ptr->w))
			return FALSE;
		if (!set_c(Text_map, ptr->x, ptr->y + ptr->h - 1, '+'))
			return FALSE;
		if (!set_c(Text_map, ptr->x + ptr->w - 1, ptr->y + ptr->h - 1, '+'))
			return FALSE;
		break;
	case East:
		if (!v_line(Text_map, ptr->x + ptr->w - 1, ptr->y, ptr->h))
			return FALSE;
		if (!set_c(Text_map, ptr->x, ptr->y, '+'))
			return FALSE;
		if (!set_c(Text_map, ptr->x, ptr->y + ptr->h - 1, '+'))
			return FALSE;
		break;
	case West:
		if (!v_line(Text_map, ptr->x, ptr->y, ptr->h))
			return FALSE;
		if (!set_c(Text_map, ptr->x + ptr->w - 1, ptr->y, '+'))
			return FALSE;
		if (!set_c(Text_map, ptr->x + ptr->w - 1, ptr->y + ptr->h - 1, '+'))
			return FALSE;
		break;
	default:
		if (!error_abort("Ungültige Richtung\n(show_t_junction)"))
			return FALSE;
		break;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL show_stairs_out(_UBYTE *Text_map, PICE *ptr)
{
	switch (ptr->pos)
	{
	case North:
		if (!v_line(Text_map, ptr->x, ptr->y, ptr->h - 1))
			return FALSE;
		if (!v_line(Text_map, ptr->x + ptr->w - 1, ptr->y, ptr->h - 1))
			return FALSE;
		if (!x_line(Text_map, ptr->x, ptr->y+ptr->h-1, ptr->w, 'V'))
			return FALSE;
		break;
	case South:
		if (!v_line(Text_map, ptr->x, ptr->y + 1, ptr->h - 1))
			return FALSE;
		if (!v_line(Text_map, ptr->x + ptr->w - 1, ptr->y + 1, ptr->h - 1))
			return FALSE;
		if (!x_line(Text_map, ptr->x, ptr->y, ptr->w, '^'))
			return FALSE;
		break;
	case East:
		if (!h_line(Text_map, ptr->x, ptr->y, ptr->w - 1))
			return FALSE;
		if (!h_line(Text_map, ptr->x, ptr->y + ptr->h - 1, ptr->w - 1))
			return FALSE;
		if (!y_line(Text_map, ptr->x + ptr->w - 1, ptr->y, ptr->h, '<'))
			return FALSE;
		break;
	case West:
		if (!h_line(Text_map, ptr->x + 1, ptr->y, ptr->w - 1))
			return FALSE;
		if (!h_line(Text_map, ptr->x + 1, ptr->y + ptr->h - 1, ptr->w - 1))
			return FALSE;
		if (!y_line(Text_map, ptr->x, ptr->y, ptr->h, '>'))
			return FALSE;
		break;
	default:
		if (!error_abort("Ungültige Richtung\n(show_stairs_out)"))
			return FALSE;
		break;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL show_stairs_down(_UBYTE *Text_map, PICE *ptr)
{
	switch (ptr->pos)
	{
	case North:
		if (!v_line(Text_map, ptr->x, ptr->y, ptr->h - 1))
			return FALSE;
		if (!v_line(Text_map, ptr->x + ptr->w - 1, ptr->y, ptr->h - 1))
			return FALSE;
		if (!x_line(Text_map, ptr->x, ptr->y + ptr->h - 1, ptr->w, '^'))
			return FALSE;
		break;
	case South:
		if (!v_line(Text_map, ptr->x, ptr->y + 1, ptr->h - 1))
			return FALSE;
		if (!v_line(Text_map, ptr->x + ptr->w - 1, ptr->y + 1, ptr->h - 1))
			return FALSE;
		if (!x_line(Text_map, ptr->x, ptr->y, ptr->w, 'V'))
			return FALSE;
		break;
	case East:
		if (!h_line(Text_map, ptr->x, ptr->y, ptr->w-1))
			return FALSE;
		if (!h_line(Text_map, ptr->x, ptr->y + ptr->h - 1, ptr->w - 1))
			return FALSE;
		if (!y_line(Text_map, ptr->x + ptr->w - 1, ptr->y, ptr->h, '>'))
			return FALSE;
		break;
	case West:
		if (!h_line(Text_map, ptr->x + 1, ptr->y, ptr->w - 1))
			return FALSE;
		if (!h_line(Text_map, ptr->x + 1, ptr->y + ptr->h - 1, ptr->w - 1))
			return FALSE;
		if (!y_line(Text_map, ptr->x, ptr->y, ptr->h, '<'))
			return FALSE;
		break;
	default:
		if (!error_abort("Ungültige Richtung\n(show_stairs_out)"))
			return FALSE;
		break;
	}
	return TRUE;
} 

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL show_small(_UBYTE *Text_map, PICE *ptr)
{
	return box(Text_map, ptr->x, ptr->y, ptr->w, ptr->h, 'S');
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL show_normal(_UBYTE *Text_map, PICE *ptr)
{
	return box(Text_map, ptr->x, ptr->y, ptr->w, ptr->h, 'N');
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL show_hazard(_UBYTE *Text_map, PICE *ptr)
{
	return box(Text_map, ptr->x, ptr->y, ptr->w, ptr->h, 'H');
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL show_large(_UBYTE *Text_map, PICE *ptr)
{
	return box(Text_map, ptr->x, ptr->y, ptr->w, ptr->h, 'X');
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL show_lair(_UBYTE *Text_map, PICE *ptr)
{
	return box(Text_map, ptr->x, ptr->y, ptr->w, ptr->h, 'L');
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL show_quest(_UBYTE *Text_map, PICE *ptr)
{
	return box(Text_map, ptr->x, ptr->y, ptr->w, ptr->h, 'Q');
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL show_big(_UBYTE *Text_map, PICE *ptr)
{
	return box(Text_map, ptr->x, ptr->y, ptr->w, ptr->h, 'B');
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL show_merscha(_UBYTE *Text_map, PICE *ptr)
{
	return box(Text_map, ptr->x, ptr->y, ptr->w, ptr->h, 'M');
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL show_test(_UBYTE *Text_map, PICE *ptr)
{
	return set_c(Text_map, ptr->x, ptr->y, '#');
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL show_door(_UBYTE *Text_map, PICE *ptr)
{
	return set_c(Text_map, ptr->x, ptr->y, '*');
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL show_secret_door(_UBYTE *Text_map, PICE *ptr)
{
	return set_c(Text_map, ptr->x, ptr->y, 'S');
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL show_x_number(_UBYTE *Text_map, PICE *ptr)
{
	_WORD i, x, y;
	_UBYTE *d;

	if (ptr->text != NULL)
	{
		d = (*(ptr->text));
		
		x = ptr->x + (ptr->w - (_WORD)strlen(d)) / 2;
		y = ptr->y + ptr->h / 2;
		
		for (i = 1; i <= N_DIGIT && d[i] != '\0'; i++)
		{
			if (!set_c(Text_map, x++, y, d[i]))
				return FALSE;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL show_y_number(_UBYTE *Text_map, PICE *ptr)
{
	_WORD i, x, y;
	_UBYTE *d;
	
	if (ptr->text != NULL)
	{
		d = (*(ptr->text));
		
		x = ptr->x + ptr->w / 2;
		y = ptr->y + (_WORD)strlen(d) + (ptr->h - (_WORD)strlen(d)) / 2;
		
		for (i = 1; i <= N_DIGIT && d[i] != '\0'; i++)
		{
			if (!set_c(Text_map, x, y--, d[i]))
				return FALSE;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL draw_text_map(_UBYTE *Text_map)
{
	_WORD i;
	PICE *ptr;
	_BOOL retV;
	
	retV = set_border(Text_map);
	
	ptr = Pice;
	for (i = 0; retV != FALSE && i < MAX_PICE; i++)
	{
		switch (ptr->type)
		{
		case PASSAGE:
			retV = show_passage(Text_map, ptr);
			break;
		case DEAD_END:
			retV = show_dead_end(Text_map, ptr);
			break;
		case LEFT_TURN:
			retV = show_left_turn(Text_map, ptr);
			break;
		case RIGHT_TURN:
			retV = show_right_turn(Text_map, ptr);
			break;
		case T_JUNCTION:
			retV = show_t_junction(Text_map, ptr);
			break;
		case STAIRS_OUT:
			retV = show_stairs_out(Text_map, ptr);
			break;
		case STAIRS_DOWN:
			retV = show_stairs_down(Text_map, ptr);
			break;
		case SMALL_ROOM:
			retV = show_small(Text_map, ptr);
			break;
		case NORMAL_ROOM:
			retV = show_normal(Text_map, ptr);
			break;
		case HAZARD:
			retV = show_hazard(Text_map, ptr);
			break;
		case LARGE_ROOM:
			retV = show_large(Text_map, ptr);
			break;
		case LAIR:
			retV = show_lair(Text_map, ptr);
			break;
		case QUEST:
			retV = show_quest(Text_map, ptr);
			break;
		case BIG:
			retV = show_big(Text_map, ptr);
			break;
		case MERSCHA:
			retV = show_merscha(Text_map, ptr);
			break;
		case TEST:
		case DOOR:
		case SECRET:
		case INVALID:
		case EMPTY:
			break;
		default:
			retV = error_abort("Ungültiger Type\n(draw_text_map)");
			break;
		}
		ptr++;
	}
	ptr = Pice;
	for (i = 0; retV != FALSE && i < MAX_PICE; i++)
	{
		switch (ptr->type)
		{
		case PASSAGE:
		case DEAD_END:
		{
			switch (ptr->pos)
			{
			case North:
			case South:
				retV = show_y_number(Text_map, ptr);
				break;
			case East:
			case West:
				retV = show_x_number(Text_map, ptr);
				break;
			default:
				retV = error_abort("Ungültige Richtung\n(draw_text_map)");
				break;
			}
			break;
		}
		case LEFT_TURN:
		case RIGHT_TURN:
		case T_JUNCTION:
		case STAIRS_OUT:
		case STAIRS_DOWN:
		case SMALL_ROOM:
		case NORMAL_ROOM:
		case HAZARD:
		case LARGE_ROOM:
		case LAIR:
		case QUEST:
		case BIG:
		case MERSCHA:
			retV = show_x_number(Text_map, ptr);
			break;
		case TEST:
			retV = show_test(Text_map, ptr);
			break;
		case DOOR:
			retV = show_door(Text_map, ptr);
			break;
		case SECRET:
			retV = show_secret_door(Text_map, ptr);
			break;
		case INVALID:
		case EMPTY:
			break;
		default:
			retV = error_abort("Ungültiger Type\n(draw_text_map)");
			break;
		}
		ptr++;
	}
	return retV;
}
