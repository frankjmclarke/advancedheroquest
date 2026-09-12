#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <table.h>
#include <features.h>
#include <file_io.h>
#include <mem.h>
#include <routine.h>
#include <boxf.h>
#include <defs.h>

#define MAXLEVEL 8
#define MAXDEEP 100
#define MAXCHARS 1024

#define SPACE_CHARS "\t\r\n "
#define COMMENT		";*"
#define TAB_START	"([{"
#define TAB_END		")]}"
#define ADDCONTENS	"&+/|,:"
#define NAME_CHARS	"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÜabcdefghijklmnopqrstuvwxyzäöüß-_."
#define STRING		"\"'`"
#define DIGITS		"0123456789"

struct table_names
{
	CONST _UBYTE *name;
	TABLE **table;
	FEATURE_PTR **test;
};

RLOCAL struct table_names CONST Referenz_Table_Names[] =
{
	{ "Passage-Length", &passage_length_table, passage_length_ok },
	{ "Passage-End", &passage_end_table, passage_end_ok},						  	 	 
	{ "Passage-Feature", &passage_feature_table, passage_feature_ok },
	{ "Room-Type", &room_type_table, room_type_ok },
	{ "Room-Doors", &room_doors_table, room_doors_ok },
	{ "Room-or-Passage", &room_or_passage_table, room_or_passage_ok },
	{ "Secret-Doors", &secret_door_table, secret_door_ok },
	{ NULL, NULL }
};
	
RLOCAL struct table_names CONST Feature_Table_Names[] =
{
	{ "Passage", &passage_table, passage_ok },
	{ "Dead-End", &dead_end_table, dead_end_ok} , 
	{ "Corner", &corner_table, corner_ok },
	{ "T-Junction", &t_junction_table, t_junction_ok },
	{ "Right-Turn", &right_turn_table, right_turn_ok },
	{ "Left-Turn", &left_turn_table, left_turn_ok },
	{ "Stairs-Down", &stairs_down_table, stairs_down_ok },
	{ "Stairs-Out", &stairs_out_table, stairs_out_ok },
	
	{ "Small", &small_table, small_ok },
	{ "Normal", &normal_table, normal_ok },
	{ "Hazard", &hazard_table, hazard_ok },
	{ "Large", &large_table, large_ok },
	{ "Lair", &lair_table, lair_ok },
	{ "Quest", &quest_table, quest_ok },
	{ "Big", &big_table, big_ok },
	{ "Merscha", &merscha_table, merscha_ok },
	
	{ "Wandering-Monsters", &wandering_monsters_table, wandering_monsters_ok },
	
	{ NULL, NULL }
};

	
LOCAL FILE *Input, *ErrorOut;
LOCAL _ULONG Zeile, Errors;
LOCAL _UBYTE *Datei;
LOCAL _WORD include_level;

LOCAL TABLE *Last_Table;
LOCAL DEFINE *Last_Define;

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL _VOID scan_error(size_t pos, CONST _UBYTE *str, _UBYTE *ptr)
{
	Errors++;
	if (ErrorOut != NULL)
	{
		fprintf(ErrorOut, "%s\n%*s^ '%s' line: %lu  %s\n", str, (int)pos, "", Datei, Zeile, ptr);
	}
}

/*** ---------------------------------------------------------------------- ***/

#if 0
LOCAL _VOID *store_ptr(_VOID *ptr)
{
	if ((ptr = heap_store(&ptr, sizeof(ptr))) == NULL)
	{
		scan_error(0, "", "Aborted, out of memory");
		abbruch("Speicherplatz für Referenz-Tabellen reicht nicht");
	}
	return ptr;
}
#endif

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID *do_store(_VOID *ptr, size_t len)
{
	_VOID *mem;
	
	if ((mem = stack_store(ptr, len)) == NULL)
	{
		scan_error(0, "", "Aborted, out of memory");
		abbruch("Speicherplatz für Referenz-Tabellen reicht nicht");
	}
	return mem;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _UBYTE *store_string(_UBYTE *ptr)
{
	return do_store(ptr, strlen(ptr)+1);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL FEATURE *store_feature(FEATURE *ptr)
{
	return do_store(ptr, sizeof(*ptr));
}

/*** ---------------------------------------------------------------------- ***/

LOCAL ENTRY *store_entry(ENTRY *ptr)
{
	return do_store(ptr, sizeof(*ptr));
}

/*** ---------------------------------------------------------------------- ***/

LOCAL TABLE *store_table(TABLE *ptr)
{
	return do_store(ptr, sizeof(*ptr));
}

/*** ---------------------------------------------------------------------- ***/

LOCAL DEFINE *store_define(DEFINE *ptr)
{
	return do_store(ptr, sizeof(*ptr));
}

/*** ---------------------------------------------------------------------- ***/

LOCAL DEFINE *find_symbol(CONST _UBYTE *name)
{
	DEFINE *ptr = Last_Define;

	if (name != NULL)
	{
		while (ptr != NULL)
		{
			if (ptr->name != NULL)
			{
				if (strcmp(name, ptr->name) == 0)
				{
					return ptr;
				}
			}
			ptr = ptr->next;
		}
	}
	return NULL;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL TABLE *find_table(CONST _UBYTE *name)
{
	TABLE *ptr = Last_Table;

	if (name != NULL)
	{
		while (ptr != NULL)
		{
			if (ptr->name != NULL)
			{
				if (strcmp(name, ptr->name) == 0)
				{
					return ptr;
				}
			}
			ptr = ptr->next;
		}
	}
	return NULL;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL FEATURE_PTR *find_feature(_UBYTE *name)
{
	FEATURE_PTR **ptr;

	if (name != NULL)
	{
		for (ptr = Feature_Table; *ptr != NULL; ptr++)
		{
			if (strcmp(name, (*ptr)->name) == 0)
			{
				return *ptr;
			}
		}
	}
	return NULL;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL TABLE *new_table(TABLE *table, _UBYTE *name)
{ 
	TABLE *ptr;

	if (name != NULL)
	{
		if ((table->name = store_string(name)) == NULL)
		{
			return NULL;
		}
		table->next = Last_Table;
	} else
	{
		table->name = NULL;
		table->next = NULL;
	}

	if ((ptr = store_table(table)) != NULL && table->name != NULL)
	{
		Last_Table = ptr;
	}
	return ptr;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID skip_chr(size_t *pos, _UBYTE *str, _UBYTE *chr)
{
	*pos += strspn(str+(*pos), chr);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL comment(size_t *pos, _UBYTE *str)
{
	skip_chr(pos, str, SPACE_CHARS);
	if (str[*pos] != '\0' && strchr(COMMENT, (_WORD)(str[*pos])) == NULL)
	{
		return FALSE;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL fgetl(char *str, size_t max, FILE *fd)
{
	char *ptr;
	
	if (fd == NULL || fgets(str, max, fd) == NULL)
	{
		*str = '\0';
		return FALSE;
	}
	
	ptr = strpbrk(str, "\r\n");
	if (ptr != NULL)
	{
		*ptr = '\0';
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL next_line(size_t *pos, _UBYTE *str, size_t max)
{
	do
	{
		*str = '\0';
		*pos = 0;
		Zeile++;
		if (!fgetl(str, max, Input))
		{
			return FALSE;
		}
	} while (comment(pos, str));
	
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID skip_end(size_t *pos, _UBYTE *str)
{
	if (!comment(pos, str))
	{
		scan_error(*pos, str, "Überflüssige Zeichen (werden ignoriert)");
		*pos = strlen(str);
	}
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL test_chr(size_t *pos, _UBYTE *str, _UBYTE *chr)
{
	skip_chr(pos, str, SPACE_CHARS);
	if (str[*pos] == '\0' || strchr(chr, str[*pos]) == NULL)
	{
		return FALSE;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _WORD scan_string(size_t *pos, _UBYTE *str, _UBYTE *name, size_t maxlen)
{
	_UBYTE c[2];
	size_t len;
	
	if (comment(pos, str))
	{
		return FALSE;
	}

	c[0] = str[*pos];
	c[1] = '\0';
	(*pos)++;
	
	len = strcspn(str + *pos, c);
	if (len < maxlen)
	{
		strncpy(name, str + *pos, len);
		name[len] = '\0';
	} else
	{
		strncpy(name, str + *pos, maxlen-1);
		name[maxlen-1] = '\0';
		scan_error(*pos + maxlen, str, "Zeichenkette zu lang (wird abgeschnitten)");
	}
	*pos += len;
	if (str[*pos] != c[0])
	{
		scan_error(*pos, str, "Unterminated string");
		return FALSE;
	}
	(*pos)++;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL scan_name(size_t *pos, _UBYTE *str, _UBYTE *name, size_t maxlen)
{
	size_t len;
	
	skip_chr(pos, str, SPACE_CHARS);

	if (isdigit(str[*pos]) || (len = strspn(str + *pos, NAME_CHARS)) == 0)
	{
		return FALSE;
	}
	
	if (len < maxlen)
	{
		strncpy(name, str + *pos, len);
		name[len] = '\0';
	} else
	{
		strncpy(name, str + *pos, maxlen-1);
		name[maxlen-1] = '\0';
		scan_error(*pos + maxlen, str, "Name is too long (will be truncated)");
	}
	*pos += len;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL scan_roll(size_t *pos, _UBYTE *str, ENTRY *entry, _WORD min, _WORD max)
{
	if (comment(pos, str))
	{
		return FALSE;
	}
	
	if ((entry->roll = (_WORD)atoi(str + (*pos))) <= 0)
	{
		scan_error(*pos, str, "Ungültiger Würfelwert");
		return FALSE;
	}
	if (min > entry->roll)
	{
		scan_error(*pos, str, "Würfelwert zu klein");
	}
	if (min < entry->roll && max < entry->roll)
	{
		scan_error(*pos, str, "Würfelwert zu groß");
	}
	
	skip_chr(pos, str, DIGITS);
	
	if (!test_chr(pos, str, "-"))
	{
		scan_error(*pos, str, "Zeichen '-' fehlt");
		return FALSE;
	}
	(*pos)++;
	
	skip_chr(pos, str, SPACE_CHARS);
	if ((entry->roll = (_WORD)atoi(str + (*pos))) <= 0)
	{
		scan_error(*pos, str, "Ungültiger Würfelwert");
		return FALSE;
	}
	if (min > entry->roll)
	{
		scan_error(*pos, str, "Würfelwert zu klein");
		return FALSE;
	}
	if (max < entry->roll)
	{
		scan_error(*pos, str, "Würfelwert zu groß");
		return FALSE;
	}
	
	skip_chr(pos, str, DIGITS);
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL scan_table(size_t *pos, _UBYTE *str, size_t maxstr, TABLE *table);

LOCAL _BOOL scan_feature(size_t *pos, _UBYTE *str, size_t maxstr, FEATURE **last)
{
	FEATURE feature, *ptr;
	TABLE table;
	_UBYTE name[MAXCHARS];

	table.entry = NULL;
	table.n = table.dice = 0;
	
	feature.next = NULL;
	
	do
	{
		if (test_chr(pos, str, ADDCONTENS))
		{
			(*pos)++;
			if (comment(pos, str) && !next_line(pos, str, maxstr))
			{
				scan_error(0, "", "Tabellen-Eintrag nicht vollständig (Datei zu ende)");
				return FALSE;
			}
		}
		
		if (test_chr(pos, str, STRING))
		{
			if (!scan_string(pos, str, name, sizeof(name)) ||
				(feature.ptr.text = store_string(name)) == NULL
			   )
			{
				return FALSE;
			}
			feature.type = TEXTFEATURE;
		} else
		{
			if (test_chr(pos, str, TAB_START))
			{
				if (!scan_table(pos, str, maxstr, &table) ||
					(feature.ptr.subtable = new_table(&table, NULL)) == NULL
				   )
				{
					return FALSE;
				}
				feature.type = SUBTABLE;
			} else
			{
				if (!scan_name(pos, str, name, sizeof(name)))
				{
					scan_error(*pos, str, "Ungültiger Eintrag");
					return FALSE;
				}
				
				if (test_chr(pos, str, TAB_START))
				{
					(*pos)++;
					if (!test_chr(pos, str, TAB_END))
					{
						scan_error(*pos, str, "')' fehlt");
						return FALSE;
					}
					(*pos)++;
					if ((feature.ptr.subtable = find_table(name)) == NULL &&
						(feature.ptr.subtable = new_table(&table, name)) == NULL
					   )
					{
						return FALSE;
					}
					feature.type = SUBTABLE;
				} else
				{
					if ((feature.ptr.room = find_feature(name)) == NULL)
					{
						scan_error(*pos, str, "Unbekannter Schlüssel-Name");
						return FALSE;
					}
					feature.type = ROOMFEATURE;
				}
			}
		}
		
		if ((ptr = store_feature(&feature)) == NULL)
		{
			return FALSE;
		}
		*last = ptr;
		last = &(ptr->next);
		
	} while (!comment(pos, str));
	
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL scan_dice(size_t *pos, _UBYTE *str, TABLE *table)
{
	if (comment(pos, str))
	{
		scan_error(*pos, str, "Würfeltyp fehlt");
		return FALSE;
	}
	
	if ((table->n = (_WORD)atoi(str + *pos)) <= 0)
	{
		scan_error(*pos, str, "Ungültige Würfelanzahl");
		return FALSE;
	}
	skip_chr(pos, str, DIGITS);
	
	if (!test_chr(pos, str, "Dd"))
	{
		scan_error(*pos, str, "'D' fehlt");
		return FALSE;
	}
	(*pos)++;
	
	skip_chr(pos, str, SPACE_CHARS);
	if ((table->dice = (_WORD)atoi(str + *pos)) <= 0)
	{
		scan_error(*pos, str, "Ungültiger Würfeltyp");
		return FALSE;
	}
	skip_chr(pos, str, DIGITS);
	
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL scan_table(size_t *pos, _UBYTE *str, size_t maxstr, TABLE *table)
{
	_WORD min, max;
	ENTRY entry, *ptr, **last;
	
	last = &(table->entry);
	*last = NULL;
	
	if (comment(pos, str) && !next_line(pos, str, maxstr))
	{
		scan_error(0, "", "Missing start of table (end of file)");
		return FALSE;
	}
	
	if (!test_chr(pos, str, TAB_START))
	{
		scan_error(*pos, str, "Ungültiger Tabellen-Anfang");
		return FALSE;
	}
	(*pos)++;

	if (!scan_dice(pos, str, table))
	{
		return FALSE;
	}
	min = table->n;
	max = table->n;
	max *= table->dice;
	skip_end(pos, str);
	
	do
	{
		while (!comment(pos, str))
		{
			if (test_chr(pos, str, TAB_END))
			{
				(*pos)++;
				if (table->entry == NULL)
				{
					scan_error(0, "", "Missing table contents");
					return FALSE;
				}
				if (min <= max)
				{
					scan_error(0, "", "Tabelle nicht vollständig (letzter Würfelwert zu klein)");
					return FALSE;
				}
				return TRUE;
			}
			
			if (!scan_roll(pos, str, &entry, min, max))
			{
				return FALSE;
			}
			min = entry.roll;
			min++;
			
			entry.next = 0;
			if (!scan_feature(pos, str, maxstr, &(entry.ptr)) ||
				(ptr = store_entry(&entry)) == NULL)
			{
				return FALSE;
			}
			
			*last = ptr;
			last = &(ptr->next);
		}
	} while (next_line(pos, str, maxstr));

	scan_error(0, "", "Missing end of table (end of file)");
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL scan_title(size_t *pos, _UBYTE *str)
{
	FEATURE new_titel = { TEXTFEATURE, NULL, { NULL } };
	FEATURE *titel, *new;
		
	if (str[*pos] == '#')
	{
		(*pos)++;
		if ((new_titel.ptr.text = store_string(str+(*pos))) != NULL &&
			(new = store_feature(&new_titel)) != NULL
		   )
		{
			if (Titel == NULL)
			{
				Titel = new;
			} else
			{
				titel = Titel;
				while (titel->next != NULL)
				{
					titel = titel->next;
				}
				titel->next = new;
			}
		}
		return TRUE;
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL cmd_scan(size_t *pos, _UBYTE *str, size_t maxstr)
{
	_UBYTE name[MAXCHARS];
	TABLE table, *ptr;

	if (!scan_title(pos, str))
	{
		if (!scan_name(pos, str, name, sizeof(name)))
		{
			scan_error(*pos, str, "Ungültiger Tabellenname");
		} else
		{	
			skip_end(pos, str);
			if (!scan_table(pos, str, maxstr, &table))
			{
				scan_error(0, name, "Table not created because it is invalid");
			} else
			{
				skip_end(pos, str);
				if ((ptr = find_table(name)) == NULL)
				{
					new_table(&table, name);
				} else
				{
					if (ptr->entry != NULL)
					{
						scan_error(0, name, "Table name already exists");
					} else
					{
						ptr->entry = table.entry;
						ptr->n = table.n;
						ptr->dice = table.dice;
					}
				}
			}
		}
	}
	return TRUE;
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL _BOOL no_else(size_t *pos, _UBYTE *str, size_t maxstr);
LOCAL _BOOL no_endif(size_t *pos, _UBYTE *str, size_t maxstr);
LOCAL _BOOL cmd_include(size_t *pos, _UBYTE *str, size_t maxstr);
LOCAL _BOOL cmd_define(size_t *pos, _UBYTE *str, size_t maxstr);
LOCAL _BOOL cmd_error(size_t *pos, _UBYTE *str, size_t maxstr);
LOCAL _BOOL cmd_if(size_t *pos, _UBYTE *str, size_t maxstr);
LOCAL _BOOL cmd_endif(size_t *pos, _UBYTE *str, size_t maxstr);
LOCAL _BOOL cmd_else(size_t *pos, _UBYTE *str, size_t maxstr);
LOCAL _BOOL cmd_skip_if(size_t *pos, _UBYTE *str, size_t maxstr);
LOCAL _BOOL cmd_skip_else(size_t *pos, _UBYTE *str, size_t maxstr);
LOCAL _BOOL cmd_scan(size_t *pos, _UBYTE *str, size_t maxstr);

typedef struct commands
{
	CONST _UBYTE *name;
	_BOOL (*function)(size_t *pos, _UBYTE *str, size_t maxstr);
} COMMANDS;

LOCAL COMMANDS CONST Do_cmd[] =
{
	{"include", cmd_include },
	{"define", cmd_define },
	{"error", cmd_error },
	{ "if", cmd_if },
	{ "else", no_else },
	{ "endif", no_endif },
	{ NULL, cmd_scan }
};

LOCAL COMMANDS CONST Do_if_true[] =
{
	{ "include", cmd_include },
	{ "define", cmd_define },
	{ "error", cmd_error },
	{ "if", cmd_if },
	{ "else", cmd_skip_else },
	{ "endif", cmd_endif },
	{  NULL, cmd_scan }
};
	
LOCAL COMMANDS CONST Do_if_false[] =
{
	{ "if", cmd_skip_if },
	{ "else", cmd_else },
	{ "endif", cmd_endif },
	{ NULL, NULL }
};
	
LOCAL COMMANDS CONST Do_else_true[] =
{
	{ "include", cmd_include },
	{ "define", cmd_define },
	{ "error", cmd_error },
	{ "if", cmd_if },
	{ "else", no_else },
	{ "endif", cmd_endif },
	{ NULL, cmd_scan }
};	
		
LOCAL COMMANDS CONST Do_skip_if[] =
{
	{ "if", cmd_skip_if },
	{ "else", cmd_skip_else },
	{ "endif", cmd_endif },
	{ NULL, NULL }
};	
	
LOCAL COMMANDS CONST Do_skip_else[] =
{
	{ "if", cmd_skip_if },
	{ "else", no_else },
	{ "endif", cmd_endif },
	{ NULL, NULL }
};	
			
LOCAL _WORD If_Level = 0;

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL command(CONST COMMANDS *cmd, size_t *pos, _UBYTE *str, size_t maxstr)
{
	_UBYTE *ptr = str+(*pos);
	size_t len = strcspn(ptr, SPACE_CHARS);
	
	if (cmd != NULL)
	{	
		while (cmd->name != NULL)
		{
			if (strnCaseCmp(cmd->name, ptr, len) == 0)
			{
				(*pos) += len;
				break;
			}
			cmd++;
		}
		return ((cmd->function != NULL) ? cmd->function(pos, str, maxstr) : TRUE);
	}
	return FALSE;
}


LOCAL CONST COMMANDS *Last_cmd = NULL;

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID do_lines(CONST COMMANDS *cmd)
{
	CONST COMMANDS *old_cmd = Last_cmd;
	size_t pos;
	_UBYTE str[MAXCHARS];
	
	if (cmd == NULL)
	{
		cmd = Last_cmd;
	} else
	{
		Last_cmd = cmd;
	}
	
	while ( next_line(&pos, str, sizeof(str)) &&
	        command(cmd, &pos, str, sizeof(str)) );
	        
	Last_cmd = old_cmd;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL cmd_include(size_t *pos, _UBYTE *str, size_t maxstr)
{
	FILE *old_input;
	_UBYTE *old_datei;
	_ULONG old_zeile;
	
	UNUSED(maxstr);
	if (include_level >= MAXLEVEL)
	{
		scan_error(0, str, "Zu viele 'Include'-Verschachtelungen");
	} else
	{
		include_level++;
		skip_chr(pos, str, SPACE_CHARS);
		if (str[*pos] == '\0')
		{
			scan_error(*pos, str, "Missing include file name");
		} else
		{
			if ((old_datei = strpbrk(str+(*pos), SPACE_CHARS)) != NULL)
			{
				*old_datei = '\0';
			}
			
			old_input = Input;
			old_datei = Datei;
			old_zeile = Zeile;
			if ((Input = fopen(str + (*pos), "r")) == NULL)
			{
				scan_error(*pos, str, "Include-Datei läßt sich nicht öffnen");
			} else
			{
				Datei = str + (*pos);
				Zeile = 0;
				do_lines(NULL);
				fclose(Input);
			}
			Input = old_input;
			Datei = old_datei;
			Zeile = old_zeile;
		}
		include_level--;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL cmd_define(size_t *pos, _UBYTE *str, size_t maxstr)
{
	_UBYTE name[MAXCHARS];
	DEFINE new, *ptr;
	
	UNUSED(maxstr);
	skip_chr(pos, str, SPACE_CHARS);
	if (str[*pos] == '\0')
	{
		scan_error(*pos, str, "Define-Symbol fehlt");
	} else
	{
		if (!scan_name(pos, str, name, sizeof(name)))
		{
			scan_error(*pos, str, "Ungültiger Symbolnname");
		} else
		{
			if (find_symbol(name) != NULL)
			{
				scan_error(*pos, str, "Symbol already exists");
			} else
			{
				if ((new.name = store_string(name)) != NULL)
				{
					new.next = Last_Define;
					if ((ptr = store_define(&new)) != NULL)
					{
						Last_Define = ptr;
					}
				}
			}
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL cmd_error(size_t *pos, _UBYTE *str, size_t maxstr)
{
	UNUSED(maxstr);
	skip_chr(pos, str, SPACE_CHARS);
	scan_error(*pos, str, "User defined error");
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL test_symbol(size_t*pos, _UBYTE *str)
{
	_BOOL true = TRUE;
	_UBYTE name[MAXCHARS];
	
	skip_chr(pos, str, SPACE_CHARS);
	if (str[*pos] != '\0' && (str[*pos] == '!' || str[*pos] == '/'))
	{
		(*pos)++;
		true = FALSE;
	}
	if (str[*pos] == '\0')
	{
		scan_error(*pos, str, "Symbolname fehlt (Bedingung nicht erfüllt)");
		return FALSE;
	}
	if (!scan_name(pos, str, name, sizeof(name)))
	{
		scan_error(*pos, str, "Ungültiger Symbolnname");
		return FALSE;
	}
			
	if (test_chr(pos, str, TAB_START))
	{
		(*pos)++;
		if (!test_chr(pos, str, TAB_END))
		{
			scan_error(*pos, str, "')' fehlt");
		} else
		{
			(*pos)++;
		}
		true = (find_table(name) != NULL) ? true : !true;
	} else
	{
		true = (find_symbol(name) != NULL) ? true : !true;
	}
	
	skip_end(pos, str);
	return true;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL cmd_if(size_t *pos, _UBYTE *str, size_t maxstr)
{
	UNUSED(maxstr);
	if (If_Level >= MAXLEVEL)
	{
		scan_error(*pos, str, "Zuviele 'If'-Verschachtelungen");
	} else
	{
		If_Level++;
		if (test_symbol(pos, str)) 
		{
			do_lines(Do_if_true);
		} else
		{
			do_lines(Do_if_false);
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL cmd_endif(size_t *pos, _UBYTE *str, size_t maxstr)
{
	UNUSED(maxstr);
	skip_end(pos, str);
	if (If_Level > 0)
	{
		If_Level--;	
		return FALSE;
	}
	scan_error(*pos, str, "Interner 'if'-Verschachtelungsfehler");
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL cmd_else(size_t *pos, _UBYTE *str, size_t maxstr)
{
	UNUSED(maxstr);
	skip_end(pos, str);
	do_lines(Do_else_true);
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL cmd_skip_if(size_t *pos, _UBYTE *str, size_t maxstr)
{
	UNUSED(maxstr);
	if (If_Level >= MAXLEVEL)
	{
		scan_error(*pos, str, "Zuviele 'If'-Verschachtelungen");
	} else
	{
		If_Level++;
		do_lines(Do_skip_if);
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL cmd_skip_else(size_t *pos, _UBYTE *str, size_t maxstr)
{
	UNUSED(maxstr);
	skip_end(pos, str);
	do_lines(Do_skip_else);
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL no_endif(size_t *pos, _UBYTE *str, size_t maxstr)
{
	UNUSED(maxstr);
	skip_end(pos, str);
	scan_error(*pos, str, "'endif' without 'if'");
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL no_else(size_t *pos, _UBYTE *str, size_t maxstr)
{
	UNUSED(maxstr);
	skip_end(pos, str);
	scan_error(*pos, str, "'else' without 'if'");
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL deep_test(TABLE *table, _WORD deep)
{
	ENTRY *entry;
	FEATURE *feature;
	
	if (deep >= MAXDEEP)
	{
		return FALSE;
	}

	entry = table->entry;
	
	while (entry != NULL)
	{
		feature = entry->ptr;
		while (feature != NULL)
		{
			switch (feature->type)
			{
			case SUBTABLE:
				if (!deep_test(feature->ptr.subtable, deep + 1))
					return FALSE;
				break;
			case TEXTFEATURE:
				break;
			case ROOMFEATURE:
				break;
			default:
				abbruch("Ungültiger Inhalt (deep_test())");
				return FALSE;
			}
			feature = feature->next;
		}
		entry = entry->next;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL feature_test(TABLE *table, _WORD *n_features)
{
	_WORD old_features;
	_BOOL ret = TRUE;
	
	ENTRY *entry;
	FEATURE *feature;
	
	entry = table->entry;
	
	while (entry != NULL)
	{
		feature = entry->ptr;
		old_features = *n_features;
		
		while (feature != NULL)
		{
			switch (feature->type)
			{
			case SUBTABLE:
				ret = feature_test(feature->ptr.subtable, n_features);
				break;
			case TEXTFEATURE:
				scan_error(0, feature->ptr.text, "Text wird ignoriert");
				break;
			case ROOMFEATURE:
				(*n_features)++;
				if ((*n_features) > 1)
				{
					scan_error(0, feature->ptr.room->name, "Zusätzlicher Schlüssel-Name");
					ret = FALSE;
				}
				break;
			default:
				abbruch("Ungültiger Inhalt (feature_test())");
				return FALSE;
			}
			feature = feature->next;
		}
		*n_features = old_features;
		entry = entry->next;
	}
	return ret;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL feature_valid(TABLE *table, FEATURE_PTR **valid_features)
{
	_BOOL ret = TRUE;
	FEATURE_PTR **ptr;
	ENTRY *entry;
	FEATURE *feature;
	
	entry = table->entry;
	
	while (entry != NULL)
	{
		feature = entry->ptr;
		
		while (feature != NULL)
		{
			switch (feature->type)
			{
			case SUBTABLE:
				if (!feature_valid(feature->ptr.subtable, valid_features))
				{
					ret = FALSE;
				}					
				break;
			case TEXTFEATURE:
				break;
			case ROOMFEATURE:
				ptr = valid_features;
				while (*ptr != NULL && feature->ptr.room != *ptr)
				{
					ptr++;
				}
				
				if (*ptr == NULL)
				{
					scan_error(0, feature->ptr.room->name, "Unzulässiger Schlüssel-Name");
					ret = FALSE;
				}
				break;
			default:
				abbruch("Ungültiger Inhalt (feature_ok())");
				return FALSE;;
			}
			feature = feature->next;
		}
		entry = entry->next;
	}
	return ret;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL init_table_names(_VOID)
{
	CONST struct table_names *ptr;
	_BOOL ret = TRUE;
	_WORD n_features;
	
	for (ptr = Referenz_Table_Names; ptr->name != NULL; ptr++)
	{
		if ((*(ptr->table) = find_table(ptr->name)) == NULL)
		{
			scan_error(0, ptr->name, "Reference table not available");
			ret = FALSE;
		} else
		{
			n_features = 0;
			if (!feature_test(*(ptr->table), &n_features))
			{
				scan_error(0, ptr->name, "Referenz-Tabelle enthält mehrere Schlüssel-Namen pro Eintrag!");
				ret = FALSE;
			}
			if (!feature_valid(*(ptr->table), ptr->test))
			{
				scan_error(0, ptr->name, "Tabelle enthält einen nicht zugelassenen Schlüssel-Namen!");
				ret = FALSE;
			}
		}
	}
	
	for (ptr = Feature_Table_Names; ptr->name != NULL; ptr++)
	{
		if ((*(ptr->table) = find_table(ptr->name)) != NULL)
		{
			if (!feature_valid(*(ptr->table), ptr->test))
			{
				scan_error(0, ptr->name, "Tabelle enthält einen nicht zugelassenen Schlüssel-Namen!");
				ret = FALSE;
			}
		}
	}
	return ret;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL check_tables(_VOID)
{
	TABLE *ptr = Last_Table;
	_BOOL ret = TRUE;
	
	while (ptr != NULL)
	{
		if (ptr->entry == NULL)
		{
			scan_error(0, ptr->name, "Table not available");
			ret = FALSE;
		} else
		{
			if (!deep_test(ptr, 0))
			{
				scan_error(0, ptr->name, "Tabelle enthält wahrscheinlich eine Rekursion!");
				ret = FALSE;
			}
		}
		ptr = ptr->next;
	}
	return ret;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL read_table(_UBYTE *inpath, _UBYTE *outpath)
{
	_BOOL ret;

	Errors = Zeile = 0;
	Datei = inpath;
	include_level = 0;
	
	ret = TRUE;
	if (Datei != NULL && (Input = fopen(Datei, "r")) != NULL)
	{
		if (outpath != NULL)
		{
			ErrorOut = fopen(outpath, "w");
		} else
		{
			ErrorOut = NULL;
		}

		stack_clear();
		If_Level = 0;
		Last_Table = NULL;
		Last_Define = NULL;
		Titel = NULL;
		
		do_lines(Do_cmd);
		if (If_Level != 0)
		{
			scan_error(0, "", "Missing 'endif' (end of file)");
		}
		
		if (!check_tables())
			ret = FALSE;
		if (ret != FALSE)
			ret = init_table_names();

		fclose(Input);
		if (ErrorOut != NULL)
			fclose(ErrorOut);
	}
	
	if (ret != FALSE && Errors == 0)
	{
		if (outpath != NULL)
		{
			F_File_Delete(outpath);
		}
	} else
	{
		ret = FALSE;
	}
	return ret;
}
