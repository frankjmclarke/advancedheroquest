#include <stairs.h>
#include <pice.h>
#include <rolldice.h>

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL _BOOL delete_stairs(_VOID)
{
	_WORD i;
	PICE *pice, *last = NULL;
	
	for (pice = Pice, i = 0; i < MAX_PICE; i++, pice++)
	{
		switch (pice->type)
		{
		case STAIRS_DOWN:
			if (last == NULL)
			{
				last = pice;
			} else
			{
				if (last->type == STAIRS_DOWN)
				{
					last->type = STAIRS_OUT;
					last = pice;
				} else
				{
					pice->type = STAIRS_OUT;
				}
			}
			break;
		case NORMAL_ROOM:
		case HAZARD:
		case LAIR:
		case QUEST:
			if (pice->feature == Stairs_and_Chest)
			{
				if (last != NULL)
				{
					last->feature = Chest;
				}
				last = pice;
			}
			break;
		default:
			break;
		}
	}

	return last != NULL;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _WORD make_stairs(_BOOL make)
{
	_WORD i;
	PICE *pice, *last = NULL;
	_UBYTE **ptr;
	
	for (pice = Pice, i = 0; i < MAX_PICE; i++, pice++)
	{
		switch (pice->type)
		{
		case STAIRS_DOWN:
			return TRUE;
		case STAIRS_OUT:
			if (last == NULL ||
				last->type == STAIRS_OUT)
			{
				last = pice;
			}
			break;
		case NORMAL_ROOM:
			if (last == NULL ||
				last->type == STAIRS_OUT ||
				last->type == NORMAL_ROOM)
			{
				last = pice;
			}
			if (pice->feature == Stairs_and_Chest)
			{
				return TRUE;
			}
			break;
		case HAZARD:
			if (last == NULL ||
				last->type == STAIRS_OUT ||
				last->type == NORMAL_ROOM ||
				last->type == HAZARD)
			{
				last = pice;
			}
			if (pice->feature == Stairs_and_Chest)
			{
				return TRUE;
			}
			break;
		case LAIR:
			if (last == NULL ||
				last->type == STAIRS_OUT ||
				last->type == NORMAL_ROOM ||
				last->type == HAZARD ||
				last->type == LAIR)
			{
				last = pice;
			}
			if (pice->feature == Stairs_and_Chest)
			{
				return TRUE;
			}
			break;
		case QUEST:
			if (last == NULL ||
				last->type == STAIRS_OUT ||
				last->type == NORMAL_ROOM ||
				last->type == HAZARD ||
				last->type == LAIR ||
				last->type == QUEST)
			{
				last = pice;
			}
			if (pice->feature == Stairs_and_Chest)
			{
				return TRUE;
			}
			break;
		default:
			break;
		}
	}
	
	if (make && last != NULL)
	{
		if (last->type == STAIRS_OUT)
		{
			last->type = STAIRS_DOWN;
		} else
		{
			ptr = last->text;
			last->text = NULL;
			if (!quest_features(last))
				return FALSE;
			if (last->text == NULL)
			{
				last->text = ptr;
			}
			last->feature = Stairs_and_Chest;
		}
		return TRUE;
	}
	
	return RETRY;
}
