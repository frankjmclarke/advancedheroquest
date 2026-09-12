/******************************************************************************
 * WINDOWS/W_PRINT.C
 ******************************************************************************/

#define __PRINTER_IMPLEMENTATION__
#include <w_print.h>
#include <windows_.h>
#include <commdlg.h>
#include <debug.h>
#include <termproc.h>
#include <grect.h>
#ifndef __WIN32__
#include <print.h>
#endif
#include <ro_mem.h>
#include <boxf.h>
#include <openwork.h>
#include <routine.h>
#include <w_draw.h>


#define USE_ABORT 0


LOCAL PRINTDLG *pd;

struct _printer {
	HDC hDC;
	_BOOL printing;
	_BOOL print_aborted;
	_BOOL grafic_dev;
#if USE_ABORT
	ABORTPROC abort_proc;
#endif
	int jobID;
	_WORD xpos;
	_WORD ypos;
	_WORD text_width;
	_WORD text_height;
	_WORD fontsize;
	GRECT pagesize;
};

/* -------------------------------------------------------------------------- */

LOCAL _VOID exit_print(_VOID)
{
	if (pd != NULL)
	{
		OFREE(pd);
		pd = NULL;
	}
}

/* -------------------------------------------------------------------------- */

LOCAL _BOOL init_print(_VOID)
{
	if (pd == NULL)
	{
		pd = NEW(PRINTDLG, "init_print");
		if (pd == NULL)
			return FALSE;
		MemSetZeroStruct(*pd);
		InstallTermproc(exit_print);
	}
	return TRUE;
}

/* -------------------------------------------------------------------------- */

LOCAL _VOID W_Scale(HDC hDC, CONST GRECT *real, CONST GRECT *virtual)
{
	SetMapMode(hDC, MM_ANISOTROPIC);
	SetViewportOrgEx(hDC, real->g_x, real->g_y, NULL);
	SetViewportExtEx(hDC, real->g_w, real->g_h, NULL);
	SetWindowOrgEx(hDC, virtual->g_x, virtual->g_y, NULL);
	SetWindowExtEx(hDC, virtual->g_w, virtual->g_h, NULL);
}

/* -------------------------------------------------------------------------- */

#if USE_ABORT

BOOL EXPORT w_print_abort(HDC hdc, int error)
{
	UNUSED(hdc);
	if (error < 0)
		return FALSE;
	return TRUE;
}
#endif /* USE_ABORT */

/* -------------------------------------------------------------------------- */

LOCAL _BOOL W_G_Open_Printer(PRINTER *printer, CONST _UBYTE *PrintJob, _BOOL grafic_dev)
{
	if (init_print() == FALSE)
		return FALSE;
	printer->hDC = NO_DC;
	printer->printing = FALSE;
	printer->print_aborted = FALSE;
	printer->grafic_dev = grafic_dev;
	printer->jobID = 0;
	printer->xpos = 0;
	printer->ypos = -1;
	printer->fontsize = FONT_SIZE_NORMAL;
	printer->text_width = 0;
	printer->text_height = 0;
	printer->pagesize.g_h = 0;
	pd->lStructSize = sizeof(*pd);
	pd->hwndOwner = GlMainHwnd;
	pd->Flags = PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION /* | PD_HIDEPRINTTOFILE */ | (pd->Flags & (PD_PRINTTOFILE | PD_COLLATE | PD_PAGENUMS | PD_SELECTION));

	if (PrintDlg(pd) != 0)
	{
		RECT re;
		DOCINFO doc;

		printer->hDC = pd->hDC;

		MemSetZeroStruct(doc);
		doc.cbSize = sizeof(doc);
		doc.lpszDocName = PrintJob;
		doc.lpszOutput = NULL;
		printer->print_aborted = FALSE;
		for (;;)
		{
			_WORD button;
			int jobID;
			
			jobID = StartDoc(printer->hDC, &doc);
			printer->jobID = jobID;
			if (printer->jobID == SP_ERROR && (pd->Flags & PD_PRINTTOFILE)) /* Dateiauswahl abgebrochen? */
				break;
#if 0
			if ((pd->Flags & PD_PRINTTOFILE) && pd->hDevNames != NULL)
			{
				DEVNAMES *names;

				names = GlobalLock(pd->hDevNames);
				doc.lpszOutput = (_UBYTE *) names + names->wOutputOffset;
				GlobalUnlock(pd->hDevNames);
			}
#endif
			if (printer->jobID > 0)
			{
				printer->printing = TRUE;
				break;
			}
			button = ok_retry_cancel("Printer not ready");
			if (button == 1) /* ignore/continue */
			{
				printer->printing = TRUE;
				break;
			}
			if (button != 2) /* cancel */
				break;
			/* else retry */
		}

		if (printer->printing)
		{
			GRECT gr;
			
			GetClipBox(printer->hDC, &re);
			RectToGrect(&gr, &re);
			rc_copy(&gr, &printer->pagesize);
			
			if (StartPage(printer->hDC) < 0)
			{
				printer->print_aborted = TRUE;
			} else
			{
				W_Scale(printer->hDC, &gr, &gr);
			}
			DeleteObject(W_GetFont(printer->hDC, printer->fontsize, &printer->text_width, &printer->text_height));
#if USE_ABORT
			printer->abort_proc = (ABORTPROC) MakeProcInstance((FARPROC) w_print_abort, GetInstance());
			SetAbortProc(printer->hDC, printer->abort_proc);
#endif /* USE_ABORT */
		}
	}
#if 1
	if (pd->hDevMode != NULL)
	{
		GlobalFree(pd->hDevMode);
		pd->hDevMode = NULL;
	}
	if (pd->hDevNames != NULL)
	{
		GlobalFree(pd->hDevNames);
		pd->hDevNames = NULL;
	}
#endif
	if (!printer->printing)
	{
		if (printer->hDC != NO_DC)
		{
			DeleteDC(printer->hDC);
			printer->hDC = NO_DC;
		}
	}
	return printer->printing;
}

/* -------------------------------------------------------------------------- */

LOCAL _VOID W_G_Close_Printer(PRINTER *printer)
{
	if (printer->hDC != NO_DC)
	{
#if 0
		Escape(printer->hDC, NEWFRAME, 0, NULL, NULL);
		Escape(printer->hDC, ENDDOC, 0, NULL, NULL);
#else
		if (printer->ypos >= 0)
			if (EndPage(printer->hDC) < 0)
				printer->print_aborted = TRUE;
		if (printer->print_aborted)
			AbortDoc(printer->hDC);
		else
			EndDoc(printer->hDC);
#endif
		SelectObject(printer->hDC, GetStockObject(SYSTEM_FONT));
		SelectObject(printer->hDC, GetStockObject(NULL_PEN));
		SelectObject(printer->hDC, GetStockObject(NULL_BRUSH));
		SelectObject(printer->hDC, CreateRectRgn(0, 0, 0, 0));
		DeleteDC(printer->hDC);
#if USE_ABORT
		if (printer->printing)
			FreeProcInstance((FARPROC) printer->abort_proc);
#endif
		printer->printing = FALSE;
	}
}

/* -------------------------------------------------------------------------- */

GLOBAL _VOID Printer_NewPage(PRINTER *printer)
{
#if 0
	Escape(printer->hDC, NEWFRAME, 0, NULL, NULL);
#else
	if (printer->ypos >= 0)
		EndPage(printer->hDC);
	StartPage(printer->hDC);
#endif
	printer->ypos = 0;
	printer->xpos = 0;
}

/* -------------------------------------------------------------------------- */

GLOBAL _BOOL Printer_NewLine(PRINTER *printer)
{
	_BOOL retV;
	
	if (printer->grafic_dev)
	{
		retV = FALSE;
	} else
	{
		if (printer->ypos < 0)
			Printer_NewPage(printer);
		printer->xpos = 0;
		printer->ypos += printer->text_height;
		if (printer->ypos >= printer->pagesize.g_h)
		{
			EndPage(printer->hDC);
			printer->ypos = -1;
		}
		retV = TRUE;
	}
	return retV; 
}

/* -------------------------------------------------------------------------- */

GLOBAL PRINTER *Printer_Open(CONST _UBYTE *PrintJob, _BOOL grafic_dev)
{
	PRINTER *printer;
	
	printer = NEW(PRINTER, "Printer_Open");
	if (printer == NULL)
		return NULL;
	if (!W_G_Open_Printer(printer, PrintJob, grafic_dev))
	{
		W_G_Close_Printer(printer);
		OFREE(printer);
		return NULL;
	}
	return printer;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Printer_Close(PRINTER *printer)
{
	if (printer != NULL)
	{
		W_G_Close_Printer(printer);
		OFREE(printer);
	}
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID w_Text(PRINTER *printer, _WORD x, _WORD y, _WORD size, CONST _UBYTE *text, size_t len)
{
	HFONT font, oldFont;
	_WORD w, h;
	int oldMapMode;
	
	font = W_GetFont(printer->hDC, size, &w, &h);
	if (font != NO_FONT)
	{
		oldMapMode = SetMapMode(printer->hDC, MM_TEXT);
		oldFont = SelectObject(printer->hDC, font);
		SetTextColor(printer->hDC, W_PAL_BLACK);
		SetBkMode(printer->hDC, OPAQUE);
		TextOut(printer->hDC, x, y, text, len);
		SelectObject(printer->hDC, oldFont);
		DeleteObject(font);
		SetMapMode(printer->hDC, oldMapMode);
	}
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL Printer_Write(PRINTER *printer, CONST _UBYTE *buf, _LONG size)
{
	if (printer != NULL && printer->hDC != NO_DC)
	{
		if (printer->grafic_dev)
		{
		} else
		{
			if (printer->ypos < 0)
				Printer_NewPage(printer);
			w_Text(printer, printer->xpos, printer->ypos, printer->fontsize, buf, size);
			printer->xpos += (_WORD)size * printer->text_width;
		}
		return TRUE;
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Printer_Bitmap(PRINTER *printer, MFDB *src, _WORD x, _WORD y, _WORD w, _WORD h, _WORD dx, _WORD dy, _WORD zoom)
{
	COLORREF oldColor;
	COLORREF oldBk;
	HDC hdcMem;
	POINT ptSize, ptOrg;
	_WORD dw, dh;
	
	get_mfdb_info(src, &dw, &dh, NULL);
	if ((x + w) > dw)
		w = dw - x;
	if (w <= 0)
		return;
	if ((y + h) > dh)
		h = dh - y;
	if (h <= 0)
		return;
	oldColor = SetTextColor(printer->hDC, W_PAL_BLACK);
	oldBk = SetBkColor(printer->hDC, W_PAL_WHITE);

	hdcMem = CreateCompatibleDC(printer->hDC);
	SelectObject(hdcMem, get_mfdb_bitmap(src));
	
	if (zoom == 0)
	{
		ptSize.x = printer->pagesize.g_w;
		ptSize.y = printer->pagesize.g_h;
	} else
	{
		ptSize.x = w * zoom;
		ptSize.y = h * zoom;
	}
	DPtoLP(printer->hDC, &ptSize, 1);
	ptOrg.x = x;
	ptOrg.y = y;
	DPtoLP(printer->hDC, &ptOrg, 1);
	
	StretchBlt(printer->hDC, dx, dy, ptSize.x, ptSize.y, hdcMem, ptOrg.x, ptOrg.y, w, h, SRCCOPY);
	DeleteDC(hdcMem);
	SetTextColor(printer->hDC, oldColor);
	SetBkColor(printer->hDC, oldBk);
}
