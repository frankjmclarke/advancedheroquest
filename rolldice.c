#include <rolldice.h>
#include <defs.h>
#include <random.h>
#include <mem.h>
#include <boxf.h>
#include <stdlib.h>
#include <string.h>

LOCAL _WORD Count_nr;

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL FEATURES referenz_table(TABLE *table)
{
	_WORD roll;
	ENTRY *entry;
	FEATURE *feature;
	
	if (table == NULL)
	{
		if (!error_abort("Tabelle existiert nicht (referenz_table)"))
			return Illegal_Feature;
	} else
	{
		roll = dice_roll(table->n, table->dice);
		entry = table->entry;
		while (entry != NULL)
		{
			if (roll <= entry->roll)
			{
				feature = entry->ptr;
				while (feature != NULL)
				{
					switch (feature->type)
					{
					case SUBTABLE:
						return referenz_table(feature->ptr.subtable);
					case ROOMFEATURE:
						return feature->ptr.room->feature;
					case TEXTFEATURE:
					default:
						if (!error_abort("Ungültiger Tabellen-Inhalt (referenz_table)"))
							return Illegal_Feature;
						break;
					}
					feature = feature->next;
				}
				if (!error_abort("Tabelle enthält kein Schlüssel-Wort (referenz_table)"))
					return Illegal_Feature;
				return Nothing;
			}
			entry = entry->next;
		}
		if (!error_abort("Tabelle enthält nicht genug Einträge (referenz_table)"))
			return Illegal_Feature;
	}
	return Nothing;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID *store_nr(_WORD nr)
{
	_VOID *ptr;
	_UBYTE str[N_DIGIT+2];
	
	*str = '#';
	itoa(nr, str+1, 10);
	
	if ((ptr = (_VOID *)heap_store(str, strlen(str)+1)) == NULL)
	{
		abbruch("Nicht genug Speicher für Monster-Liste");
	}
	return ptr;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID *store_ptr(_VOID *ptr)
{
	if ((ptr = heap_store(&ptr, sizeof(ptr))) == NULL)
	{
		abbruch("Nicht genug Speicher für Monster-Liste");
	}
	return ptr;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL make_features(PICE *ptr, TABLE *table)
{
	_WORD roll;
	ENTRY *entry;
	FEATURE *feature;
	_VOID *p;
	
	if (table != NULL)
	{
		roll = dice_roll(table->n, table->dice);
		entry = table->entry;
		while (entry != NULL)
		{
			if (roll <= entry->roll)
			{
				feature = entry->ptr;
				while (feature != NULL)
				{
					switch (feature->type)
					{
					case SUBTABLE:
						if (!make_features(ptr, feature->ptr.subtable))
							return FALSE;
						break;
					case ROOMFEATURE:
						if (ptr->feature == Nothing)
						{
							ptr->feature = feature->ptr.room->feature;
						}
						break;
					case TEXTFEATURE:
						if (ptr->text == NULL)
						{	
							p = store_nr(++Count_nr);
							if (p == NULL)
								return FALSE;
							ptr->text = store_ptr(p);
							if (ptr->text == NULL)
								return FALSE;
						}
						if (store_ptr(feature->ptr.text) == NULL)
							return FALSE;
						break;
					default:
						break;
					}
					feature = feature->next;
				}
				return TRUE;
			}
			entry = entry->next;
		}
		if (!error_abort("Kein passender Tabelleneintrag vorhanden"))
			return FALSE;
	}
	ptr->feature = Nothing;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL FEATURES feature_table(PICE *ptr, TABLE *table)
{
	if (table != NULL)
	{
		if (!make_features(ptr, table))
			return Illegal_Feature;
		if (ptr->text != NULL)
		{
			if (store_ptr(NULL) == NULL)
				return Illegal_Feature;
		}
	} else
	{
		ptr->feature = Nothing;
		ptr->text = NULL;
	}
	return ptr->feature;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL init_table(_VOID)
{
	Count_nr = 0;
	heap_clear();
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL SECTIONS passage_length(_VOID)
{
	SECTIONS value = referenz_table(passage_length_table);

	switch (value)
	{
	case Section0:
	case Section1:
	case Section2:
	case Section3:
	case Illegal_Section:
		return value;
	}
	if (!error_abort("Ungültiger Rückgabewert\n(passage_length)"))
		return Illegal_Section;
	return Section0;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL ENDS passage_end(_VOID)
{
	ENDS value = referenz_table(passage_end_table);
	
	switch (value)
	{
	case T_Junction:
	case Dead_End:
	case Right_Turn:
	case Left_Turn:
	case Stairs_Down:
	case Stairs_Out:
	case Corner:
	case Illegal_End:
		return value;
	case Ok:
		break;
	}
	if (!error_abort("Ungültiger Rückgabewert\n(passage_end)"))
		return Illegal_End;
	return Dead_End;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL FEATURES passage_feature(_VOID)
{
	ENDS value = referenz_table(passage_feature_table);
	
	switch (value)
	{
	case Wandering_Monsters:
	case Nothing:
	case Door1:
	case Door2:
	case Illegal_End:
		return value;
	case Right_Turn:
	case Left_Turn:
	case Stairs_Down:
	case Stairs_Out:
		break;
	}
	if (!error_abort("Ungültiger Rückgabewert\n(passage_features)"))
		return Illegal_Feature;
	return Nothing;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL ROOMS room_type(_VOID)
{
	ROOMS value = referenz_table(room_type_table);

	switch (value)
	{
	case Small:
	case Normal:
	case Hazard:
	case Large:
	case Lair:
	case Quest:
	case Merscha:
	case Invalid:
		return value;
	case Big:
		break;
	}
	if (!error_abort("Ungültiger Rückgabewert\n(room_type)"))
		return Invalid;
	return Normal;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL ROOMDOORS room_doors(_VOID)
{
	ROOMDOORS value = referenz_table(room_doors_table);
	
	switch (value)
	{
	case None:
	case Room_Door1:
	case Room_Door2:
	case Illegal_Room_Doors:
		return value;
	}
	if (!error_abort("Ungültiger Rückgabewert\n(room_doors)"))
		return Illegal_Room_Doors;
	return None;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL ROOMORPASSAGE room_or_passage(_VOID)
{
	ROOMORPASSAGE value = referenz_table(room_or_passage_table);
	
	switch (value)
	{
	case Passage:
	case Room:
	case Illegal_Passage:
		return value;
	}
	if (!error_abort("Ungültiger Rückgabewert\n(room_or_passage)"))
		return Illegal_Passage;
	return Room;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL ROOMDOORS secret_door(_VOID)
{
	ROOMDOORS value = referenz_table(secret_door_table);
	
	switch (value)
	{
	case None:
	case Room_Door1:
	case Illegal_Room_Doors:
		return value;
	case Room_Door2:
		break;
	}
	if (!error_abort("Ungültiger Rückgabewert\n(secret_door)"))
		return Illegal_Room_Doors;
	return None;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL wandering_monsters(PICE *ptr)
{
	if (ptr != NULL)
	{
		switch (feature_table(ptr, wandering_monsters_table))
		{
		case Nothing:
			break;
		case Illegal_Feature:
			return FALSE;
		default:
			ptr->feature = Nothing;
			if (!error_abort("Ungültiger Rückgabewert\n(wandering_monsters)"))
				return FALSE;
			break;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL passage_features(PICE *ptr)
{
	if (ptr != NULL)
	{
		switch (feature_table(ptr, passage_table))
		{
		case Nothing:
			break;
		case Illegal_Feature:
			return FALSE;
		default:
			ptr->feature = Nothing;
			if (!error_abort("Ungültiger Rückgabewert\n(passage_features)"))
				return FALSE;
			break;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL dead_end_features(PICE *ptr)
{
	if (ptr != NULL)
	{
		switch (feature_table(ptr, dead_end_table))
		{
		case Nothing:
			break;
		case Illegal_Feature:
			return FALSE;
		default:
			ptr->feature = Nothing;
			if (!error_abort("Ungültiger Rückgabewert\n(dead_end_features)"))
				return FALSE;
			break;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL corner_features(PICE *ptr)
{
	if (ptr != NULL)
	{
		switch (feature_table(ptr, corner_table))
		{
		case Nothing:
			break;
		case Illegal_Feature:
			return FALSE;
		default:
			ptr->feature = Nothing;
			if (!error_abort("Ungültiger Rückgabewert\n(corner_features)"))
				return FALSE;
			break;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL t_junction_features(PICE *ptr)
{
	if (ptr != NULL)
	{
		switch (feature_table(ptr, t_junction_table))
		{
		case Nothing:
			break;
		case Illegal_Feature:
			return FALSE;
		default:
			ptr->feature = Nothing;
			if (!error_abort("Ungültiger Rückgabewert\n(t_junction_features)"))
				return FALSE;
			break;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL left_turn_features(PICE *ptr)
{
	if (ptr != NULL)
	{
		switch (feature_table(ptr, left_turn_table))
		{
		case Nothing:
			break;
		case Illegal_Feature:
			return FALSE;
		default:
			ptr->feature = Nothing;
			if (!error_abort("Ungültiger Rückgabewert\n(left_turn_features)"))
				return FALSE;
			break;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL right_turn_features(PICE *ptr)
{
	if (ptr != NULL)
	{
		switch (feature_table(ptr, right_turn_table))
		{
		case Nothing:
			break;
		case Illegal_Feature:
			return FALSE;
		default:
			ptr->feature = Nothing;
			if (!error_abort("Ungültiger Rückgabewert\n(right_turn_features)"))
				return FALSE;
			break;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL stairs_out_features(PICE *ptr)
{
	if (ptr != NULL)
	{
		switch (feature_table(ptr, stairs_out_table))
		{
		case Nothing:
			break;
		case Illegal_Feature:
			return FALSE;
		default:
			ptr->feature = Nothing;
			if (!error_abort("Ungültiger Rückgabewert\n(stairs_out_features)"))
				return FALSE;
			break;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL stairs_down_features(PICE *ptr)
{
	if (ptr != NULL)
	{
		switch (feature_table(ptr, stairs_down_table))
		{
		case Nothing:
			break;
		case Illegal_Feature:
			return FALSE;
		default:
			ptr->feature = Nothing;
			if (!error_abort("Ungültiger Rückgabewert\n(stairs_down_features)"))
				return FALSE;
			break;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL small_features(PICE *ptr)
{
	if (ptr != NULL)
	{
		switch (feature_table(ptr, small_table))
		{
		case Nothing:
		case Wandering_Monsters:
		case Maiden:
		case Witch:
		case Man_at_Arms:
		case Rogue:
		case Chasm:
		case Statue:
		case Rats:
		case Bats:
		case Mould:
		case Mushrooms:
		case Grate:
		case Pool:
		case Magic_Circle:
		case Trapdoor:
		case Throne:
		case Rockfall:
		case Wight:
		case Slime:
		case Cess_Pit:
		case Bridge:
		case Apparition:
		case Well:
		case Moths:
		case Chest:
		case Stairs_and_Chest:
		case Tomb:
		case Weapons_Rack:
		case Cupboard:
		case Table:
		case Fireplace:
		case Bookcase:
		case Rack:
			break;
		case Illegal_Feature:
			return FALSE;
		default:
			ptr->feature = Nothing;
			if (!error_abort("Ungültiger Rückgabewert\n(small_features)"))
				return FALSE;
			break;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL normal_features(PICE *ptr)
{
	if (ptr != NULL)
	{
		switch (feature_table(ptr, normal_table))
		{
		case Nothing:
		case Wandering_Monsters:
		case Maiden:
		case Witch:
		case Man_at_Arms:
		case Rogue:
		case Chasm:
		case Statue:
		case Rats:
		case Bats:
		case Mould:
		case Mushrooms:
		case Grate:
		case Pool:
		case Magic_Circle:
		case Trapdoor:
		case Throne:
		case Rockfall:
		case Wight:
		case Slime:
		case Cess_Pit:
		case Bridge:
		case Apparition:
		case Well:
		case Moths:
		case Chest:
		case Stairs_and_Chest:
		case Tomb:
		case Weapons_Rack:
		case Cupboard:
		case Table:
		case Fireplace:
		case Bookcase:
		case Rack:
			break;
		case Illegal_Feature:
			return FALSE;
		default:
			ptr->feature = Nothing;
			if (!error_abort("Ungültiger Rückgabewert\n(normal_features)"))
				return FALSE;
			break;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL hazard_features(PICE *ptr)
{
	if (ptr != NULL)
	{
		switch (feature_table(ptr, hazard_table))
		{
		case Nothing:
		case Wandering_Monsters:
		case Maiden:
		case Witch:
		case Man_at_Arms:
		case Rogue:
		case Chasm:
		case Statue:
		case Rats:
		case Bats:
		case Mould:
		case Mushrooms:
		case Grate:
		case Pool:
		case Magic_Circle:
		case Trapdoor:
		case Throne:
		case Rockfall:
		case Wight:
		case Slime:
		case Cess_Pit:
		case Bridge:
		case Apparition:
		case Well:
		case Moths:
		case Chest:
		case Stairs_and_Chest:
		case Tomb:
		case Weapons_Rack:
		case Cupboard:
		case Table:
		case Fireplace:
		case Bookcase:
		case Rack:
			break;
		case Illegal_Feature:
			return FALSE;
		default:
			ptr->feature = Nothing;
			if (!error_abort("Ungültiger Rückgabewert\n(hazard_features)"))
				return FALSE;
			break;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL large_features(PICE *ptr)
{
	if (ptr != NULL)
	{
		switch (feature_table(ptr, large_table))
		{
		case Nothing:
		case Chest:
		case Stairs_and_Chest:
		case Tomb:
			break;
		case Illegal_Feature:
			return FALSE;
		default:
			ptr->feature = Nothing;
			if (!error_abort("Ungültiger Rückgabewert\n(large_features)"))
				return FALSE;
			break;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL lair_features(PICE *ptr)
{
	if (ptr != NULL)
	{
		switch (feature_table(ptr, lair_table))
		{
		case Nothing:
		case Chest:
		case Stairs_and_Chest:
		case Tomb:
			break;
		case Illegal_Feature:
			return FALSE;
		default:
			ptr->feature = Nothing;
			if (!error_abort("Ungültiger Rückgabewert\n(lair_features)"))
				return FALSE;
			break;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL quest_features(PICE *ptr)
{
	FEATURES feature;
	
	if (ptr != NULL)
	{	
		switch (feature = feature_table(ptr, quest_table))
		{
		case Nothing:
		case Chest:
		case Stairs_and_Chest:
		case Tomb:
			break;
		case Illegal_Feature:
			return FALSE;
		default:
			ptr->feature = Nothing;
			if (!error_abort("Ungültiger Rückgabewert\n(quest_features)(%d)", feature))
				return FALSE;
			break;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL big_features(PICE *ptr)
{
	if (ptr != NULL)
	{
		switch (feature_table(ptr, big_table))
		{
		case Nothing:
			break;
		case Illegal_Feature:
			return FALSE;
		default:
			ptr->feature = Nothing;
			if (!error_abort("Ungültiger Rückgabewert\n(big_features)"))
				return FALSE;
			break;
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL merscha_features(PICE *ptr)
{
	if (ptr != NULL)
	{
		switch (feature_table(ptr, merscha_table))
		{
		case Nothing:
			break;
		case Illegal_Feature:
			return FALSE;
		default:
			ptr->feature = Nothing;
			if (!error_abort("Ungültiger Rückgabewert\n(merscha_features)"))
				return FALSE;
			break;
		}
	}
	return TRUE;
}
