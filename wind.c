#include <wind.h>
#include <demo.h>
#include <menu.rh>
#include <boxf.h>
#include <image.h>
#include <pcx.h>
#include <bmp.h>
#include <map.h>
#include <icon.h>
#include <text.h>
#include <liste.h>
#include <defs.h>
#include <ro_mem.h>
#include <file_io.h>
#include <string.h>
#include <w_draw.h>
#include <w_print.h>

GLOBAL WINDOW_DEF *Grafik_Karte = NULL;
GLOBAL WINDOW_DEF *Text_Karte = NULL;
GLOBAL WINDOW_DEF *Monster_Liste = NULL;
GLOBAL WINDOW_DEF *Statistik = NULL;
GLOBAL _WORD display_zoom;
GLOBAL _WORD print_zoom = 0;
GLOBAL _WORD tabstop = 8;

#define WINDOW_ATTRIBUTES (WAT_NAME | WAT_VERSLIDE | WAT_HORSLIDE | WAT_CLOSER | WAT_FULLER | WAT_SIZER | WAT_SMALLER)

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL DLG_RETURN string_button(DIALOG *dialog, _WORD button, _BOOL *ret, _VOID *para)
{
	*ret = FALSE;
	switch (button)
	{
	case DO_INIT:
		Dialog_SetStr(dialog, STEXT, para);
		break;
	}
	return DLG_CONTINUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID hide_string(DIALOG *ptr)
{
	Dialog_Hide(ptr);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL DIALOG *show_string(CONST _UBYTE *str)
{
	return Dialog_Show(FSTRING, string_button, FALSE, str);
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

#if DEMO 
LOCAL _VOID demo(_VOID)
{
	ok("Demo version!\nSaving is not possible!");
}

/*** ---------------------------------------------------------------------- ***/

#else
LOCAL _BOOL writeopen(FILE **fd, _UBYTE *pathname, CONST _UBYTE *mode)
{
	if ((*fd = fopen(pathname, mode)) == NULL)
	{
		nichtangelegt(pathname);
		return FALSE;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID fileclose(FILE *fd)
{
	if (fd != NULL)
	{
		if (fclose(fd) == EOF)
		{
			clearerr(fd);
			fclose(fd);
		}
	}
}
#endif /* DEMO */

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL _BOOL save_grafic(PATH *p, _BOOL compress, _WORD zoom, IMG_TYPE type)
{
	_BOOL retV = TRUE;
	
	if (Grafik_Karte != NULL)
	{
#if DEMO
		UNUSED(compress);
		UNUSED(zoom);
		UNUSED(p);
		UNUSED(type);
		demo();
#else
		DIALOG *ptr;
		FILE *fd;
		CONST _UBYTE *ext;
		_BOOL (*mfdb_to_pic)(FILE *fd, MFDB *mfdb, _BOOL compress, _WORD zoom);
		
		switch (type)
		{
		case IMG:
			ext = "img";
			mfdb_to_pic = mfdb_to_img;
			break;
			
		case PCX:
			ext = "pcx";
			mfdb_to_pic = mfdb_to_pcx;
			break;
		
		case BMP:
			ext = "bmp";
			mfdb_to_pic = mfdb_to_bmp;
			break;
		
		default:
			abbruch("Invalid image format!");
			return FALSE;
		}
		ptr = show_string("Saving graphic map");
		set_ext(p, ext);
		if (writeopen(&fd, p->pathname, "wb"))
		{
			/*
			 * The window holds a colour bitmap, and the writers below are
			 * monochrome (see the "TODO: color formats" in bmp.c). Render the
			 * map again into a 1 bit deep bitmap and save that, so the output
			 * files are byte for byte what they were before colour existed.
			 */
			MFDB *mono = get_mfdb((Xsize + 2) * ICON_SCALE, (Ysize + 2) * ICON_SCALE, 1, NULL);

			if (mono == NULL)
			{
				abbruch("Not enough memory for graphic map");
				retV = FALSE;
			} else
			{
				draw_img_map(mono);
				if (!mfdb_to_pic(fd, mono, compress, zoom))
				{
					schreibfehler();
					retV = FALSE;
				}
				free_mfdb(mono);
			}
			fileclose(fd);
		}
		hide_string(ptr);
#endif /* DEMO */
	}
	return retV;
}

/*** ---------------------------------------------------------------------- ***/

#if !DEMO
LOCAL _BOOL print_grafic_out(MFDB *mfdb, PRINTER *printer)
{
	_WORD w, h;
	
	get_mfdb_info(mfdb, &w, &h, NULL);
	Printer_Bitmap(printer, mfdb, 0, 0, w, h, 0, 0, print_zoom);
	return TRUE;
}
#endif

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL print_grafic(_WORD zoom)
{
	_BOOL retV = TRUE;
	
	if (Grafik_Karte != NULL)
	{
#if DEMO
		UNUSED(zoom);
		demo();
#else
		DIALOG *ptr;
		PRINTER *printer;
		WINDOW_PROC proc;

		UNUSED(zoom); /* TODO */
		ptr = show_string("Printing graphic map");
		if ((printer = Printer_Open("Printing graphic map", TRUE)) != NULL)
		{
			proc = Wind_Proc_Ptr(Grafik_Karte);
			retV = proc(WMY_PRINT, Grafik_Karte, printer);
			Printer_Close(printer);
		} else
		{
			retV = FALSE;
		}
		hide_string(ptr);
#endif /* DEMO */
	}
	return retV;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _WORD Grafik_Zoom_Get(_VOID)
{
	return display_zoom;
}

/*** ---------------------------------------------------------------------- ***/

/*
 * Change the on-screen magnification. Clamped, so a bad profile value cannot
 * overflow the _WORD arithmetic in draw_grafic(). Takes effect immediately:
 * the scroll document is resized and the window repainted, no restart.
 */
GLOBAL _VOID Grafik_Zoom_Set(_WORD zoom)
{
	L_GRECT show;
	MFDB *ptr;
	_WORD w, h;

	if (zoom < ZOOM_MIN)
		zoom = ZOOM_MIN;
	if (zoom > ZOOM_MAX)
		zoom = ZOOM_MAX;
	if (zoom == display_zoom)
		return;
	display_zoom = zoom;

	if (Grafik_Karte != NULL)
	{
		ptr = Wind_Buf_Ptr(Grafik_Karte);
		if (ptr != NULL)
		{
			get_mfdb_info(ptr, &w, &h, NULL);
			Wind_GetDoc(Grafik_Karte, &show);
			show.xx = 0;
			show.yy = 0;
			show.ww = (_LONG)w * zoom;
			show.hh = (_LONG)h * zoom;
			Wind_SetDoc(Grafik_Karte, &show);
		}
		Wind_Redraw(Grafik_Karte);
	}
}

/*** ---------------------------------------------------------------------- ***/

/*
 * Pick the largest whole-number zoom at which the whole map still fits the
 * window. Integer only: the tiles are 1 bit deep, so whole-number scaling is
 * exact pixel replication and stays crisp, where a fractional factor would
 * drop or double rows unevenly.
 */
GLOBAL _VOID Grafik_Zoom_Fit(_VOID)
{
	GRECT work;
	MFDB *ptr;
	_WORD w, h, zx, zy;

	if (Grafik_Karte == NULL)
		return;
	ptr = Wind_Buf_Ptr(Grafik_Karte);
	if (ptr == NULL)
		return;
	get_mfdb_info(ptr, &w, &h, NULL);
	if (w <= 0 || h <= 0)
		return;
	Wind_GetWork(Grafik_Karte, &work);
	zx = (_WORD)(work.g_w / w);
	zy = (_WORD)(work.g_h / h);
	Grafik_Zoom_Set(zx < zy ? zx : zy);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID draw_grafic(WINDOW_DEF *window, MFDB *mfdb, CONST GRECT *area)
{
	L_GRECT show;
	GRECT scroll;
	GRECT gr;
	_WORD dx, dy;
	
	Wind_GetDoc(window, &show);
	Wind_GetScroll(window, &scroll);
	gr.g_x = -(_WORD)show.xx;
	gr.g_y = -(_WORD)show.yy;
	get_mfdb_info(mfdb, &gr.g_w, &gr.g_h, NULL);
	gr.g_w *= display_zoom;
	gr.g_h *= display_zoom;
	if (rc_intersect(area, &gr))
	{
		gr.g_x = (gr.g_x + (_WORD)show.xx) / display_zoom;
		gr.g_y = (gr.g_y + (_WORD)show.yy) / display_zoom;
		gr.g_w = (gr.g_w + display_zoom * 2 - 1) / display_zoom;
		gr.g_h = (gr.g_h + display_zoom * 2 - 1) / display_zoom;
		dx = (area->g_x / display_zoom) * display_zoom;
		dy = (area->g_y / display_zoom) * display_zoom;
		W_Draw_Bitmap(window, mfdb, gr.g_x, gr.g_y, gr.g_w, gr.g_h, dx, dy, display_zoom);
	}
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL grafic_proc(WIND_MESSAGE msg, WINDOW_DEF *window, _VOID *buf)
{
	L_GRECT show;
	_WORD w, h;
	MFDB *ptr;
	
	switch (msg)
	{
	case WMY_OPEN:
		Wind_SetFac(window, 1, 1, 8, 8);
		ptr = buf;
		get_mfdb_info(ptr, &w, &h, NULL);
		show.xx = 0;
		show.yy = 0;
		show.ww = w * display_zoom;
		show.hh = h * display_zoom;
		Wind_SetDoc(window, &show);
		break;
	case WMY_CLOSE:
		Grafik_Karte = NULL;
		ptr = buf;
		free_mfdb(ptr);
		break;
	case WMY_UPDATE:
		ptr = Wind_Buf_Ptr(window);
		draw_grafic(window, ptr, buf);
		break;
	case WMY_OUT:
#if DEMO
		demo();
		break;
#else
		/* geht hier so nicht, braucht compress & zoom zum sichern */
		return FALSE;
#endif /* DEMO */
	case WMY_PRINT:
#if DEMO
		demo();
		break;
#else
		ptr = Wind_Buf_Ptr(window);
		return print_grafic_out(ptr, buf);
#endif /* DEMO */
	default:
		break;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID show_grafic(_UBYTE *name)
{
	_VOID *ptr;
	_UBYTE str[256];
	MFDB *img;

	if (Grafik_Karte == NULL)
	{
		/*
		 * The on-screen map is drawn into a colour bitmap so each piece can be
		 * tinted (see pice_colors() in icon.c). Saving renders a fresh 1 bit
		 * deep copy instead, so the written files are unchanged.
		 *
		 * The original here called GetScreenPlanes(), which does not exist in
		 * this tree; the function is GetNumPlanes(). That is presumably why the
		 * block was left switched off.
		 */
		img = get_mfdb((Xsize + 2) * ICON_SCALE, (Ysize + 2) * ICON_SCALE, GetNumPlanes(), NULL);
		if (img == NULL)
			img = get_mfdb((Xsize + 2) * ICON_SCALE, (Ysize + 2) * ICON_SCALE, 1, NULL);
		if (img != NULL)
		{
			ptr = show_string("Creating graphic map");
			draw_img_map(img);
			hide_string(ptr);
			sprintf(str, "Graphic: %s", name);
			Grafik_Karte = Wind_Open(W_GRAFIK, WINDOW_ATTRIBUTES, grafic_proc, str, img);
		} else
		{
			abbruch("Not enough memory for graphic map");
		}
	}
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

typedef struct {
	_UBYTE *ptr;
	size_t size;
	_WORD fontsize;
	WINDOW_DEF **window;
	size_t zeilen;
	size_t spalten;
	_UBYTE **tab;
} WINDTXT;

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL save_text(PATH *p)
{
	_BOOL retV = TRUE;
	
	if (Text_Karte != NULL)
	{
#if DEMO
		UNUSED(p);
		demo();
#else
		DIALOG *ptr;
		FILE *fd;
		WINDOW_PROC proc;
		
		set_ext(p, "txt");
		ptr = show_string("Saving text map");
		if (writeopen(&fd, p->pathname, "w"))
		{
			proc = Wind_Proc_Ptr(Text_Karte);
			retV = proc(WMY_OUT, Text_Karte, fd);
			fileclose(fd);
		} else
		{
			retV = FALSE;
		}
		hide_string(ptr);
#endif /* DEMO */
	}
	return retV;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL print_text_file(WINDOW_DEF *wr, CONST _UBYTE *title)
{
	_BOOL retV = TRUE;
	
	if (wr != NULL)
	{
#if DEMO
		UNUSED(title);
		demo();
#else
		DIALOG *ptr;
		PRINTER *printer;
		WINDOW_PROC proc;
		
		ptr = show_string(title);
		if ((printer = Printer_Open(title, FALSE)) != NULL)
		{
			proc = Wind_Proc_Ptr(wr);
			retV = proc(WMY_PRINT, wr, printer);
			Printer_Close(printer);
		} else
		{
			retV = FALSE;
		}
		hide_string(ptr);
#endif /* DEMO */
	}
	return retV;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL print_text(_VOID)
{
	return print_text_file(Text_Karte, "Printing text map");
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _UBYTE *next_line_end(_UBYTE *ptr, _UBYTE *end)
{
	while (ptr < end)
	{
		if (*ptr != '\0' && *ptr != '\r' && *ptr != '\n')
		{
			ptr++;
		} else
		{
			if ((ptr + 1) < end && *ptr == '\r' && ptr[1] == '\n')
				ptr += 2;
			else
				ptr++;
			break;
		}
	}
	return ptr;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _UBYTE *next_line(_UBYTE *ptr, _UBYTE *end)
{
	while (ptr < end)
	{
		if (*ptr != '\0' && *ptr != '\r' && *ptr != '\n')
		{
			ptr++;
		} else
		{
			if ((ptr + 1) < end && *ptr == '\r' && ptr[1] == '\n')
				*ptr++ = '\0', *ptr++ = '\0';
			else
				*ptr++ = '\0';
			break;
		}
	}
	return ptr;
}

/*** ---------------------------------------------------------------------- ***/

#if !DEMO
LOCAL _BOOL print_text_out(WINDTXT *buf, FILE *fd, PRINTER *printer)
{
	size_t i;
	_BOOL retV = TRUE;
	
	if (buf != NULL)
	{
		for (i = 0; i < buf->zeilen; i++)
		{
			if (printer != NULL)
			{
				if (Printer_Write(printer, buf->tab[i], strlen(buf->tab[i])) == FALSE ||
					Printer_NewLine(printer) == FALSE)
				{
					retV = FALSE;
					break;
				}
			} else
			{
				if (fputs(buf->tab[i], fd) == EOF ||
					fputc('\n', fd) == EOF)
				{
					retV = FALSE;
					break;
				}
			}
		}
		if (retV != FALSE &&
			fd != NULL &&
			(fflush(fd) == EOF || ferror(fd)))
		{
			retV = FALSE;
		}
	}
	if (retV == FALSE)
		schreibfehler();
	return retV;
}
#endif /* DEMO */

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL tab_expand(_UBYTE **pptr, size_t *plen, size_t tabstop)
{
	_UBYTE *ptr, *dst;
	size_t len, i;
	size_t newlen;
	_BOOL found;
	
	ptr = *pptr;
	newlen = 0;
	len = *plen;
	found = FALSE;
	while (len)
	{
		if (*ptr == '\t')
		{
			newlen += tabstop - (newlen % tabstop);
			found = TRUE;
		} else
		{
			newlen++;
		}
		ptr++;
		len--;
	}
	if (found)
	{
		dst = MALLOC(newlen + 1, "tab_expand");
		if (dst != NULL)
		{
			ptr = *pptr;
			*pptr = dst;
			len = *plen;
			*plen = newlen;
			newlen = 0;
			while (len)
			{
				if (*ptr == '\t')
				{
					i = tabstop - (newlen % tabstop);
					newlen += i;
					while (i--)
						*dst++ = ' ';
					ptr++;
				} else
				{
					newlen++;
					*dst++ = *ptr++;
				}
				len--;
			}
			*dst = '\0';
			return TRUE;
		}
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL make_text(WINDTXT *buf)
{
	size_t i;
	size_t len;
	_UBYTE *ptr;
	_UBYTE *end;
	
	buf->zeilen = 0;
	buf->spalten = 0;
	buf->tab = NULL;
	ptr = buf->ptr;
	end = ptr + buf->size;
	while (ptr < end)
	{
		ptr = next_line_end(ptr, end);
		buf->zeilen++;
	}
	if (buf->zeilen != 0)
	{
		buf->tab = MALLOC(buf->zeilen * sizeof(_UBYTE *), "make_text");
		if (buf->tab == NULL)
		{
			buf->zeilen = 0;
			return FALSE;
		}
		ptr = buf->ptr;
		for (i = 0; i < buf->zeilen; i++)
		{
			buf->tab[i] = ptr;
			len = strlen(ptr);
			if (tab_expand(&ptr, &len, tabstop))
				SFREE(ptr);
			if (buf->spalten < len)
				buf->spalten = len;
			ptr = next_line(buf->tab[i], end);
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID draw_text(WINDOW_DEF *window, WINDTXT *buf, GRECT *area)
{
	L_GRECT show;
	size_t i;
	_WORD w, h;
	size_t start, end;
	GRECT scroll;
	_WORD y;
	_UBYTE *ptr;
	size_t len;
	
	UNUSED(area);
	Wind_GetDoc(window, &show);
	Wind_GetFac(window, &w, &h, NULL, NULL);
	Wind_GetScroll(window, &scroll);
	start = (size_t)(show.yy / h);
	end = start + (scroll.g_h + h) / h;
	y = (_WORD)(start * h - show.yy);
	for (i = start; i < end && i < buf->zeilen; i++)
	{
		ptr = buf->tab[i];
		len = strlen(ptr);
		if (tab_expand(&ptr, &len, tabstop))
		{
			W_Text(window, (_WORD)-show.xx, y, buf->fontsize, ptr, len);
			SFREE(ptr);
		} else
		{
			W_Text(window, (_WORD)-show.xx, y, buf->fontsize, ptr, len);
		}
		y += h;
	}
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL text_proc(WIND_MESSAGE msg, WINDOW_DEF *window, _VOID *buf)
{
	WINDTXT *ptr;
	L_GRECT show;
	_WORD w, h;
	
	switch (msg)
	{
	case WMY_OPEN:
		ptr = buf;
		if (!make_text(ptr))
			return FALSE;
		W_GetFontSize(window, ptr->fontsize, &w, &h);
		Wind_SetFac(window, w, h, 1, 1);
		show.xx = 0;
		show.yy = 0;
		show.ww = ptr->spalten * w;
		show.hh = ptr->zeilen * h;
		Wind_SetDoc(window, &show);
		break;
	case WMY_CLOSE:
		ptr = buf;
		if (ptr->window != NULL)
			*(ptr->window) = NULL;
		if (ptr->ptr != NULL)
			FREE(ptr->ptr, ptr->size);
		if (ptr->tab != NULL)
			FREE(ptr->tab, ptr->zeilen * sizeof(_UBYTE *));
		OFREE(ptr);
		break;
	case WMY_UPDATE:
		ptr = Wind_Buf_Ptr(window);
		draw_text(window, ptr, buf);
		break;
	case WMY_OUT:
#if DEMO
		demo();
		break;
#else
		ptr = Wind_Buf_Ptr(window);
		return print_text_out(ptr, buf, NULL);
#endif /* DEMO */
	case WMY_PRINT:
#if DEMO
		demo();
		break;
#else
		ptr = Wind_Buf_Ptr(window);
		return print_text_out(ptr, NULL, buf);
#endif /* DEMO */
	default:
		break;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID show_text(_UBYTE *name)
{
	DIALOG *ptr;
	WINDTXT *buf;
	_UBYTE str[256];

	if (Text_Karte == NULL)
	{
		buf = NEW(WINDTXT, "show_text: windtxt");
		if (buf != NULL)
		{
			buf->size = sizeof(_UBYTE) * (Xsize + 3) * (Ysize + 2);
			if ((buf->ptr = M0ALLOC(buf->size, "show_txt: ptr")) == NULL)
			{
				OFREE(buf);
				return;
			}
			buf->fontsize = FONT_SIZE_SMALL;
			buf->window = &Text_Karte;
			ptr = show_string("Creating text map");
			draw_text_map(buf->ptr);
			hide_string(ptr);
			
			sprintf(str, "Text: %s", name);
			Text_Karte = Wind_Open(W_TEXT, WINDOW_ATTRIBUTES, text_proc, str, buf);
		}
	}
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL WINDOW_DEF *show_text_file(_UBYTE *filename)
{
	WINDTXT *buf;
	_ULONG size;
	FILE *fd;
	
	if (F_File_Size(filename, NO_FILE, &size) != GERR_OK)
		return NULL;
	if ((size_t) size != size)
		return NULL;
	buf = NEW(WINDTXT, "show_text_file: windtxt");
	if (buf != NULL)
	{
		buf->size = (size_t)size;
		if ((buf->ptr = MALLOC(buf->size, "show_txt: ptr")) == NULL)
		{
			OFREE(buf);
			return NULL;
		}
		fd = fopen(filename, "rb");
		if (fd == NULL)
		{
			FREE(buf->ptr, buf->size);
			OFREE(buf);
			return NULL;
		}
		buf->size = fread(buf->ptr, 1, buf->size, fd);
		fclose(fd);
		buf->fontsize = FONT_SIZE_NORMAL;
		buf->window = NULL;
		
		return Wind_Open(W_DATEI, WINDOW_ATTRIBUTES, text_proc, filename, buf);
	}
	return NULL;
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL _BOOL save_liste(PATH *p)
{
	_BOOL retV = TRUE;
	
	if (Monster_Liste != NULL)
	{
#if DEMO
		UNUSED(p);
		demo();
#else
		DIALOG *ptr;
		FILE *fd;
		WINDOW_PROC proc;
		
		set_ext(p, "mon");
		ptr = show_string("Saving monster list");
		if (writeopen(&fd, p->pathname, "w"))
		{
			proc = Wind_Proc_Ptr(Monster_Liste);
			retV = proc(WMY_OUT, Monster_Liste, fd);
			fileclose(fd);
		} else
		{
			retV = FALSE;
		}
		hide_string(ptr);
#endif /* DEMO */
	}
	return retV;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL print_liste(_VOID)
{
	return print_text_file(Monster_Liste, "Printing monster list");
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL print_file(WINDOW_DEF *wr)
{
	return print_text_file(wr, "Printing file");
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID max_spalten(WINDTXT *buf)
{
	size_t i;
	size_t len;
	_UBYTE *ptr;
	
	buf->spalten = 0;
	for (i = 0; i < buf->zeilen; i++)
	{
		ptr = buf->tab[i];
		len = strlen(ptr);
		if (tab_expand(&ptr, &len, tabstop))
			SFREE(ptr);
		if (len > buf->spalten)
			buf->spalten = len;
	}
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL liste_proc(WIND_MESSAGE msg, WINDOW_DEF *window, _VOID *buf)
{
	WINDTXT *ptr;
	L_GRECT show;
	_WORD w, h;
	
	switch (msg)
	{
	case WMY_OPEN:
		ptr = buf;
		if (ptr->zeilen != 0)
		{
			ptr->tab = MALLOC(ptr->zeilen * sizeof(_UBYTE *), "liste_proc: tab");
			if (ptr->tab == NULL)
				return FALSE;
			if (draw_monster_liste(ptr->tab) == 0)
				return FALSE;
		}
		max_spalten(buf);
		W_GetFontSize(window, ptr->fontsize, &w, &h);
		Wind_SetFac(window, w, h, 1, 1);
		show.xx = 0;
		show.yy = 0;
		show.ww = ptr->spalten * w;
		show.hh = ptr->zeilen * h;
		Wind_SetDoc(window, &show);
		break;
	case WMY_CLOSE:
		ptr = buf;
		if (ptr->window != NULL)
			*(ptr->window) = NULL;
		if (ptr->ptr != NULL)
			FREE(ptr->ptr, ptr->size);
		if (ptr->tab != NULL)
			FREE(ptr->tab, ptr->zeilen * sizeof(_UBYTE *));
		OFREE(ptr);
		break;
	case WMY_UPDATE:
		ptr = Wind_Buf_Ptr(window);
		draw_text(window, ptr, buf);
		break;
	case WMY_OUT:
#if DEMO
		demo();
		break;
#else
		ptr = Wind_Buf_Ptr(window);
		return print_text_out(ptr, buf, NULL);
#endif /* DEMO */
	case WMY_PRINT:
#if DEMO
		demo();
		break;
#else
		ptr = Wind_Buf_Ptr(window);
		return print_text_out(ptr, NULL, buf);
#endif /* DEMO */
	default:
		break;
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID show_liste(_UBYTE *name)
{
	DIALOG *ptr;
	WINDTXT *buf;
	_UBYTE str[256];
	
	if (Monster_Liste == NULL)
	{
		buf = NEW(WINDTXT, "show_text: windtxt");
		if (buf != NULL)
		{
			buf->fontsize = FONT_SIZE_NORMAL;
			buf->size = 0;
			buf->window = &Monster_Liste;
			buf->ptr = NULL;
			buf->tab = NULL;
			ptr = show_string("Creating monster list");
			buf->zeilen = draw_monster_liste(NULL);
			hide_string(ptr);
			
			sprintf(str, "Monster list: %s", name);
			Monster_Liste = Wind_Open(W_LISTE, WINDOW_ATTRIBUTES, liste_proc, str, buf);
		}
	}
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL _VOID show_statistik(_UBYTE *name)
{
	UNUSED(name);
	if (Statistik == NULL)
	{
	}
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL save_statistik(PATH *p)
{
	_BOOL retV = TRUE;
	
	if (Statistik != NULL)
	{
#if DEMO
		UNUSED(p);
		demo();
#else
		DIALOG *ptr;
		FILE *fd;
		WINDOW_PROC proc;
		
		set_ext(p, "sta");
		ptr = show_string("Saving statistics");
		if (writeopen(&fd, p->pathname, "w"))
		{
			proc = Wind_Proc_Ptr(Statistik);
			retV = proc(WMY_OUT, Statistik, fd);
			fileclose(fd);
		} else
		{
			retV = FALSE;
		}
		hide_string(ptr);
#endif /* DEMO */
	}
	return retV;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL print_statistik(_VOID)
{
	return print_text_file(Statistik, "Printing statistics");
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL _VOID close_all_windows(_BOOL delete)
{
	Wind_Close_All(delete);
}
