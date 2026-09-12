#include <set.h>
#include <queue.h>
#include <move.h>
#include <test.h>
#include <defs.h>

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL _BOOL push_straight(PICE *ptr)
{
	return push_passage(ptr);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL push_left(PICE *ptr)
{
	PICE foo;
	
	copy_pice(&foo, ptr);
	foo.pos = left(ptr);
	if (foo.pos == Illegal_Dir)
		return FALSE;
	return push_passage(&foo);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL push_right(PICE *ptr)
{
	PICE foo;
	
	copy_pice(&foo, ptr);
	foo.pos = right(ptr);
	if (foo.pos == Illegal_Dir)
		return FALSE;
	return push_passage(&foo);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL set_end(PICE *from, ENDS type)
{
	PICE end;
	
	switch (type)
	{
	case Left_Turn:
		if (!init_straight(&end, LEFT_TURN, from))
			return FALSE;
		if (!left_turn_features(&end))
			return FALSE;
		if (insert_pice(&end))
		{
			if (!push_left(&end))
				return FALSE;
		} else
		{
			if (!error_abort("Ende 'Left Turn' konnte nicht gesetzt werden\n(set_end)"))
				return FALSE;
		}
		break;
	case Right_Turn:
		if (!init_straight(&end, RIGHT_TURN, from))
			return FALSE;
		if (!right_turn_features(&end))
			return FALSE;
		if (insert_pice(&end))
		{
			if (!push_right(&end))
				return FALSE;
		} else
		{
			if (!error_abort("Ende 'Right Turn' konnte nicht gesetzt werden\n(set_end)"))
				return FALSE;
		}
		break;
	case T_Junction:
		if (!init_straight(&end, T_JUNCTION, from))
			return FALSE;
		if (!t_junction_features(&end))
			return FALSE;
		if (insert_pice(&end))
		{
			if (!push_left(&end))
				return FALSE;
			if (!push_right(&end))
				return FALSE;
		} else
		{
			if (!error_abort("Ende 'T Junction' konnte nicht gesetzt werden\n(set_end)"))
				return FALSE;
		}
		break;
	case Corner:
		if (!init_straight(&end, CORNER, from))
			return FALSE;
		if (!corner_features(&end))
			return FALSE;
		if (insert_pice(&end))
		{
			if (!push_straight(&end))
				return FALSE;
			if (!push_left(&end))
				return FALSE;
			if (!push_right(&end))
				return FALSE;
		} else
		{
			if (!error_abort("Ende 'Corner' konnte nicht gesetzt werden\n(set_end)"))
				return FALSE;
		}
		break;
	case Stairs_Down:
		if (!init_straight(&end, STAIRS_DOWN, from))
			return FALSE;
		if (!stairs_down_features(&end))
			return FALSE;
		if (!insert_pice(&end))
		{
			if (!error_abort("Ende 'Stairs Down' konnte nicht gesetzt werden\n(set_end)"))
				return FALSE;
		}
		break;
	case Stairs_Out:
		if (!init_straight(&end, STAIRS_OUT, from))
			return FALSE;
		if (!stairs_out_features(&end))
			return FALSE;
		if (!insert_pice(&end))
		{
			if (!error_abort("Ende 'Stairs Out' konnte nicht gesetzt werden\n(set_end)"))
				return FALSE;
		}
		break;
	case Dead_End:
		if (!init_straight(&end, DEAD_END, from))
			return FALSE;
		if (!dead_end_features(&end))
			return FALSE;
		if (!insert_pice(&end))
		{
			if (!error_abort("Ende 'Dead End' konnte nicht gesetzt werden\n(set_end)"))
				return FALSE;
		}
		break;
	case Ok:
		break;
	case Illegal_End:
		return FALSE;
	default:
		if (!error_abort("Ungültiges Ende\n(set_end)"))
			return FALSE;
		break;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL set_passage(PICE *from, SECTIONS len, FEATURES feature)
{
	PICE foo;
	
	if (len == Illegal_Section)
		return FALSE;
	if (len != Section0)
	{
		if (!init_straight(&foo, PASSAGE, from))
			return FALSE;
		
		if (feature == Wandering_Monsters)
		{
			if (!wandering_monsters(&foo))
				return FALSE;
		} else
		{
			if (!passage_features(&foo))
				return FALSE;
		}
	}
	
	while (len--)
	{
		if (insert_pice(&foo))
		{
			copy_pice(from, &foo);
			if (!init_straight(&foo, PASSAGE, from))
				return FALSE;
		} else
		{
			if (!error_abort("Passage konnte nicht gesetzt werden\n(set_passage)"))
				return FALSE;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL set_room(PICE *ptr, ROOMS roomtype)
{
	switch (roomtype)
	{
	case Small:
		if (!small_features(ptr))
			return FALSE;
		if (insert_pice(ptr))
		{
			if (!push_room(ptr))
				return FALSE;
		} else
		{
			if (!error_abort("'Small' Raum konnte nicht gesetzt werden\n(set_room)"))
				return FALSE;
		}
		break;
	case Normal:
		if (!normal_features(ptr))
			return FALSE;
		if (insert_pice(ptr))
		{
			if (!push_room(ptr))
				return FALSE;
		} else
		{
			if (!error_abort("'Normal' Raum konnte nicht gesetzt werden\n(set_room)"))
				return FALSE;
		}
		break;
	case Hazard:
		if (!hazard_features(ptr))
			return FALSE;
		if (insert_pice(ptr))
		{
			if (ptr->feature != Trapdoor)
			{
				if (!push_room(ptr))
					return FALSE;
			}
		} else
		{
			if (!error_abort("'Hazard' Raum konnte nicht gesetzt werden\n(set_room)"))
				return FALSE;
		}
		break;
	case Large:
		if (!large_features(ptr))
			return FALSE;
		if (insert_pice(ptr))
		{
			if (!push_room(ptr))
				return FALSE;
		} else
		{
			if (!error_abort("'Large' Raum konnte nicht gesetzt werden\n(set_room)"))
				return FALSE;
		}
		break;
	case Lair:
		if (!lair_features(ptr))
			return FALSE;
		if (insert_pice(ptr))
		{
			if (!push_room(ptr))
				return FALSE;
		} else
		{
			if (!error_abort("'Lair' Raum konnte nicht gesetzt werden\n(set_room)"))
				return FALSE;
		}
		break;
	case Quest:
		if (!quest_features(ptr))
			return FALSE;
		if (insert_pice(ptr))
		{
			if (!push_room(ptr))
				return FALSE;
		} else
		{
			if (!error_abort("'Quest' Raum konnte nicht gesetzt werden\n(set_room)"))
				return FALSE;
		}
		break;
	case Big:
		if (!big_features(ptr))
			return FALSE;
		if (insert_pice(ptr))
		{
			if (!push_room(ptr))
				return FALSE;
		} else
		{
			if (!error_abort("'Big' Raum konnte nicht gesetzt werden\n(set_room)"))
				return FALSE;
		}
		break;
	case Merscha:
		if (!merscha_features(ptr))
			return FALSE;
		if (insert_pice(ptr))
		{
			if (!push_room(ptr))
				return FALSE;
		} else
		{
			if (!error_abort("'Merscha' Raum konnte nicht gesetzt werden\n(set_room)"))
				return FALSE;
		}
		break;
	case Invalid:
		return FALSE;
	default:
		if (!error_abort("Ungültiger Raum\n(set_room)"))
			return FALSE;
		break;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL set_door(PICE *ptr, TYPE doortype)
{
	PICE door;

	copy_pice(&door, ptr);
	if (!init_pice(&door, doortype, ptr->x, ptr->y, back(ptr)))
		return FALSE;
	if (!insert_pice(&door))
	{
		if (!error_abort("Tür konnte nicht gesetzt werden\n(set_door)"))
			return FALSE;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL set_room_door(PICE *ptr, TYPE doortype)
{
	PICE door;
	
	copy_pice(&door, ptr);
	if (test_door(&door))
	{
		if (!set_door(&door, doortype))
			return FALSE;
	} else
	{
		if (!error_abort("Ungültige Tür-Position\n(set_room_door)"))
			return FALSE;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL set_passage_door(PICE *ptr, SECTIONS len, TYPE doortype)
{
	PICE door;

	copy_pice(&door, ptr);
	if (test_passage_door(&door, len))
	{
		if (!set_door(&door, doortype))
			return FALSE;
	}
	return TRUE;
}
