#include <statist.h>

typedef struct statistik
{
	_UBYTE *name;
	_WORD count;
	struct statistik **ptr;
} STATISTIK;

GLOBAL _BOOL make_statistik(_BOOL questroom)
{
	_WORD i;
	_WORD last = MAX_PICE;
	
	for (i = 0; i < MAX_PICE; i++)
	{
		switch (pice->type)
		{
		case ROOM:
			switch (pice->feature)
			{
			case Nothing:
			case Wandering_Monsters:
				return normal;
			default:
				break;
			}
			error_abort("Unknown room feature\n(pice_icon)");
			return normal;
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
			default:
				break;
			}
			error_abort("Unknown hazard feature\n(make_statistik)");
			return hazard;
		case LAIR:
			switch (pice->feature)
			{
			case Nothing:
			case Wandering_Monsters:
				return large;
			case Chest:
				return lair;
			default:
				break;
			}
			error_abort("Unknown lair feature\n(make_statistik)");
			return large;
		case QUEST:
			switch (pice->feature)
			{
			case Nothing:
			case Wandering_Monsters:
				return large;
			case Chest:
				return lair;
			case Stairs_and_Chest:
				return quest;
			default:
				break;
			}
			error_abort("Unknown lair feature\n(make_statistik)");
			return large;
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
		error_abort("Invalid section\n(make_statistik)");
		return NULL;
	}
	return NULL;
}
