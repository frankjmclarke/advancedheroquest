#include <liste.h>
#include <pice.h>
#include <defs.h>
#include <boxf.h>

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL size_t draw_monster_liste(_UBYTE **mem)
{
	_WORD i;
	size_t size = 0;
	_UBYTE **ptr;
	PICE *pice;
	FEATURE *titel;
	
	for (titel = Titel; titel != NULL; titel = titel->next)
	{
		if (mem != NULL)
		{
			*mem = titel->ptr.text;
			if (*mem == NULL)
				*mem = "";
			mem++;
		}
		size++;
	}
	
	pice = Pice;
	for (i = 0; i < MAX_PICE; i++)
	{
		if ((ptr = pice->text) != NULL)
		{
			switch (pice->type)
			{
			case SMALL_ROOM:
			case NORMAL_ROOM:
			case HAZARD:
			case LARGE_ROOM:
			case LAIR:
			case QUEST:
			case BIG:
			case MERSCHA:
			case PASSAGE:
			case DEAD_END:
			case LEFT_TURN:
			case RIGHT_TURN:
			case T_JUNCTION:
			case CORNER:
			case STAIRS_OUT:
			case STAIRS_DOWN:
				do
				{	
					if (mem != NULL)
					{
						*mem = *ptr;
						if (*mem == NULL)
							*mem = "";
						mem++;
					}
					size++;
				} while (*ptr++ != NULL);
				break;
			case DOOR:
			case SECRET:
			case TEST:
			case INVALID:
			case EMPTY:
				break;
			default:
				abbruch("Ungültiger Type\n(draw_monster_liste)");
				return 0;
			}
		}
		pice++;
	}
	return size;
}
