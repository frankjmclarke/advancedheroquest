#include <makemap.h>
#include <rolldice.h>
#include <move.h>
#include <test.h>
#include <set.h>
#include <map.h>
#include <queue.h>
#include <defs.h>

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL _BOOL do_passage_room(PICE *from, SECTIONS len, ROOMS roomtype, TYPE doortype)
{
	PICE room;

	copy_pice(&room, from);
	roomtype = test_passage_room(&room, len, roomtype);
	if (roomtype != Invalid)
	{
		if (!set_room(&room, roomtype))
			return FALSE;
		if (!set_room_door(&room, doortype))
			return FALSE;
		return TRUE;
	}
	
	if (!set_passage_door(from, len, doortype))
		return FALSE;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL do_passage_blocked(PICE *from)
{
	PICE *ptr, *foo_ptr;
	PICE foo;
	
	if (get_square(from->x, from->y, &ptr) && ptr != NULL)
	{
		switch (ptr->type)
		{
		case LEFT_TURN:
		case RIGHT_TURN:
			if (!init_pice(&foo, DEAD_END, 0, 0, straight(ptr)))
				return FALSE;
			if (!pos_back(&foo, ptr))
				return FALSE;
			get_square(foo.x, foo.y, &foo_ptr);
			if (foo_ptr != NULL && foo_ptr->type == PASSAGE)
			{
				delete_pice(ptr);
				foo_ptr->type = DEAD_END;
			} else
			{
				ptr->type = STAIRS_OUT;
			}
			return TRUE;
		case T_JUNCTION:
			if (left(ptr) == straight(from))
			{
				ptr->type = RIGHT_TURN;
				return TRUE;
			}		
			if (right(ptr) == straight(from))
			{
				ptr->type = LEFT_TURN;
				return TRUE;
			}
			/* FALLTHROUGH */
		case CORNER:
			if (straight(ptr) == straight(from) ||
				left(ptr) == straight(from) ||
				right(ptr) == straight(from))
			{
				ptr->type = T_JUNCTION;
				ptr->pos = straight(from);
				if (ptr->pos == Illegal_Dir)
					return FALSE;
				return TRUE;
			}
			break;
		default:
			break;
		}
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL do_passage(PICE *from, SECTIONS len, ENDS end, FEATURES feature, _BOOL *abort)
{
	PICE foo;
	
	*abort = FALSE;
	if (len == Illegal_Section || end == Illegal_End || feature == Illegal_Feature)
	{
		*abort = TRUE;
		return FALSE;
	}
	copy_pice(&foo, from);
	len = test_passage(&foo, len);
	if (len == Illegal_Section)
	{
		*abort = TRUE;
		return FALSE;
	}
	if (len == Section0)
	{
		do_passage_blocked(from);
		return FALSE;
	}
	end = test_end(&foo, end);
	
	copy_pice(&foo, from);
	if (end == Dead_End)
	{
		if (!set_passage(&foo, len-1, feature))
		{
			*abort = TRUE;
			return FALSE;
		}
	} else
	{
		if (!set_passage(&foo, len, feature))
		{
			*abort = TRUE;
			return FALSE;
		}
	}
	if (!set_end(&foo, end))
	{
		*abort = TRUE;
		return FALSE;
	}
	
	copy_pice(&foo, from);
	switch (feature)
	{
	case Wandering_Monsters:
	case Nothing:
		if (end == Dead_End && (secret_door() == Room_Door1 || secret_door() == Room_Door1))
		{
			if (!do_passage_room(&foo, len, room_type(), SECRET))
			{
				*abort = TRUE;
				return FALSE;
			}
		}
		break;
	case Door2:
		if (!do_passage_room(&foo, len, room_type(), DOOR))
		{
			*abort = TRUE;
			return FALSE;
		}
		if (!do_passage_room(&foo, len, room_type(), DOOR))
		{
			*abort = TRUE;
			return FALSE;
		}
		break;
	case Door1:
		if (!do_passage_room(&foo, len, room_type(), DOOR))
		{
			*abort = TRUE;
			return FALSE;
		}
		break;
	case Illegal_Feature:
		*abort = TRUE;
		return FALSE;
	default:
		if (!error_abort("Ungültiger Inhalt\n(do_passage)"))
		{
			*abort = TRUE;
			return FALSE;
		}
		break;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL do_room_room(PICE *from, ROOMORPASSAGE door, TYPE doortype)
{
	PICE room, passage, foo;
	ROOMS roomtype;
	_BOOL abort;
	
	switch (door)
	{
	case Room:
		copy_pice(&room, from);
		roomtype = test_room_room(&room, room_type());
		if (roomtype != Invalid)
		{
			if (!set_room(&room, roomtype))
				return FALSE;
			if (!set_room_door(&room, doortype))
				return FALSE;
			return TRUE;
		}
		/* FALLTHROUGH */
	case Passage:
		copy_pice(&passage, from);
		if (test_room_passage(&passage))
		{
			copy_pice(&foo, &passage);
			foo.pos = left(&passage);
			if (foo.pos == Illegal_Dir)
				return FALSE;
			if (!do_passage(&foo, passage_length(), passage_end(), passage_feature(), &abort))
			{
				if (abort)
					return FALSE;
				copy_pice(&foo, &passage);
				foo.pos = right(&passage);
				if (foo.pos == Illegal_Dir)
					return FALSE;
				if (!do_passage(&foo, passage_length(), passage_end(), passage_feature(), &abort))
				{
					if (abort)
						return FALSE;
					break;
				} else
				{
					copy_pice(&foo, &passage);
					foo.pos = left(&passage);
					foo.type = DEAD_END;
				}
			} else
			{
				copy_pice(&foo, &passage);
				foo.pos = right(&passage);
				if (foo.pos == Illegal_Dir)
					return FALSE;
				if (!do_passage(&foo, passage_length(), passage_end(), passage_feature(), &abort))
				{
					if (abort)
						return FALSE;
					copy_pice(&foo, &passage);
					foo.pos = right(&passage);
					if (foo.pos == Illegal_Dir)
						return FALSE;
					foo.type = DEAD_END; 
				}
			}
			if (insert_pice(&foo))
			{
				if (!set_room_door(&passage, doortype))
					return FALSE;
				return TRUE;
			} else
			{
				if (!error_abort("Passage konnte nicht gesetzt werden\n(do_room_room)"))
					return FALSE;
			}
		}
		break;
	case Illegal_Passage:
		return FALSE;
	default:
		if (!error_abort("Ungültiger Türtype\n(do_room_room)"))
			return FALSE;
		return FALSE;
	}
	
	copy_pice(&room, from);
	if (test_room_door(&room))
	{
		if (!set_door(&room, doortype))
			return FALSE;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL do_room(PICE *room, ROOMDOORS doors)
{
	switch (doors)
	{
	case None:
		if (secret_door() == Room_Door1 || secret_door() == Room_Door1 || secret_door() == Room_Door1)
		{
			if (!do_room_room(room, room_or_passage(), SECRET))
				return FALSE;
		}
		break;
	case Door2:
		if (!do_room_room(room, room_or_passage(), DOOR))
			return FALSE;
		if (!do_room_room(room, room_or_passage(), DOOR))
			return FALSE;
		break;
	case Door1:
		if (!do_room_room(room, room_or_passage(), DOOR))
			return FALSE;
		break;
	case Illegal_Room_Doors:
		return FALSE;
	default:
		if (!error_abort("Ungültige Türenanzahl\n(do_room)"))
			return FALSE;
		return FALSE;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL do_start(PICE *from)
{
	_BOOL abort;
	
	if (!init_at(from, STAIRS_OUT, from))
		return FALSE;
	if (test_pice(from))
	{
		if (insert_pice(from))
		{
			from->pos = back(from);
			if (from->pos == Illegal_Dir)
				return FALSE;
			if (!do_passage(from, Section2, T_Junction, Nothing, &abort))
			{
				if (abort)
					return FALSE;
			}
		} else
		{
			if (!error_abort("Start 'Stairs Up' konnte nicht gesetzt werden\n(do_start)"))
				return FALSE;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL makemap(_WORD x_size, _WORD y_size, _WORD x_start, _WORD y_start, DIRECTION pos)
{
	PICE last;
	_BOOL abort;
	
	if (!init_map(x_size, y_size) ||
		!init_table())
	{
		return FALSE;
	}

	last.x = x_start;
	last.y = y_start;
	last.pos = pos;
	
	if (!init_queues())
		return FALSE;
	if (!do_start(&last))
		return FALSE;
	
	for (;;)
	{
		if (pop_passage(&last))
		{
			if (!do_passage(&last, passage_length(), passage_end(), passage_feature(), &abort))
			{
				if (abort)
					return FALSE;
			}
		} else
		{
			if (pop_room(&last))
			{
				if (!do_room(&last, room_doors()))
					return FALSE;
			} else
			{
				return TRUE;
			}
		}
	}
}
