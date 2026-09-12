#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <path.h>
#include <file_io.h>
#include <w_draw.h>
#include <w_dialog.h>
#include <w_mouse.h>
#include <boxf.h>
#include <routine.h>
#include <profile.h>
#include <debug.h>

#include <menu.rh>

#include <defs.h>
#include <makemap.h>
#include <random.h>
#include <icon.h>
#include <wind.h>
#include <table.h>
#include <mem.h>
#include <stairs.h>
#include <map.h>
#include <demo.h>

#define MIN_X 16
#define MIN_Y 16
#define MIN_PICE 25
#define MIN_MEM	10

#define DEF_MAXX	50
#define DEF_MAXY	50
#define DEF_MAXPICE 1000
#define DEF_MAXMEM	50

#define MM 23
#define TO_MM(x) ((_WORD)(((_LONG)(x) * MM) / 10))

struct ahq_para
{
	_WORD x, y;
	DIRECTION eingang;
	_WORD von_x, von_y, rnd;
	_BOOL treppe, nureine, weiter;
	_UBYTE name[60];
	_BOOL grafik, text, monster, statistik;
	_BOOL save_grafik, save_text, save_monster, save_statistik;
	_BOOL compress;
	_WORD zoom;
	IMG_TYPE type;
	PATH karte;
	PATH tabelle;
	PATH fensterdatei;
	PATH zeigdatei;
	PATH editor;
	PATH editpath;
	_WORD max_x;
	_WORD max_y;
	_WORD max_pice;
	_WORD max_mem;
	_BOOL autosave;
	_BOOL ask_exit;
	_BOOL fullscreen;			/* open maximized, ignoring the saved window rects */
};


LOCAL struct ahq_para AHQ_para;

LOCAL CONST _UBYTE *CONST Grafik_Format[] = { "IMG", "PCX", "BMP" };
LOCAL CONST _UBYTE *CONST Eingang[] = { "Norden", "Süden", "Osten", "Westen" };

GLOBAL _UBYTE ProgramName[] = "HQ-Map";

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

#if DEMO
LOCAL _VOID do_demo(_VOID)
{
	Dialog_Select(FDEMO, FUNK_NULL, NULL);
}
#endif /* DEMO */

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID size_error(_WORD imin, _WORD imax)
{
	_UBYTE str[256];
	
	if (imin < 0)
	{
		if (imax < 0)
		{
			sprintf(str, "Value out of range!");
		} else
		{
			sprintf(str, "Value out of range!\n(max. %d)", imax);
		}
	} else
	{
		if (imax < 0)
		{
			sprintf(str, "Value out of range!\n(min. %d)", imin);
		} else
		{
			sprintf(str, "Value out of range!\n(min. %d, max. %d)", imin, imax);
		}
	}
	abbruch(str);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL error_abort(CONST char *str, ...)
{
	_BOOL weiter;
	_UBYTE buf[1024];
	va_list args;
	
	va_start(args, str);
	vsprintf(buf, str, args);
	va_end(args);
	weiter = weiter_abbruch(buf);
	return weiter;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID Profile_ReadPath(PATH *p, CONST _UBYTE *section)
{
	Profile_ReadString(section, "path", p->path, PtrSize(p->path));
	Profile_ReadString(section, "filename", p->filename, PtrSize(p->filename));
	strBcpy(p->pathname, p->path);
	F_Path_Append(p->pathname, p->filename);
	Profile_ReadString(section, "select", p->select, PtrSize(p->select));
	Profile_ReadString(section, "text", p->text, PtrSize(p->text));
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID read_profile(_VOID)
{
	_WORD w;
	
	Profile_UseFile("ahq_map", "JAlbuschies");
	
	Profile_ReadInt("Map", "X", DEF_MAXX, &AHQ_para.x);
	Profile_ReadInt("Map", "Y", DEF_MAXY, &AHQ_para.y);
	Profile_ReadInt("Map", "Eingang", South, &w);
	AHQ_para.eingang = (DIRECTION)w;
	Profile_ReadInt("Map", "VonX", AHQ_para.x / 2, &AHQ_para.von_x);
	Profile_ReadInt("Map", "VonY", 0, &AHQ_para.von_y);
	Profile_ReadInt("Map", "Seed", 0, &AHQ_para.rnd);
	Profile_ReadBool("Map", "Treppe", FALSE, &AHQ_para.treppe);
	Profile_ReadBool("Map", "NurEine", FALSE, &AHQ_para.nureine);
	Profile_ReadBool("Map", "Weiter", FALSE, &AHQ_para.weiter);
	Profile_ReadString("Map", "Name", "", PtrSize(AHQ_para.name));
	Profile_ReadBool("Show", "Grafik", TRUE, &AHQ_para.grafik);
	Profile_ReadBool("Show", "Text", FALSE, &AHQ_para.text);
	Profile_ReadBool("Show", "Statistik", FALSE, &AHQ_para.statistik);
	Profile_ReadBool("Show", "Monster", FALSE, &AHQ_para.monster);
	Profile_ReadBool("Save", "Grafik", TRUE, &AHQ_para.save_grafik);
	Profile_ReadBool("Save", "Text", FALSE, &AHQ_para.save_text);
	Profile_ReadBool("Save", "Statistik", FALSE, &AHQ_para.save_statistik);
	Profile_ReadBool("Save", "Monster", FALSE, &AHQ_para.save_monster);
	Profile_ReadBool("Grafik", "Compress", TRUE, &AHQ_para.compress);
	Profile_ReadInt("Grafik", "Zoom", 1, &AHQ_para.zoom);
	Profile_ReadInt("Grafik", "Type", IMG, &w);
	AHQ_para.type = (IMG_TYPE)w;
	Profile_ReadInt("Params", "MaxX", DEF_MAXX, &AHQ_para.max_x);
	Profile_ReadInt("Params", "MaxY", DEF_MAXY, &AHQ_para.max_y);
	Profile_ReadInt("Params", "MaxPice", DEF_MAXPICE, &AHQ_para.max_pice);
	Profile_ReadInt("Params", "MaxMem", DEF_MAXX, &AHQ_para.max_mem);
	init_path(&AHQ_para.karte, "*.*", "Save map");
	init_path(&AHQ_para.tabelle, "*.tab", "Load reference table");
	init_path(&AHQ_para.fensterdatei, "*.*", "Save window contents");
	init_path(&AHQ_para.zeigdatei, "*.*", "Show file");
	init_path(&AHQ_para.editor, "*.exe", "Set editor path");
	init_path(&AHQ_para.editpath, "*.txt", "Edit file");
	Profile_ReadPath(&AHQ_para.karte, "Karte");
	Profile_ReadPath(&AHQ_para.tabelle, "Tabelle");
	Profile_ReadPath(&AHQ_para.fensterdatei, "Fenster");
	Profile_ReadPath(&AHQ_para.editor, "Editor");
	Profile_ReadPath(&AHQ_para.editpath, "Editpath");
	Profile_ReadBool("Config", "Autosave", TRUE, &AHQ_para.autosave);
	Profile_ReadBool("Config", "AskExit", TRUE, &AHQ_para.ask_exit);
	Profile_ReadInt("Config", "Zoom", 1, &display_zoom);
	Profile_ReadBool("Config", "Fullscreen", TRUE, &AHQ_para.fullscreen);
	Profile_ReadInt("Print", "Zoom", 0, &print_zoom);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID Profile_WritePath(PATH *p, CONST _UBYTE *section)
{
	Profile_WriteString(section, "path", p->path);
	Profile_WriteString(section, "filename", p->filename);
	Profile_WriteString(section, "select", p->select);
	Profile_WriteString(section, "text", p->text);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID write_profile(_VOID)
{
	Profile_WriteInt("Map", "X", AHQ_para.x);
	Profile_WriteInt("Map", "Y", AHQ_para.y);
	Profile_WriteInt("Map", "Eingang", AHQ_para.eingang);
	Profile_WriteInt("Map", "VonX", AHQ_para.von_x);
	Profile_WriteInt("Map", "VonY", AHQ_para.von_y);
	Profile_WriteInt("Map", "Seed", AHQ_para.rnd);
	Profile_WriteBool("Map", "Treppe", AHQ_para.treppe);
	Profile_WriteBool("Map", "NurEine", AHQ_para.nureine);
	Profile_WriteBool("Map", "Weiter", AHQ_para.weiter);
	Profile_WriteString("Map", "Name", AHQ_para.name);
	Profile_WriteBool("Show", "Grafik", AHQ_para.grafik);
	Profile_WriteBool("Show", "Text", AHQ_para.text);
	Profile_WriteBool("Show", "Statistik", AHQ_para.statistik);
	Profile_WriteBool("Show", "Monster", AHQ_para.monster);
	Profile_WriteBool("Save", "Grafik", AHQ_para.save_grafik);
	Profile_WriteBool("Save", "Text", AHQ_para.save_text);
	Profile_WriteBool("Save", "Statistik", AHQ_para.save_statistik);
	Profile_WriteBool("Save", "Monster", AHQ_para.save_monster);
	Profile_WriteBool("Grafik", "Compress", AHQ_para.compress);
	Profile_WriteInt("Grafik", "Zoom", AHQ_para.zoom);
	Profile_WriteInt("Grafik", "Type", AHQ_para.type);
	Profile_WriteInt("Params", "MaxX", AHQ_para.max_x);
	Profile_WriteInt("Params", "MaxY", AHQ_para.max_y);
	Profile_WriteInt("Params", "MaxPice", AHQ_para.max_pice);
	Profile_WriteInt("Params", "MaxMem", AHQ_para.max_mem);
	Profile_WritePath(&AHQ_para.karte, "Karte");
	Profile_WritePath(&AHQ_para.tabelle, "Tabelle");
	Profile_WritePath(&AHQ_para.fensterdatei, "Fenster");
	Profile_WritePath(&AHQ_para.editor, "Editor");
	Profile_WritePath(&AHQ_para.editpath, "Editpath");
	Profile_WriteBool("Config", "Autosave", AHQ_para.autosave);
	Profile_WriteBool("Config", "AskExit", AHQ_para.ask_exit);
	Profile_WriteInt("Config", "Zoom", display_zoom);
	Profile_WriteBool("Config", "Fullscreen", AHQ_para.fullscreen);
	Profile_WriteInt("Print", "Zoom", print_zoom);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL mem_init(_WORD x, _WORD y, _WORD pice, _WORD mem)
{
	if (!mem_alloc(x, y, pice, mem))
	{
		return FALSE;
	}
	AHQ_para.max_x = x;
	AHQ_para.max_y = y;
	AHQ_para.max_pice = pice;
	AHQ_para.max_mem = mem;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID karte_ok(_BOOL ok)
{
	Wind_Menu_Enable(MWEITER, ok);
	Wind_Menu_Enable(MABSPEIC, ok);
	Wind_Menu_Enable(MGRAFIK, ok);
	Wind_Menu_Enable(MTEXT, ok);
	Wind_Menu_Enable(MMONSTER, ok);
	Wind_Menu_Enable(MSTATIST, ok);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL do_table(PATH *file, _BOOL init)
{
	DIALOG *ptr;
	_BOOL ok;
	PATH old;
	
	if (init && !F_File_Exists(file->pathname))
	{
		abbruch("Reference tables not available!");
		ok = FALSE;
	} else
	{
		close_all_windows(FALSE);
		get_path(&old);
		if (F_Path_Set(file->path) == FALSE)
		{
			abbruch("The directory does not exist!");
			ok = FALSE;
		} else
		{	
			ptr = show_string("Reading reference table");
			ok = read_table(file->filename, "errors.txt");
			hide_string(ptr);
			
			if (ok)
			{
				Wind_Menu_Enable(MKARTE, TRUE);
				if (Titel != NULL)
					strBcpy(AHQ_para.name, Titel->ptr.text);
			} else
			{
				Wind_Menu_Enable(MKARTE, FALSE);
				abbruch("Could not create reference table!");
				if (F_File_Exists("errors.txt"))
				{
					show_text_file("errors.txt");
				}
			}
		}
		F_Path_Set(old.path);
	}
	return ok;
}

/*** ---------------------------------------------------------------------- ***/

typedef struct {
	_WORD x, y, pice, mem;
} PARA;
	
LOCAL DLG_RETURN para_button(DIALOG *dialog, _WORD button, _BOOL *ret, _VOID *_para)
{
	PARA *para;
	
	para = _para;
	switch (button)
	{
	case DO_INIT:
		Dialog_SetInt(dialog, PMAXX, para->x, 3);
		Dialog_SetInt(dialog, PMAXY, para->y, 3);
		Dialog_SetInt(dialog, PXCM, TO_MM(para->x), 4);
		Dialog_SetInt(dialog, PYCM, TO_MM(para->y), 4);
		Dialog_SetInt(dialog, PMAXPICE, para->pice, 5);
		Dialog_SetInt(dialog, PMAXMEM, para->mem, 4);
		Dialog_SetBool(dialog, PASK, AHQ_para.ask_exit);
		Dialog_SetBool(dialog, PAUTOSAVE, AHQ_para.autosave);
		Dialog_SetCurr(dialog, PMAXX);
		break;
	case PABBRUCH:
		*ret = FALSE;
		return DLG_END;
	case PMAXX:
		para->x = Dialog_GetInt(dialog, PMAXX);
		Dialog_SetInt(dialog, PXCM, TO_MM(para->x), 4);
		break;
	case PMAXY:
		para->y = Dialog_GetInt(dialog, PMAXY);
		Dialog_SetInt(dialog, PYCM, TO_MM(para->y), 4);
		break;
	case POK:
		para->x = Dialog_GetInt(dialog, PMAXX);
		if (para->x < MIN_X)
		{
			size_error(MIN_X, -1);
			Dialog_SetCurr(dialog, PMAXX);
			return DLG_CONTINUE;
		}
		para->y = Dialog_GetInt(dialog, PMAXY);
		if (para->y < MIN_Y)
		{
			size_error(MIN_Y, -1);
			Dialog_SetCurr(dialog, PMAXY);
			return DLG_CONTINUE;
		}
		para->pice = Dialog_GetInt(dialog, PMAXPICE);
		if (para->pice < MIN_PICE)
		{
			size_error(MIN_PICE, -1);
			Dialog_SetCurr(dialog, PMAXPICE);
			return DLG_CONTINUE;
		}
		para->mem = Dialog_GetInt(dialog, PMAXMEM);
		if (para->mem < MIN_MEM)
		{
			size_error(MIN_MEM, -1);
			Dialog_SetCurr(dialog, PMAXMEM);
			return DLG_CONTINUE;
		}
		AHQ_para.autosave = Dialog_GetBool(dialog, PAUTOSAVE);
		AHQ_para.ask_exit = Dialog_GetBool(dialog, PASK);
		*ret = TRUE;
		return DLG_END;
	case PDEFAULT:
		Dialog_SetInt(dialog, PMAXX, DEF_MAXX, 3);
		Dialog_SetInt(dialog, PMAXY, DEF_MAXY, 3);
		Dialog_SetInt(dialog, PMAXPICE, DEF_MAXPICE, 5);
		Dialog_SetInt(dialog, PMAXMEM, DEF_MAXMEM, 4);
		Dialog_SetInt(dialog, PXCM, TO_MM(DEF_MAXX), 4);
		Dialog_SetInt(dialog, PYCM, TO_MM(DEF_MAXY), 4);
		return DLG_CONTINUE;
	}
	return DLG_IGNORE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL do_para(_VOID)
{
	PARA para;
	
	for (;;)
	{
		para.x = AHQ_para.max_x;
		para.y = AHQ_para.max_y;
		para.pice = AHQ_para.max_pice;
		para.mem = AHQ_para.max_mem;
		
		if (!Dialog_Select(FPARA, para_button, &para))
		{
			return FALSE;
		}
		
		close_all_windows(FALSE);
		
		if (mem_init(para.x, para.y, para.pice, para.mem))
		{
			return TRUE;
		}
		
		if (!mem_init(AHQ_para.max_x, AHQ_para.max_y, AHQ_para.max_pice,
					  AHQ_para.max_mem)
	   	)
	   	{
	   		if (!mem_init(MIN_X, MIN_Y, MIN_PICE, MIN_MEM))
	   		{
	   			abbruch("Fatal error!\nNot enough memory!");
				exit(-1);
	   		}
	   	}
	}
}

/*** ---------------------------------------------------------------------- ***/

LOCAL DLG_RETURN karte_button(DIALOG *dialog, _WORD button, _BOOL *ret, _VOID *para)
{
	_WORD i, x, y;
	DIRECTION Von;
	
	UNUSED(para);
	
	switch (button)
	{
	case DO_INIT:
		Dialog_SetStr(dialog, KNAME, AHQ_para.name);
		x = min(AHQ_para.x, AHQ_para.max_x);
		y = min(AHQ_para.y, AHQ_para.max_y);
		Dialog_SetInt(dialog, KX, x, 3);
		Dialog_SetInt(dialog, KY, y, 3);
		Dialog_SetInt(dialog, KXCM, TO_MM(x), 4);
		Dialog_SetInt(dialog, KYCM, TO_MM(y), 4);
		Dialog_SetPopup(dialog, KPOS, Eingang, 4, (_WORD)AHQ_para.eingang);
		Dialog_SetInt(dialog, KEX, min(AHQ_para.von_x, x), 3);
		Dialog_SetInt(dialog, KEY, min(AHQ_para.von_y, y), 3);
		Dialog_SetInt(dialog, KZUFALL, AHQ_para.rnd, 5);
		Dialog_SetBool(dialog, KIMMER, AHQ_para.treppe);
		Dialog_SetBool(dialog, KNUREINE, AHQ_para.nureine);
		Dialog_SetBool(dialog, KWEITER, AHQ_para.weiter);
		Dialog_SetCurr(dialog, KNAME);
		return DLG_CONTINUE;
	case KPOS:
		Von = (DIRECTION)Dialog_GetPopup(dialog, KPOS);
		x = Dialog_GetInt(dialog, KX);
		y = Dialog_GetInt(dialog, KY);
		switch (Von)
		{
		case North:
			x /= 2;
			y -= End.h;
			break;
		case South:
			x /= 2;
			y = 0;
			break;
		case East:
			x -= End.w;
			y /= 2;
			break;
		case West:
			x = 0;
			y /= 2;
			break;
		default:
			x /= 2;
			y /= 2;
			break;
		}
		Dialog_SetInt(dialog, KEX, x, 3);
		Dialog_SetInt(dialog, KEY, y, 3);
		return DLG_CONTINUE;
	case KABBR:
		*ret = FALSE;
		return DLG_END;
	case KX:
		x = Dialog_GetInt(dialog, KX);
		Dialog_SetInt(dialog, KXCM, TO_MM(x), 4);
		break;
	case KY:
		y = Dialog_GetInt(dialog, KY);
		Dialog_SetInt(dialog, KYCM, TO_MM(y), 4);
		break;
	case KEX:
		break;
	case KEY:
		break;
	case KNAECHST:
	case KOK:
		x = Dialog_GetInt(dialog, KX);
		if (x < MIN_X || x > MAX_X)
		{
			size_error(MIN_X, MAX_X);
			Dialog_SetCurr(dialog, KX);
			return DLG_CONTINUE;
		}
		y = Dialog_GetInt(dialog, KY);
		if (y < MIN_Y || y > MAX_Y)
		{
			size_error(MIN_Y, MAX_Y);
			Dialog_SetCurr(dialog, KY);
			return DLG_CONTINUE;
		}
		i = Dialog_GetInt(dialog, KEX);
		if (i < 0 || i > x - End.w)
		{
			size_error(0, x - End.w);
			Dialog_SetCurr(dialog, KEX);
			return DLG_CONTINUE;
		}
		i = Dialog_GetInt(dialog, KEY);
		if (i < 0 || i > y - End.h)
		{
			size_error(0, y - End.h);
			Dialog_SetCurr(dialog, KEY);
			return DLG_CONTINUE;
		}

		if (button == KNAECHST)
		{
			Dialog_SetInt(dialog, KZUFALL, Dialog_GetInt(dialog, KZUFALL) + 1, 5);
		}
		
		Dialog_GetStr(dialog, KNAME, PtrSize(AHQ_para.name));
		AHQ_para.x = Dialog_GetInt(dialog, KX);
		AHQ_para.y = Dialog_GetInt(dialog, KY);
		AHQ_para.eingang = (DIRECTION)Dialog_GetPopup(dialog, KPOS);
		AHQ_para.von_x = Dialog_GetInt(dialog, KEX);
		AHQ_para.von_y = Dialog_GetInt(dialog, KEY);
		AHQ_para.rnd = Dialog_GetInt(dialog, KZUFALL);
		if (AHQ_para.rnd == 0)
			AHQ_para.rnd = (_UWORD)(clock() & 0x7FFF);
		new_rand(AHQ_para.rnd);
		AHQ_para.treppe = Dialog_GetBool(dialog, KIMMER);
		AHQ_para.nureine = Dialog_GetBool(dialog, KNUREINE);
		AHQ_para.weiter = Dialog_GetBool(dialog, KWEITER);
		*ret = TRUE;
		return DLG_END;
	}
	return DLG_IGNORE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL do_karte(_VOID)
{
	if (!Dialog_Select(FKARTE, karte_button, NULL))
	{
		return FALSE;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL DLG_RETURN speicher_button(DIALOG *dialog, _WORD button, _BOOL *ret, _VOID *para)
{
	_WORD i;

	UNUSED(para);

	switch (button)
	{
	case DO_INIT:
		Dialog_SetStr(dialog, SNAME, AHQ_para.name);
		Dialog_SetInt(dialog, SZOOM, AHQ_para.zoom, 0);
		Dialog_SetPopup(dialog, SGTYPE, Grafik_Format, 3, (_WORD)AHQ_para.type);
		Dialog_SetBool(dialog, SKOMP, AHQ_para.compress);
		Dialog_SetBool(dialog, SGRAFIK, AHQ_para.save_grafik);
		Dialog_SetBool(dialog, STEXTK, AHQ_para.save_text);
		Dialog_SetBool(dialog, SMONSTER, AHQ_para.save_monster);
		Dialog_SetBool(dialog, SSTATIST, AHQ_para.save_statistik);
		Dialog_Enable(dialog, SGRAFIK, Grafik_Karte != NULL);
		Dialog_Enable(dialog, STEXTK, Text_Karte != NULL);
		Dialog_Enable(dialog, SMONSTER, Monster_Liste != NULL);
		Dialog_Enable(dialog, SSTATIST, Statistik != NULL);
		Dialog_SetCurr(dialog, SGRAFIK);
		return DLG_CONTINUE;
	case SABBR:
		*ret = FALSE;
		return DLG_END;
	case SOK:
		if (!get_filename(&AHQ_para.karte))
			return DLG_CONTINUE;
		i = Dialog_GetInt(dialog, SZOOM);
		if (i < 1 || i > 99)
		{
			size_error(1, 99);
			Dialog_SetCurr(dialog, SZOOM);
			return DLG_CONTINUE;
		}
		AHQ_para.zoom = i;
		AHQ_para.type = (IMG_TYPE)Dialog_GetPopup(dialog, SGTYPE);
		AHQ_para.compress = Dialog_GetBool(dialog, SKOMP);
		AHQ_para.save_grafik = Dialog_GetBool(dialog, SGRAFIK);
		AHQ_para.save_text = Dialog_GetBool(dialog, STEXTK);
		AHQ_para.save_monster = Dialog_GetBool(dialog, SMONSTER);
		AHQ_para.save_statistik = Dialog_GetBool(dialog, SSTATIST);
		*ret = TRUE;
		return DLG_END;
	case SZOOM:
		break;
	}
	return DLG_IGNORE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL do_speichern(_VOID)
{
	if (!Dialog_Select(FSPEICH, speicher_button, NULL))
	{
		return FALSE;
	}
	
#if DEMO
	do_demo();
#else

	if (AHQ_para.save_grafik)
	{
		if (!save_grafic(&AHQ_para.karte, AHQ_para.compress, AHQ_para.zoom, AHQ_para.type))
			return FALSE;
	}
	
	if (AHQ_para.save_text)
	{
		if (!save_text(&AHQ_para.karte))
			return FALSE;
	}
	
	if (AHQ_para.save_monster)
	{
		if (!save_liste(&AHQ_para.karte))
			return FALSE;
	}

	if (AHQ_para.save_statistik && Statistik != NULL)
	{
		if (!save_statistik(&AHQ_para.karte))
			return FALSE;
	}

#endif /* DEMO */
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _WORD try_makemap(_BOOL weiter)
{
	_BOOL ret;
	_UBYTE str[256];
	DIALOG *ptr;

	new_rand(AHQ_para.rnd);
	if (weiter)
		sprintf(str, "Generating map (#%u), press a key to cancel", AHQ_para.rnd);
	else
		sprintf(str, "Generating map (#%u)", AHQ_para.rnd);
	ptr = show_string(str);
	ret = makemap(AHQ_para.x, AHQ_para.y, AHQ_para.von_x, AHQ_para.von_y, AHQ_para.eingang);
	hide_string(ptr);

	if (ret == FALSE)
		return FALSE;

	if (AHQ_para.nureine && delete_stairs())
		return TRUE;
	
	switch (make_stairs(AHQ_para.treppe))
	{
	case FALSE:
		return FALSE;
	case TRUE:
		return TRUE;
	}
	
	return RETRY;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL do_makemap(_VOID)
{
	_WORD button;

	close_all_windows(FALSE);
	
	for (;;)
	{
		switch (try_makemap(AHQ_para.weiter))
		{
			case FALSE: return FALSE;
			case TRUE: return TRUE;
		}
		
		if (AHQ_para.weiter)
		{
			if (W_Keypressed(NULL))
			{
				abbruch("Kartengenerierung abgebrochen!\nEs gibt keine Treppe abwärts!");
				return FALSE;
			}
		} else
		{
			button = Form_Alert(1, _T("[1][Es gibt keine Treppe abwärts!][.[Ok|[Nächste|:[Abbruch]"));
			switch (button)
			{
			case 1:
				return TRUE;
			case 2:
				break;
			default:
				return FALSE;
			}
		}
		++AHQ_para.rnd;
	}
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID do_show(_WORD object)
{
	switch (object)
	{
	case MGRAFIK:
		if (Wind_Menu_Checked(MGRAFIK) && 
			Wind_Menu_Enabled(MGRAFIK))
		{
			if (Grafik_Karte == NULL)
				show_grafic(AHQ_para.name);
			else
				Wind_On_Top(Grafik_Karte);
		}
		break;
	case MTEXT:
		if (Wind_Menu_Checked(MTEXT) &&
			Wind_Menu_Enabled(MTEXT))
		{
			if (Text_Karte == NULL)
				show_text(AHQ_para.name);
			else
				Wind_On_Top(Text_Karte);
		}
		break;
	case MMONSTER:
		if (Wind_Menu_Checked(MMONSTER) &&
			Wind_Menu_Enabled(MMONSTER))
		{
			if (Monster_Liste == NULL)
				show_liste(AHQ_para.name);
			else
				Wind_On_Top(Monster_Liste);
		}
		break;
	case MSTATIST:
		if (Wind_Menu_Checked(MSTATIST) &&
			Wind_Menu_Enabled(MSTATIST))
		{
			if (Statistik == NULL)
				show_statistik(AHQ_para.name);
			else
				Wind_On_Top(Statistik);
		}
		break;
	}
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL test_editor(_BOOL warn)
{
	if (F_File_Exists(AHQ_para.editor.pathname))
	{
		Wind_Menu_Enable(MEDITOR, TRUE);
		return TRUE;
	}
	Wind_Menu_Enable(MEDITOR, FALSE);
	if (warn)
		abbruch("Editor not found!");
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL wind_out(PATH *p)
{
	WINDOW_DEF *wr;
	
	if ((wr = Wind_Top()) != NULL)
	{
		if (wr == Grafik_Karte)
			return save_grafic(p, AHQ_para.compress, AHQ_para.zoom, AHQ_para.type);
		if (wr == Text_Karte)
			return save_text(p);
		if (wr == Monster_Liste)
			return save_liste(p);
		if (wr == Statistik)
			return save_statistik(p);
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL wind_print(_VOID)
{
	WINDOW_DEF *wr;
	
	if ((wr = Wind_Top()) != NULL)
	{
		if (wr == Grafik_Karte)
			return print_grafic(AHQ_para.zoom);
		if (wr == Text_Karte)
			return print_text();
		if (wr == Monster_Liste)
			return print_liste();
		if (wr == Statistik)
			return print_statistik();
		return print_file(wr);
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL do_menu(_WORD eintrag)
{
	_BOOL retV = TRUE;

	switch (eintrag)
	{
	case MINFO:
		Dialog_Select(FINFO, FUNK_NULL, NULL);
		break;
	
	case MSPEICH:
		if (Wind_Top() != NULL &&
			get_filename(&AHQ_para.fensterdatei))
		{
			wind_out(&AHQ_para.fensterdatei);
		}
		break;

	case MDRUCK:
		wind_print();
		break;

	case MWECHSEL:
		Wind_Next_Top();
		break;
	
	case MSCHLIES:
		Wind_Close(Wind_Top());
		break;

	case MALLESCH:
		close_all_windows(FALSE);
		break;

	case MENDE:
		retV = FALSE;
		break;

	case MKARTE:
		if (do_karte())
		{
			SetMouse(MOUSE_BUSY);
			if (do_makemap())
			{	
				karte_ok(TRUE);
				do_show(MSTATIST);
				do_show(MMONSTER);
				do_show(MTEXT);
				do_show(MGRAFIK);
#if DEMO					
				do_demo();
#endif /* DEMO */
			} else
			{
				karte_ok(FALSE);
			}
			SetMouse(MOUSE_RESTORE);
		}
		break;

	case MWEITER:
		++AHQ_para.rnd;
		SetMouse(MOUSE_BUSY);
		if (do_makemap())
		{	
			karte_ok(TRUE);
			do_show(MSTATIST);
			do_show(MMONSTER);
			do_show(MTEXT);
			do_show(MGRAFIK);
#if DEMO					
			do_demo();
#endif /* DEMO */
		} else
		{
			karte_ok(FALSE);
		}
		SetMouse(MOUSE_RESTORE);
		break;

	case MABSPEIC:
		do_speichern();
		break;

	case MGRAFIK:
	case MTEXT:
	case MMONSTER:
	case MSTATIST:
		if (Wind_Menu_Checked(eintrag))
		{
			Wind_Menu_Check(eintrag, FALSE);
		} else
		{
			Wind_Menu_Check(eintrag, TRUE);
			do_show(eintrag);
		}
		break;

	case MTABELLE:
		if (get_filename(&AHQ_para.tabelle))
		{
			do_table(&AHQ_para.tabelle, FALSE);
			break;
		}
		break;

	case MPARA:
		if (!Wind_Menu_Enabled(MKARTE) || /* noch keine Tabelle gelesen */
			!Wind_Menu_Enabled(MGRAFIK) || /* noch keine Karte erzeugt */
			ok_abbruch("Durch verändern der Programmparameter\nwird die aktuelle Karte gelöscht!")
		   )
		{
			if (do_para())
			{
				do_table(&AHQ_para.tabelle, FALSE);
				karte_ok(FALSE);
			}
		}
		break;

	case MEPFAD:
		if (get_filename(&AHQ_para.editor))
		{
			test_editor(TRUE);
		}
		break;

	case MEDITOR:
		if (test_editor(TRUE))
		{
			if (get_filename(&AHQ_para.editpath))
			{
				Wind_Pexec(AHQ_para.editor.pathname, AHQ_para.editpath.pathname);
			}
		}
		break;
	
	case MSAVEOPT:
		write_profile();
		break;
	}
	return retV;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL size_t WindPos_Read_Write(_BOOL save, WIND_RESTORE *buf, size_t numElems)
{
	size_t retV;
	size_t i;
	_UBYTE buf1[20];
	_UBYTE buf2[80];
	int group, reOpen;
	long var_bez;
	int x, y, w, h;
	
	if (save)
	{
		Profile_DeleteSection("Windows");
		for (i = 0; i < numElems; i++)
		{
			sprintf(buf1, "%d", (int) i);
			sprintf(buf2, "%d, %d, {%d, %d, %d, %d}, %ld",
				buf[i].group,
				buf[i].reOpen,
				buf[i].pos.g_x,
				buf[i].pos.g_y,
				buf[i].pos.g_w,
				buf[i].pos.g_h,
				buf[i].var_bez);
			Profile_WriteString("Windows", buf1, buf2);
		}
		retV = numElems;
	} else if (buf == NULL)
	{
		for (i = 0; ; i++)
		{
			sprintf(buf1, "%d", (int) i);
			if (!Profile_ReadString("Windows", buf1, "", PtrSize(buf2)) ||
				buf2[0] == '\0')
				break;
		}
		retV = i;
	} else
	{
		for (i = 0; i < numElems; i++)
		{
			sprintf(buf1, "%d", (int) i);
			if (!Profile_ReadString("Windows", buf1, "", PtrSize(buf2)) ||
				sscanf(buf2, "%d, %d, {%d, %d, %d, %d}, %ld",
					&group, &reOpen,
					&x, &y, &w, &h,
					&var_bez) != 7)
				break;
			buf[i].group = group;
			buf[i].reOpen = reOpen;
			buf[i].pos.g_x = x;
			buf[i].pos.g_y = y;
			buf[i].pos.g_w = w;
			buf[i].pos.g_h = h;
			buf[i].var_bez = var_bez;
		}
		retV = i;
	}
	return retV;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL WindPos_Save_Restore(_BOOL save, _WORD art, _LONG *var_bez)
{
	UNUSED(var_bez);
	
	switch (art)
	{
	case 0:
		if (save)
		{
			if (AHQ_para.ask_exit && !ok_abbruch("Programm beenden?"))
				return FALSE;
			AHQ_para.grafik = Wind_Menu_Checked(MGRAFIK);
			AHQ_para.text = Wind_Menu_Checked(MTEXT);
			AHQ_para.monster = Wind_Menu_Checked(MMONSTER);
			AHQ_para.statistik = Wind_Menu_Checked(MSTATIST);
		} else
		{
			if (AHQ_para.grafik)
				Wind_Menu_Check(MGRAFIK, TRUE);
			if (AHQ_para.text)
				Wind_Menu_Check(MTEXT, TRUE);
			if (AHQ_para.monster)
				Wind_Menu_Check(MMONSTER, TRUE);
			if (AHQ_para.statistik)
				Wind_Menu_Check(MSTATIST, TRUE);
			test_editor(FALSE);
			karte_ok(FALSE);
			if (do_table(&AHQ_para.tabelle, TRUE))
			{
				SetMouse(MOUSE_BUSY);
				if (try_makemap(FALSE) == TRUE)
				{	
					karte_ok(TRUE);
				}
				SetMouse(MOUSE_RESTORE);
			}
#if DEMO					
			do_demo();
#endif /* DEMO */
		}
		break;
	
	case W_DATEI:
		if (!save)
			show_text_file(AHQ_para.zeigdatei.pathname);
		break;

	case W_STATISTIK:
		if (!save)
			do_show(MSTATIST);
		break;

	case W_LISTE:
		if (!save)
			do_show(MMONSTER);
		break;

	case W_TEXT:
		if (!save)
			do_show(MTEXT);
		break;

	case W_GRAFIK:
		if (!save)
			do_show(MGRAFIK);
		break;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

_WORD WindFormMain(_WORD argc, CONST _UBYTE **argv)
{
	UNUSED(argc);
	UNUSED(argv);
	
	read_profile();
	
	/* must happen before the main window is shown by Wind_Hide_Show_Init */
	Wind_Set_Fullscreen(AHQ_para.fullscreen);
	
	if (init_icons())
	{
		if (!mem_init(AHQ_para.max_x, AHQ_para.max_y, AHQ_para.max_pice, AHQ_para.max_mem))
		{
			if (!mem_init(DEF_MAXX, DEF_MAXY, DEF_MAXPICE, DEF_MAXMEM))
			{
				if (!mem_init(MIN_X, MIN_Y, MIN_PICE, MIN_MEM))
				{
					abbruch("Nicht genügend Speicher!");
					free_icons();
					return -1;
				}
				ok("The minimum memory configuration has been installed!");
			}
			ok("The default memory configuration has been installed!");
		}
	
		if (Wind_Hide_Show_Init(WindPos_Read_Write, WindPos_Save_Restore))
		{
			Evnt_Multi(do_menu, MENUE);
		}
		
		mem_freeall();
	}
	free_icons();
	
	if (AHQ_para.autosave)
		write_profile();
	
	return 0;
}
