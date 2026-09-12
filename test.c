#include <test.h>
#include <map.h>
#include <move.h>
#include <random.h>
#include <defs.h>

#define LEFT	1
#define RIGHT	2
#define TOP		4


/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL _WORD test_end_sides(PICE *from)
{
	PICE foo;
	_WORD sides = 0;
	
	if (!init_left(&foo, PASSAGE, from))
		return -1;
	if (test_pice(&foo))
	{
		sides |= LEFT;
	}	
	if (!init_right(&foo, PASSAGE, from))
		return -1;
	if (test_pice(&foo))
	{
		sides |= RIGHT;
	}	
	if (!init_straight(&foo, PASSAGE, from))
		return -1;
	if (test_pice(&foo))
	{
		sides |= TOP;
	}
	return sides;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL ENDS test_end(PICE *from, ENDS endtype)
{
	PICE end, *old;
	_WORD sides;

	switch (endtype)
	{
	case Left_Turn:
		if (!init_straight(&end, LEFT_TURN, from))
			return Illegal_End;
		if (test_pice(&end))
		{
			sides = test_end_sides(&end);
			if (sides < 0)
				return Illegal_End;
			
			if (sides & LEFT)
			{
				return Left_Turn;
			}
			if (sides & RIGHT)
			{
				return Right_Turn;
			}
		}
		break;
	case Right_Turn:
		if (!init_straight(&end, LEFT_TURN, from))
			return Illegal_End;
		if (test_pice(&end))
		{
			sides = test_end_sides(&end);
			if (sides < 0)
				return Illegal_End;
			
			if (sides & RIGHT)
			{
				return Right_Turn;
			}
			if (sides & LEFT)
			{
				return Left_Turn;
			}
		}
		break;
	case T_Junction:
		if (!init_straight(&end, T_JUNCTION, from))
			return Illegal_End;
		if (test_pice(&end))
		{
			sides = test_end_sides(&end);
			if (sides < 0)
				return Illegal_End;

			if ((sides & LEFT) && (sides & RIGHT))
			{
				return T_Junction;
			}
			if (sides & LEFT)
			{
				return Left_Turn;
			}		
			if (sides & RIGHT)
			{
				return Right_Turn;
			}
		}
		break;
	case Corner:
		if (!init_straight(&end, T_JUNCTION, from))
			return Illegal_End;
		if (test_pice(&end))
		{
			sides = test_end_sides(&end);
			if (sides < 0)
				return Illegal_End;
			
			if ((sides & TOP) && (sides & LEFT) && (sides & RIGHT))
			{
				return Corner;
			}
			if ((sides & LEFT) && (sides & RIGHT))
			{
				return T_Junction;
			}
			if (sides & LEFT)
			{
				return Left_Turn;
			}		
			if (sides & RIGHT)
			{
				return Right_Turn;
			}
		}
		break;
	case Stairs_Down:
		if (!init_straight(&end, STAIRS_DOWN, from))
			return Illegal_End;
		if (test_pice(&end))
		{
			return Stairs_Down;
		}
		break;
	case Stairs_Out:
		if (!init_straight(&end, STAIRS_OUT, from))
			return Illegal_End;
		if (test_pice(&end))
		{
			return Stairs_Out;
		}
		break;
	case Dead_End:
		if (!init_straight(&end, T_JUNCTION, from))
			return Illegal_End;
		if (test_pice(&end))
		{
			return Dead_End;
		}
		break;
	case Illegal_End:
		return Illegal_End;
	default:
		if (!error_abort("Ungültiges Ende\n(test_end)"))
			return Illegal_End;
		return Dead_End;
	}
	
	if (get_square(end.x, end.y, &old) && old != NULL)
	{
		switch (old->type)
		{
		case LEFT_TURN:
			if (left(old) == back(&end))
			{
				return Ok;
			}
			break;
		case RIGHT_TURN:
			if (right(old) == back(&end))
			{
				return Ok;
			}
			break;
		case T_JUNCTION:
			if (left(old) == back(&end) ||
				right(old) == back(&end))
			{
				return Ok;
			}
			break;
		case CORNER:
			if (left(old) == back(&end) ||
				right(old) == back(&end) ||
				straight(old) == back(&end))
			{
				return Ok;
			}
			break;
		default:
			break;
		}
	}
	return Dead_End;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL SECTIONS test_passage(PICE *from, SECTIONS len)
{
	PICE foo;
	SECTIONS i;

	if (len == Illegal_Section)
		return Illegal_Section;
	for (i = 0; i < len; i++)	
	{
		if (!init_straight(&foo, PASSAGE, from))
			return Illegal_Section;
		if (!test_pice(&foo))
		{
			return i;
		}
		copy_pice(from, &foo);
	}
	return i;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL test_pice_from(PICE *from, PICE *test, DIRECTION pos, _BOOL *abort)
{
	PICE foo;
	
	copy_pice(&foo, test);
	*abort = FALSE;
	switch (pos)
	{
	case North:
		if (!pos_north(&foo, from))
		{
			*abort = TRUE;
			return FALSE;
		}
		break;
	case South:
		if (!pos_south(&foo, from))
		{
			*abort = TRUE;
			return FALSE;
		}
		break;
	case East:
		if (!pos_east(&foo, from))
		{
			*abort = TRUE;
			return FALSE;
		}
		break;
	case West:
		if (!pos_west(&foo, from))
		{
			*abort = TRUE;
			return FALSE;
		}
		break;
	case Illegal_Dir:
		*abort = TRUE;
		return FALSE;
	default:
		if (!error_abort("Ungültige Richtung\n(test_pice_from)"))
		{
			*abort = TRUE;
			return FALSE;
		}
		return FALSE;
	}
	return test_pice(&foo);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _WORD find_passage_rooms(PICE *from, SECTIONS len, PICE *room, _WORD count)
{
	PICE foo1, foo2;
	_WORD i, n = 0, dummy;
	_BOOL abort;
	
	do
	{
		copy_pice(&foo2, from);
		for (i = 0; i < len; i++)
		{
			if (!init_straight(&foo1, PASSAGE, &foo2))
				return 0;
			copy_pice(&foo2, &foo1);
			
			if (test_pice_from(&foo1, room, left(&foo1), &abort))
			{
				if (++n == count)
				{
					pos_left(room, &foo1);
					return n;
				}
			}
			if (abort)
				return 0;
			if (test_pice_from(&foo1, room, right(&foo1), &abort))
			{
				if (++n == count)
				{
					pos_right(room, &foo1);
					return n;
				}
			}
			if (abort)
				return 0;
		}

		dummy = room->w;
		room->w = room->h;
		room->h = dummy;
	} while (room->w > room->h);

	if (count)
	{
		if (!error_abort("Fataler Fehler!\n(find_passage_room)"))
			return 0;
	}
	
	return n;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL ROOMS test_passage_room(PICE *from, SECTIONS len, ROOMS roomtype)
{
	PICE room;
	_WORD rooms, roll;
	
	switch (roomtype)
	{
	case Merscha:
	case Big:
		switch (roomtype)
		{
		case Merscha:
			init_at(&room, MERSCHA, from);
			break;
		case Big:
			init_at(&room, BIG, from);
			break;
		default:
			break;
		}
		rooms = find_passage_rooms(from, len, &room, 0);
		if (rooms)
		{
			break;
		}
		roomtype = Lair;
		/* FALLTHROUGH */
	case Quest:
	case Lair:
	case Large:
		switch (roomtype)
		{
		case Quest:
			init_at(&room, QUEST, from);
			break;
		case Lair:
			init_at(&room, LAIR, from);
			break;
		case Large:
			init_at(&room, LARGE_ROOM, from);
			break;
		default:
			break;
		}
		rooms = find_passage_rooms(from, len, &room, 0);
		if (rooms)
		{
			break;
		}
		roomtype = Hazard;
		/* FALLTHROUGH */
	case Small:
	case Normal:
	case Hazard:
		switch (roomtype)
		{
		case Hazard:
			init_at(&room, HAZARD, from);
			break;
		case Normal:
			init_at(&room, NORMAL_ROOM, from);
			break;
		case Small:
			init_at(&room, SMALL_ROOM, from);
			break;
		default:
			break;
		}
		rooms = find_passage_rooms(from, len, &room, 0);
		break;
	case Invalid:
		return Invalid;
	default:
		if (!error_abort("Ungültiger Raumtype\n(test_passage_room)"))
			return Invalid;
		return Invalid;
	}
	
	if (rooms)
	{
		roll = get_rand(rooms) + 1;
		if (find_passage_rooms(from, len, &room, roll))
		{
			copy_pice(from, &room);
			return roomtype;
		}
	}
	return Invalid;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL are_room_doors_ok(PICE *ptr)
{
	switch (ptr->type)
	{
	case SMALL_ROOM:
	case NORMAL_ROOM:
	case HAZARD:
		switch (ptr->feature)
		{
		case Trapdoor:
		case Chasm:
		case Bridge:
			return FALSE;
		default:
			break;
		}
		return TRUE;
	case LARGE_ROOM:
	case LAIR:
	case QUEST:
		return TRUE;
	case BIG:
	case MERSCHA:
	default:
		break;
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _WORD find_room_rooms(PICE *from, PICE *room, _WORD count)
{
	PICE foo_top, foo_right, foo_left;
	_WORD i, n = 0, dummy;
	_WORD side_size, top_size;
	
	side_size = length(from) / Section.h;
	if (side_size == 0)
		return 0;
	top_size = wide(from) / Section.h;
	if (top_size == 0)
		return 0;
	
	do
	{
		copy_pice(&foo_top, room);
		copy_pice(&foo_left, room);
		copy_pice(&foo_right, room);
		pos_straight(&foo_top, from);
		pos_left(&foo_left, from);
		pos_right(&foo_right, from);
				
		for (i = 0; i < top_size; i++)
		{
			if (test_pice(&foo_top))
			{
				if (++n == count)
				{
					copy_pice(room, &foo_top);
					return n;
				}
			}
			switch (from->pos)
			{
			case North:
			case South:
				foo_top.x += Section.h;
				break;
			case East:
			case West:
				foo_top.y += Section.h;
				break;
			case Illegal_Dir:
				return 0;
			default:
				if (!error_abort("Ungültige Richtungsangabe\n(find_room_room)"))
					return 0;
				return 0;
			}
		}
		
		if (are_room_doors_ok(from))
		{
			for (i = 0; i < side_size; i++)
			{
				if (test_pice(&foo_left))
				{
					if (++n == count)
					{
						copy_pice(room, &foo_left);
						return n;
					}
				}
				if (test_pice(&foo_right))
				{
					if (++n == count)
					{
						copy_pice(room, &foo_right);
						return n;
					}
				}
				switch (from->pos)
				{
				case North:
				case South:
					foo_right.y += Section.h;
					foo_left.y += Section.h;
					break;
				case East:
				case West:
					foo_right.x += Section.h;
					foo_left.x += Section.h;
					break;
				case Illegal_Dir:
					return 0;
				default:
					if (!error_abort("Ungültige Richtungsangabe\n(find_room_room)"))
						return FALSE;
					return 0;
				}
			}
		}
		dummy = room->w;
		room->w = room->h;
		room->h = dummy;
	} while (room->w > room->h);

	if (count)
	{
		if (!error_abort("Fataler Fehler!\n(find_room_room)"))
			return 0;
		return 0;
	}
	
	return n;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL ROOMS test_room_room(PICE *from, ROOMS roomtype)
{
	PICE room;
	_WORD rooms, roll;
	
	switch (roomtype)
	{
	case Merscha:
	case Big:
		switch (roomtype)
		{
		case Merscha:
			init_at(&room, MERSCHA, from);
			break;
		case Big:
			init_at(&room, BIG, from);
			break;
		default:
			break;
		}
		rooms = find_room_rooms(from, &room, 0);
		if (rooms)
		{
			break;
		}
		roomtype = Lair;
		/* FALLTHROUGH */
	case Quest:
	case Lair:
	case Large:
		switch (roomtype)
		{
		case Quest:
			init_at(&room, QUEST, from);
			break;
		case Lair:
			init_at(&room, LAIR, from);
			break;
		case Large:
			init_at(&room, LARGE_ROOM, from);
			break;
		default:
			break;
		}
		rooms = find_room_rooms(from, &room, 0);
		if (rooms)
		{
			break;
		}
		roomtype = Hazard;
		/* FALLTHROUGH */
	case Hazard:
	case Normal:
	case Small:
		switch (roomtype)
		{
		case Hazard:
			init_at(&room, HAZARD, from);
			break;
		case Normal:
			init_at(&room, NORMAL_ROOM, from);
			break;
		case Small:
			init_at(&room, SMALL_ROOM, from);
			break;
		default:
			break;
		}
		rooms = find_room_rooms(from, &room, 0);
		break;
	case Invalid:
		return Invalid;
	default:
		if (!error_abort("Ungültiger Raumtype\n(test_room_room)"))
			return Invalid;
		return Invalid;
	}
	
	if (rooms)
	{
		roll = get_rand(rooms) + 1;
		if (find_room_rooms(from, &room, roll))
		{
			copy_pice(from, &room);
			return roomtype;
		}
	}
	return Invalid;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _WORD find_room_passages(PICE *from, _WORD count)
{
	PICE foo_top, foo_left, foo_right;
	_WORD i, n = 0;
	_WORD side_size, top_size;
	
	side_size = length(from)/Section.h;
	top_size = wide(from)/Section.h;
	
	if (!init_pice(&foo_top, PASSAGE, 0, 0, left(from)))
		return 0;
	if (!pos_straight(&foo_top, from))
		return 0;
	if (!init_pice(&foo_left, PASSAGE, 0, 0, straight(from)))
		return 0;
	if (!pos_left(&foo_left, from))
		return 0;
	if (!init_pice(&foo_right, PASSAGE, 0, 0, straight(from)))
		return 0;
	if (!pos_right(&foo_right, from))
		return 0;
	
	for (i = 0; i < top_size; i++)
	{
		if (test_pice(&foo_top))
		{
			if (++n == count)
			{
				copy_pice(from, &foo_top);
				return n;
			}
		}
		switch (from->pos)
		{
		case North:
		case South:
			foo_top.x += Section.h;
			break;
		case East:
		case West:
			foo_top.y += Section.h;
			break;
		case Illegal_Dir:
			return 0;
		default:
			if (!error_abort("Ungültige Richtungsangabe\n(find_room_passages)"))
				return 0;
			return 0;
		}
	}
	if (are_room_doors_ok(from))
	{
		for (i = 0; i < side_size; i++)
		{
			if (test_pice(&foo_left))
			{
				if (++n == count)
				{
					copy_pice(from, &foo_left);
					return n;
				}
			}
			if (test_pice(&foo_right))
			{
				if (++n == count)
				{
					copy_pice(from, &foo_right);
					return n;
				}
			}
			switch (from->pos)
			{
			case North:
			case South:
				foo_right.y += Section.h;
				foo_left.y += Section.h;
				break;
			case East:
			case West:
				foo_right.x += Section.h;
				foo_left.x += Section.h;
				break;
			case Illegal_Dir:
				return 0;
			default:
				if (!error_abort("Ungültige Richtungsangabe\n(find_room_passages)"))
					return 0;
				return 0;
			}
		}
	}

	if (count)
	{
		if (!error_abort("Fataler Fehler!\n(find_room_passages)"))
			return 0;
		return 0;
	}
	
	return n;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL test_room_passage(PICE *from)
{
	PICE passage;
	_WORD passages, roll;
	
	copy_pice(&passage, from);
	passages = find_room_passages(&passage, 0);
	if (passages)
	{
		roll = get_rand(passages)+1;
		if (find_room_passages(from, roll))
		{
			return TRUE;
		}
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _WORD find_room_doors(PICE *from, _WORD count)
{
	PICE foo_top, foo_left, foo_right, *ptr;
	_WORD i, n = 0;
	_WORD side_size, top_size;
	
	side_size = length(from);
	if (side_size == 0)
		return 0;
	top_size = wide(from);
	if (top_size == 0)
		return 0;
	if (!init_straight(&foo_top, DOOR, from))
		return 0;
	if (!init_left(&foo_left, DOOR, from))
		return 0;
	if (!init_right(&foo_right, DOOR, from))
		return 0;
	
	for (i = 0; i < top_size; i++)
	{
		if (get_square(foo_top.x, foo_top.y, &ptr) && ptr != NULL)
		{
			if (++n == count)
			{
				copy_pice(from, &foo_top);
				return n;
			}
		}
		switch (from->pos)
		{
		case North:
		case South:
			foo_top.x++;
			break;
		case East:
		case West:
			foo_top.y++;
			break;
		case Illegal_Dir:
			return 0;
		default:
			if (!error_abort("Ungültige Richtungsangabe\n(find_room_doors)"))
				return 0;
			return 0;
		}
	}
	
	if (are_room_doors_ok(from))
	{
		for (i = 0; i < side_size; i++)
		{
			if (get_square(foo_left.x, foo_left.y, &ptr) && ptr != NULL)
			{
				if (++n == count)
				{
					copy_pice(from, &foo_left);
					return n;
				}
			}
			if (get_square(foo_right.x, foo_right.y, &ptr) && ptr != NULL)
			{
				if (++n == count)
				{
					copy_pice(from, &foo_right);
					return n;
				}
			}
			switch (from->pos)
			{
			case North:
			case South:
				foo_right.y++;
				foo_left.y++;
				break;
			case East:
			case West:
				foo_right.x++;
				foo_left.x++;
				break;
			case Illegal_Dir:
				return 0;
			default:
				if (!error_abort("Ungültige Richtungsangabe\n(find_room_doors)"))
					return 0;
				return 0;
			}
		}
	}

	if (count)
	{
		if (!error_abort("Fataler Fehler!\n(find_room_doors)"))
			return 0;
		return 0;
	}
	
	return n;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL test_room_door(PICE *from)
{
	PICE door;
	_WORD doors, roll;
	
	copy_pice(&door, from);
	doors = find_room_doors(&door, 0);
	if (doors)
	{
		roll = get_rand(doors)+1;
		if (find_room_doors(from, roll))
		{
			return TRUE;
		}
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _WORD find_passage_doors(PICE *from, SECTIONS len, _WORD count)
{
	PICE foo, foo_right, foo_left, *ptr;
	_WORD i, j, n = 0;
	
	if (!init_at(&foo, PASSAGE, from))
		return 0;
	
	for (i = 0; i < len; i++)
	{
		if (!init_left(&foo_left, DOOR, &foo))
			return 0;
		if (!init_right(&foo_right, DOOR, &foo))
			return 0;
		
		if (!init_straight(from, PASSAGE, &foo))
			return 0;
		copy_pice(&foo, from);
		
		for (j = 0; j < Section.h; j++)
		{
			if (get_square(foo_left.x, foo_left.y, &ptr) && ptr != NULL)
			{
				if (are_room_doors_ok(from))
				{	
					if (++n == count)
					{
						copy_pice(from, &foo_left);
						return n;
					}
				}
			}
			if (get_square(foo_right.x, foo_right.y, &ptr) && ptr != NULL)
			{
				if (are_room_doors_ok(from))
				{	
					if (++n == count)
					{
						copy_pice(from, &foo_right);
						return n;
					}
				}
			}
			switch (foo.pos)
			{
			case North:
			case South:
				foo_left.y++;
				foo_right.y++;
				break;
			case East:
			case West:
				foo_left.x++;
				foo_right.x++;
				break;
			case Illegal_Dir:
				return 0;
			default:
				if (!error_abort("Ungültige Richtungsangabe\n(find_passage_doors)"))
					return 0;
				return 0;
			}
		}
	}
	
	if (count)
	{
		if (!error_abort("Fataler Fehler!\n(find_passage_door)"))
			return 0;
		return 0;
	}
	
	return n;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL test_passage_door(PICE *from, SECTIONS len)
{
	PICE door;
	_WORD doors, roll;
	
	copy_pice(&door, from);
	doors = find_passage_doors(&door, len, 0);
	if (doors)
	{
		roll = get_rand(doors) + 1;
		if (find_passage_doors(from, len, roll))
		{
			return TRUE;
		}
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _WORD find_doors(PICE *from, _WORD count)
{
	PICE foo, *ptr;
	_WORD i, n = 0;
	_WORD size;
	
	size = wide(from);
	if (size == 0)
		return 0;
	if (!init_back(&foo, DOOR, from))
		return 0;
	
	for (i = 0; i < size; i++)
	{
		if (get_square(foo.x, foo.y, &ptr) && ptr != NULL)
		{
			if (++n == count)
			{
				copy_pice(from, &foo);
				return n;
			}
		}
		
		switch (straight(&foo))
		{
		case North:
		case South:
			foo.x++;
			break;
		case East:
		case West:
			foo.y++;
			break;
		case Illegal_Dir:
			return 0;
		default:
			if (!error_abort("Ungültige Richtungsangabe\n(find_doors)"))
				return 0;
			return 0;
		}
	}
	return n;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL test_door(PICE *from)
{
	PICE door;
	_WORD doors, roll;
	
	copy_pice(&door, from);
	doors = find_doors(&door, 0);
	if (doors)
	{
		roll = get_rand(doors)+1;
		if (find_doors(from, roll))
		{
			return TRUE;
		}
	}
	return FALSE;
}
