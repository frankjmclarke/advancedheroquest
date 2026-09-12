/*****************************************************************************
 * WINDOWS/WINDOW.C
 *****************************************************************************/

#define __WINDOW_IMPLEMENTATION__
#include <window.h>
#include <windows_.h>
#include <w_draw.h>
#include <ro_mem.h>
#include <routine.h>
#include <grect.h>
#include <termproc.h>
#ifndef __WIN32__
#include <wownt16.h>
#endif
#include <boxf.h>
#include <openwork.h>
#include <debug.h>
#include <ro_help.h>
#include <keycode.h>

#define MAXTITLE 80
#define WI_MIN_SIZE 75

#define MAX_USE_WIND 100

/* Erweiterte Window Attributes */
#define WR_NONE		 0x0000
#define WR_FULLED	 0x0001
#define WR_ONTOP	 0x0002
#define WR_NOTOP	 0x0004
#define WR_RESIDENT	 0x0008
#define WR_NOSCROLL	 0x0010
#define WR_OPENING	 0x0020
#define WR_NOBACK	 0x0040
#define WR_AUTOPOS	 0x0080
#define WR_DONTBLANK 0x0100
#define WR_MODAL     0x0200
#define WR_NORESTORE 0x0400
#define WR_ICONIFIED 0x0800
#define WR_WINDOBJ   0x1000
#define WR_HASWIND   0x2000
#define WR_BACKBUF   0x4000
#define WR_OVERLAPPED 0x8000

struct _window_def
{
	GRECT work;
	GRECT scroll;
	GRECT lastpos;
	GRECT wr_open_size;
	
	WINDOW_PROC wr_proc;
	_UWORD wr_flags;
	_UWORD wr_state;
	_WORD wr_type;

	L_GRECT wr_show;
	_WORD wr_xfac;
	_WORD wr_yfac;
	_WORD wr_xunits;
	_WORD wr_yunits;
	GRECT wr_currpos;
	
	_VOID    *wr_buf;
	_UBYTE   wr_title[MAXTITLE];
	
	HWND hwnd;
	HDC hDC;
	_LONG scrollhsize;
	_LONG scrollhpos;
	_LONG scrollvsize;
	_LONG scrollvpos;
	
	WINDOW_DEF *wr_next;
};

#define X_WI_RASTER 8

#define align( xx, nn )    ((nn)*(((xx)+(nn)/2)/(nn)))

GLOBAL HWND GlMainHwnd;
GLOBAL HWND MdiClientHwnd;

LOCAL WH_READWRITE fu_Wind_Read_Write;
LOCAL WH_SAVERESTORE fu_Wind_Save_Restore;
LOCAL MENU_FUNC gl_event_menu;
LOCAL WINDOW_DEF *window_list;
LOCAL _BOOL all_iconified;
LOCAL HMENU GlMenuHandle;
LOCAL HACCEL GlAccelTable;
LOCAL WNDPROC mdiclient_proc;

#define WCN_Main "HQ_Main"
#define WCN_MDI_Client "HQ_Client"


#define ValidWin(wr) \
	((wr) != NULL && (wr)->hwnd != NO_WINDOW)

#define align_koord(xx, nn)    ((nn)*(((xx)+(nn)/2)/(nn)))
#define truncate_koord(xx, nn) ((nn)*(((xx)+(nn)-1)/(nn)))

#define FOR_ALL_WINDS(wr) for (wr = window_list; wr != NULL; wr = wr->wr_next)

#ifndef SIF_ALL

/* SetScrollInfo/GetScrollInfo */
#define SIF_ALL (23)
#define SIF_PAGE	(2)
#define SIF_POS (4)
#define SIF_RANGE	(1)
#define SIF_DISABLENOSCROLL (8)

typedef struct tagSCROLLINFO {
	_ULONG cbSize;
	_ULONG fMask;
	long  nMin;
	long  nMax;
	_ULONG nPage;
	long  nPos;
	long  nTrackPos;
} SCROLLINFO, *LPSCROLLINFO;
typedef SCROLLINFO const *LPCSCROLLINFO;


int WINAPI SetScrollInfo(HWND hwnd, long nbar, LPCSCROLLINFO si, BOOL redraw);
BOOL WINAPI GetScrollInfo(HWND hwnd, long nbar, LPSCROLLINFO si);

#endif /* SIF_ALL */

#define PROGRAM_EXIT 2

#define MSG_NO_WINDOW "Kein Fenster mehr vefügbar!"

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL _VOID (*p_setscrollinfo)(HWND hwnd, long nbar, LPCSCROLLINFO si, BOOL redraw);

#ifdef __WIN32__

LOCAL _VOID call_setscrollinfo(HWND hwnd, long nbar, LPCSCROLLINFO si, BOOL redraw)
{
	SetScrollInfo(hwnd, nbar, si, redraw);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL have_setscrollinfo(_VOID)
{
	p_setscrollinfo = call_setscrollinfo;
	return TRUE;
}

#else

/*** ---------------------------------------------------------------------- ***/

LOCAL DWORD user32;
LOCAL DWORD setscrollinfo_addr;

#define WOW_TYPE_HWND 0

extern DWORD WINAPI CallProc32W(DWORD p1, DWORD p2, DWORD p3, DWORD p4, DWORD proc_addr, DWORD addr_conv, DWORD nparams);
extern DWORD WINAPI WOWHandle32(WORD h16, int type);

#define HWND_32(h16) WOWHandle32((WORD)(h16), WOW_TYPE_HWND)

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID call_setscrollinfo(HWND hwnd, long nbar, LPCSCROLLINFO si, BOOL redraw)
{
	DWORD wnd;
	
	wnd = (DWORD)(UINT)(hwnd);
	CallProc32W(wnd, (DWORD) nbar, (DWORD) si, (DWORD) redraw, setscrollinfo_addr, 0x02l, 4);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID exit_setscrollinfo(_VOID)
{
	if (user32 != 0 && user32 != (DWORD)-1)
	{
		FreeLibrary32W(user32);
		user32 = (DWORD)-1;
		p_setscrollinfo = FUNK_NULL;
	}
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL have_setscrollinfo(_VOID)
{
	if (p_setscrollinfo)
		return TRUE;
	if (user32 == 0)
	{
		HINSTANCE kernel;
		UINT old_error_mode;
		FARPROC loadlibraryex32w;
		
		old_error_mode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
		
		user32 = (DWORD) -1;
		kernel = LoadLibrary("KERNEL");
		if (kernel >= HINSTANCE_ERROR)
		{
			loadlibraryex32w = GetProcAddress(kernel, "LoadLibraryEx32W");
			if (loadlibraryex32w != FUNK_NULL)
			{
				user32 = (*((DWORD (WINAPI *)(LPCSTR, DWORD, DWORD)) loadlibraryex32w))("USER32", NULL, 0l);
				/* user32 = LoadLibraryEx32W("USER32", NULL, 0l); */
				if (user32 == 0)
					user32 = (DWORD) -1;
			}
			FreeLibrary(kernel);
		}
		
		SetErrorMode(old_error_mode);
	}
	if (user32 == (DWORD)-1)
		return FALSE;
	setscrollinfo_addr = GetProcAddress32W(user32, "SetScrollInfo");
	if (setscrollinfo_addr == 0)
		return FALSE;
	p_setscrollinfo = call_setscrollinfo;
	InstallTermproc(exit_setscrollinfo);
	return TRUE;
}

#endif /* __WIN32__ */

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL WINDOW_DEF *new_wind_ptr(_VOID)
{
	WINDOW_DEF *wr;
	
	wr = NEW(WINDOW_DEF, "new_wind_ptr");
	if (wr != NULL)
	{
		MemSetZeroStruct(*wr);
		wr->hwnd = NO_WINDOW;
		wr->wr_next = window_list;
		window_list = wr;
	}
	return wr;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL win_do_resize(HWND hwnd)
{
	RECT r;

	if (hwnd == NO_WINDOW || !IsWindowVisible(hwnd))
		return FALSE;

	GetClientRect(hwnd, &r);
	if (hwnd == GlMainHwnd)
	{
		if (MdiClientHwnd != NO_WINDOW)
		{
			MoveWindow(MdiClientHwnd, r.left, r.top, r.right - r.left, r.bottom - r.top, TRUE);
		}
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID get_current_pos(WINDOW_DEF *wr, HWND hwnd)
{
	RECT rect, rMain;

	rMain.left = rMain.top = 0;
	if (GetParent(hwnd) == MdiClientHwnd)
		GetWindowRect(MdiClientHwnd, &rMain);
	GetWindowRect(hwnd, &rect);

	RectToGrect(&wr->wr_currpos, &rect);
	wr->wr_currpos.g_x -= rMain.left;
	wr->wr_currpos.g_y -= rMain.top;
	GetClientRect(hwnd, &rect);
	RectToGrect(&wr->work, &rect);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL Wind_Hide_Show_Init(WH_READWRITE fu_Read_Write, WH_SAVERESTORE fu_Save_Restore)
{
	fu_Wind_Read_Write = fu_Read_Write;
	fu_Wind_Save_Restore = fu_Save_Restore;
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL Wind_Hide_Show_Windows(_BOOL hide)
{
	WIND_RESTORE *wr;
	size_t ii;
	WINDOW_DEF *win;
	RECT r;
	
	if (hide)
	{
		ii = 1;
		for (win = window_list; win != NULL; win = win->wr_next)
			ii++;
		wr = MALLOC(sizeof(WIND_RESTORE) * ii, "Wind_Hide_Show_Windows: hide");
		if (wr != NULL)
		{
			MemSetZero(wr, ii * sizeof(*wr));
			if (GlMainHwnd != NO_WINDOW)
			{
				wr[0].reOpen = 1;
				wr[0].group = 0;
				GetWindowRect(GlMainHwnd, &r);
				RectToGrect(&wr[0].pos, &r);
				wr[0].var_bez = 0;
				if (fu_Wind_Save_Restore != FUNK_NULL &&
					!fu_Wind_Save_Restore(TRUE, wr[0].group, &wr[0].var_bez))
				{
					FREE(wr, ii * sizeof(*wr));
					return FALSE;
				}
			}
			for (ii = 1, win = window_list; win != NULL; ii++, win = win->wr_next)
			{
				wr[ii].reOpen = 0;
				wr[ii].group = win->wr_type;
	
				if (win->hwnd != NO_WINDOW)
				{
					wr[ii].reOpen = 1;
					get_current_pos(win, win->hwnd);
					if (win->wr_state & WR_ICONIFIED)
						rc_copy(&win->lastpos, &win->wr_open_size);
					else
						rc_copy(&win->wr_currpos, &win->wr_open_size);
					if (fu_Wind_Save_Restore != FUNK_NULL &&
						!fu_Wind_Save_Restore(TRUE, wr[ii].group, &wr[ii].var_bez))
						return FALSE;
				}
				wr[ii].pos = win->wr_open_size;
			}
			if (fu_Wind_Read_Write != FUNK_NULL)
				fu_Wind_Read_Write(TRUE, wr, ii);
			FREE(wr, ii * sizeof(*wr));
		}
		Wind_Close_All(TRUE);
	} else
	{
		ii = 0;
		wr = NULL;
		if (fu_Wind_Read_Write != FUNK_NULL)
		{
			ii = fu_Wind_Read_Write(FALSE, NULL, 0);
			if (ii <= 0)
				ii = 1;
			wr = MALLOC(ii * sizeof(*wr), "Wind_Hide_Show_Windows: show");
			if (wr != NULL)
			{
				MemSetZero(wr, ii * sizeof(*wr));
				ii = fu_Wind_Read_Write(FALSE, wr, ii);
				if (GlMainHwnd != NO_WINDOW && ii > 0 && wr[0].reOpen)
				{
					GrectToRect(&r, &wr[0].pos);
					MoveWindow(GlMainHwnd, r.left, r.top, r.right - r.left, r.bottom - r.top, TRUE);
				}
			}
		}
		win_do_resize(GlMainHwnd);
		ShowWindow(GlMainHwnd, GlCmdShow);
		UpdateWindow(GlMainHwnd);
		if (wr != NULL)
		{
			if (fu_Wind_Save_Restore != FUNK_NULL &&
				!fu_Wind_Save_Restore(FALSE, wr[0].group, &wr[0].var_bez))
				return FALSE;
			while (ii > 0)
			{
				--ii;
				if (wr[ii].group != 0)
				{
					win = new_wind_ptr();
					if (win != NULL)
					{
						win->wr_open_size = wr[ii].pos;
						win->wr_type = wr[ii].group;
						if (wr[ii].reOpen > 0)
						{
							if (fu_Wind_Save_Restore != FUNK_NULL)
								fu_Wind_Save_Restore(FALSE, wr[ii].group, &wr[ii].var_bez);
						}
					}
				}
			}
			SFREE(wr);
		}
	}
	return TRUE;
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL _VOID setWiPtr(HWND hwnd, WINDOW_DEF *ptr)
{
	SetWindowLong(hwnd, 0, (_LONG)ptr);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL WINDOW_DEF *getWiPtr( HWND hwnd )
{
	return (WINDOW_DEF *)GetWindowLong(hwnd, 0);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL HWND top_wind_handle(_VOID)
{
	if (MdiClientHwnd != NO_WINDOW)
		return GetTopWindow(MdiClientHwnd);
	return GetTopWindow(HWND_DESKTOP);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL getWindBuf(WINDOW_DEF **wiBuf, HWND hwnd)
{
	if (hwnd != NO_WINDOW)
	{
		WINDOW_DEF *wr = getWiPtr(hwnd);

		if (wr != NULL)
			if (wr->hwnd == hwnd)
			{
				*wiBuf = wr;
				return TRUE;
			}
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL WINDOW_DEF *Wind_Top(_VOID)
{
	WINDOW_DEF *wr;

	if (getWindBuf(&wr, top_wind_handle()))
		return wr;
	return NULL;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Wind_Redraw(WINDOW_DEF *wr)
{
	if (ValidWin(wr))
	{
		/* InvalidateRgn(WINOS(wr)->hwnd, NULL, TRUE); */
		/* UpdateWindow(WINOS(wr)->hwnd); haengt */
		RedrawWindow(wr->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
	}
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Wind_Redraw_Rect(WINDOW_DEF *wr, CONST GRECT *gr, _BOOL now)
{
	if (ValidWin(wr))
	{
		RECT rect;
		GRECT work;

		Wind_GetWork(wr, &work);
		if (rc_intersect(gr, &work))
		{
			work.g_x += wr->work.g_x;
			work.g_y += wr->work.g_y;
			GrectToRect(&rect, &work);
			if (now)
			{
#if 0
				InvalidateRect(os->hwnd, &rect, TRUE);
#endif
				UpdateWindow(wr->hwnd);
				RedrawWindow(wr->hwnd, &rect, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
			} else
			{
				InvalidateRect(wr->hwnd, &rect, TRUE);
				RedrawWindow(wr->hwnd, &rect, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
			}
		}
	}
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Wind_Set_Sliders(WINDOW_DEF *wr, _WORD which, _WORD mode)
{
	if (ValidWin(wr))
	{
		if (((wr->wr_flags & WAT_HORSLIDE) && (which & HORIZONTAL)) ||
			((wr->wr_flags & WAT_VERSLIDE) && (which & VERTICAL)))
		{
			_LONG pos, range;
			HWND hwnd = wr->hwnd;
			GRECT gr;

			Wind_GetScroll(wr, &gr);
			if ((wr->wr_flags & WAT_HORSLIDE) && (which & HORIZONTAL))
			{
				if (!have_setscrollinfo())
				{
					pos = wr->wr_show.xx;
					range = wr->wr_show.ww - gr.g_w;
					if (range < 0)
						range = 0;
					if ((mode & SLSIZE) && range != wr->scrollhsize)
						SetScrollRange(hwnd, SB_HORZ, 0, (_WORD) range, !(mode & SLPOS) || pos == wr->scrollhpos);
					if ((mode & SLPOS) && pos != wr->scrollhpos)
						SetScrollPos(hwnd, SB_HORZ, (_WORD) pos, TRUE);
				} else
				{
					SCROLLINFO si;

					pos = wr->wr_show.xx;
					range = wr->wr_show.ww;
					if (pos != wr->scrollhpos || range != wr->scrollhsize)
					{
						si.cbSize = sizeof(si);
						si.fMask = SIF_PAGE|SIF_RANGE;
						if (mode & SLPOS)
							si.fMask |= SIF_POS;
						si.nMin = 0;
						si.nMax = range - 1;
						if (si.nMax < 0)
							si.nMax = 0;
						si.nPage = gr.g_w;
						si.nPos = pos;
						si.nTrackPos = si.nPos;
						(*p_setscrollinfo)(hwnd, SB_HORZ, &si, TRUE);
					}
				}
				wr->scrollhpos = pos;
				wr->scrollhsize = range;
			}
			if ((wr->wr_flags & WAT_VERSLIDE) && (which & VERTICAL))
			{
				if (!have_setscrollinfo())
				{
					pos = wr->wr_show.yy;
					range = wr->wr_show.hh - gr.g_h;
					if (range < 0)
						range = 0;
					if ((mode & SLSIZE) && range != wr->scrollvsize)
						SetScrollRange(hwnd, SB_VERT, 0, (_WORD) range, !(mode & SLPOS) || pos == wr->scrollvpos);
					if ((mode & SLPOS) && pos != wr->scrollvpos)
						SetScrollPos(hwnd, SB_VERT, (_WORD) pos, TRUE);
				} else
				{
					SCROLLINFO si;

					pos = wr->wr_show.yy;
					range = wr->wr_show.hh - 1;
					if (range < 0)
						range = 0;
					if (pos != wr->scrollvpos || range != wr->scrollvsize)
					{
						si.cbSize = sizeof(si);
						si.fMask = SIF_RANGE|SIF_PAGE;
						if (mode & SLPOS)
							si.fMask |= SIF_POS;
						si.nMin = 0;
						si.nMax = range;
						si.nPage = gr.g_h;
						si.nPos = pos;
						si.nTrackPos = si.nPos;
						(*p_setscrollinfo)(hwnd, SB_VERT, &si, TRUE);
					}
				}
				wr->scrollvpos = pos;
				wr->scrollvsize = range;
			}
		}
	}
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID W_BeginPaint(WINDOW_DEF *wr, PAINTSTRUCT *ps, GRECT *gr)
{
	RECT r;
	
	wr->hDC = BeginPaint(wr->hwnd, ps);
	GetClientRect(wr->hwnd, &r);
	RectToGrect(gr, &r);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID W_EndPaint(WINDOW_DEF *wr, PAINTSTRUCT *ps)
{
	SelectObject(wr->hDC, GetStockObject(SYSTEM_FONT));
	SelectObject(wr->hDC, GetStockObject(NULL_PEN));
	SelectObject(wr->hDC, GetStockObject(NULL_BRUSH));
	EndPaint(wr->hwnd, ps);
	wr->hDC = NO_DC;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Wind_GetSize(WINDOW_DEF *wr, GRECT *gr)
{
	if (!ValidWin(wr))
	{
		xywh2rect(0, 0, 0, 0, gr);
		return;
	}
	rc_copy(&wr->wr_currpos, gr);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Wind_GetWork(WINDOW_DEF *win, GRECT *gr)
{
	Wind_GetSize(win, gr);
	gr->g_x = 0;
	gr->g_y = 0;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID get_work(WINDOW_DEF *wr)
{
	GRECT work;

	Wind_GetSize(wr, &work);
	rc_copy(&wr->work, &work);
	work.g_x = 0;
	work.g_y = 0;
	rc_copy(&work, &wr->scroll);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Wind_GetScroll(WINDOW_DEF *wind, GRECT *gr)
{
	if (ValidWin(wind))
	{
		get_work(wind);
		rc_copy(&wind->scroll, gr);
	} else
	{
		xywh2rect(0, 0, 0, 0, gr);
	}
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID doUpdate(WINDOW_DEF *wr)
{
	GRECT t1, gr;
	PAINTSTRUCT ps;
	
	W_BeginPaint(wr, &ps, &t1);

	if (!(wr->wr_state & WR_ICONIFIED))
	{
		Wind_GetScroll(wr, &gr);
		get_work(wr);

		if (!(wr->wr_state & WR_DONTBLANK))
		{
			GRECT r3;

			rc_copy(&t1, &r3);
			if (rc_intersect(&wr->scroll, &r3))
				W_Clear_Rect(wr, &r3);
		}

		(*wr->wr_proc)(WMY_UPDATE, wr, &t1);
		Wind_Set_Sliders(wr, HORIZONTAL | VERTICAL, SLPOS | SLSIZE);
	}

	W_EndPaint(wr, &ps);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID iconify_window(WINDOW_DEF *wr)
{
	if (!(wr->wr_state & WR_ICONIFIED))
	{
		rc_copy(&wr->wr_currpos, &wr->lastpos);
		wr->wr_state |= WR_ICONIFIED;
		get_current_pos(wr, wr->hwnd);
		get_work(wr);
	}
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID uniconify_window(WINDOW_DEF *wr)
{
	if ((wr->wr_state & WR_ICONIFIED))
	{
		wr->wr_state &= ~WR_ICONIFIED;
		get_current_pos(wr, wr->hwnd);
		get_work(wr);
	}
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL alliconify(_VOID)
{
	if (!all_iconified)
	{
		all_iconified = TRUE;
		return TRUE;
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL alluniconify(_VOID)
{
	if (all_iconified)
	{
		all_iconified = FALSE;
		return TRUE;
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID put_on_top(WINDOW_DEF *wr)
{
	WINDOW_DEF *parent;

	if (wr != NULL && wr != window_list && window_list != NULL)
	{
		for (parent = window_list; parent->wr_next != wr; parent = parent->wr_next)
			;
		parent->wr_next = wr->wr_next;
		wr->wr_next = window_list;
		window_list = wr;
	}
}

/*** ---------------------------------------------------------------------- ***/

#if 0
LOCAL _VOID put_on_bottom(WINDOW_DEF *wr)
{
	WINDOW_DEF *parent;

	parent = window_list;
	if (wr != NULL && parent != NULL && parent->wr_next != NULL)
	{
		if (wr == parent)
		{
			parent = wr->wr_next;
			window_list = parent;
		} else
		{
			for (parent = window_list; parent->wr_next != wr; parent = parent->wr_next)
				;
			parent->wr_next = wr->wr_next;
		}
		while (parent->wr_next != NULL)
			parent = parent->wr_next;
		parent->wr_next = wr;
		wr->wr_next = NULL;
	}
}
#endif

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID untopped_wind(WINDOW_DEF *wr)
{
	if (wr != NULL)
	{
		if (wr->wr_state & WR_ONTOP)
		{
			wr->wr_proc(WMY_UNTOP, wr, wr->wr_buf);
			wr->wr_state &= ~WR_ONTOP;
		}
	}
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID topped_wind(WINDOW_DEF *wr)
{
	if (wr != NULL)
	{
		put_on_top(wr);
		if (!(wr->wr_state & WR_ONTOP))
		{
			wr->wr_proc(WMY_TOP, wr, wr->wr_buf);
			wr->wr_state |= WR_ONTOP;
		}
	}
}

/*** ---------------------------------------------------------------------- ***/

LOCAL LRESULT close_mainwind(_VOID)
{
	if (GlMainHwnd != NO_WINDOW)
	{
		return SendMessage(GlMainHwnd, WM_CLOSE, 0, 0L);
	}
	return 0;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL HWND hndl_button(HWND hwnd, unsigned int message, LPARAM lParam)
{
	_WORD mausX, mausY;
	WIPR_HIT hit;
	WINDOW_DEF *wr;

	mausX = LOWORD(lParam);
	mausY = HIWORD(lParam);

	if (GetCapture() != NO_WINDOW)
	{
		ReleaseCapture();
	}

	wr = getWiPtr(hwnd);
	if (wr != NULL)
	{
		hwnd = wr->hwnd;
		if (!(wr->wr_state & WR_ICONIFIED))
		{
			_BOOL quit;

			hit.xx = mausX + (_WORD) wr->wr_show.xx;
			hit.yy = mausY + (_WORD) wr->wr_show.yy;
			hit.clicks = 1;
			if (message == WM_LBUTTONDBLCLK ||
				message == WM_RBUTTONDBLCLK ||
				message == WM_MBUTTONDBLCLK)
			{
				hit.clicks = 2;
			} else
			{
				MSG msg;

#if 0
				EventTimer(GetDoubleClickTime());
#endif
				if (PeekMessage(&msg, hwnd, WM_LBUTTONDBLCLK, WM_LBUTTONDBLCLK, PM_REMOVE))
					hit.clicks = 2;
			}
			quit = !wr->wr_proc(WMY_HIT, wr, &hit);
			if (quit)
				close_mainwind();
		} else
		{
			if (!alluniconify())
				uniconify_window(wr);
		}
		hwnd = wr->hwnd;
	}
	return hwnd;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID update_area(_VOID *para, GRECT *area)
{
	WINDOW_DEF *wr = (WINDOW_DEF *) para;

	wr->hDC = GetDC(wr->hwnd);
	if (!(wr->wr_state & WR_DONTBLANK))
	{
		GRECT r3, scroll;

		rc_copy(area, &r3);
		Wind_GetScroll(wr, &scroll);
		if (rc_intersect(&scroll, &r3))
			W_Clear_Rect(wr, &r3);
	}
	(*wr->wr_proc)(WMY_UPDATE, wr, area);
	ReleaseDC(wr->hwnd, wr->hDC);
	wr->hDC = NO_DC;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID do_arrow(WINDOW_DEF *wr, _WORD type, _LONG amount)
{
	GRECT scroll;

	Wind_GetScroll(wr, &scroll);
	Wind_Arrow(wr, type, amount, &scroll, update_area);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL Wind_Scroll_Area(WINDOW_DEF *wr, _WORD dir, _LONG amount, _BOOL scroll, CONST GRECT *drawRedrGr, L_GRECT *show, _WORD xRaster, _WORD yRaster, REDRAW_FUNC func, _VOID *para)
{
	GRECT redrX;
	_LONG oldpos, newpos, max_slide;

	rc_copy(drawRedrGr, &redrX);
	/* redrX.g_w = redrX.g_h = */

	if (dir & HORIZONTAL)
	{
		oldpos = show->xx;
		max_slide = show->ww - ((redrX.g_w / xRaster) * xRaster);
	} else
	{
		oldpos = show->yy;
		max_slide = show->hh - ((redrX.g_h / yRaster) * yRaster);
		xRaster = yRaster;
	}

	newpos = oldpos + amount;
	if (newpos > max_slide)
		newpos = max_slide;
	if (newpos < 0)
		newpos = 0;
	newpos = truncate_koord(newpos, xRaster);
	amount = newpos - oldpos;

	if (amount == 0)
		return FALSE;

	if (dir & HORIZONTAL)
	{
		show->xx = newpos;
		if (scroll && abs(amount) < redrX.g_w)
		{
			W_ScrollGrect(wr, (_WORD)(-amount), 0, drawRedrGr, func, para);
			return FALSE;
		}
	} else
	{
		show->yy = newpos;
		if (scroll && abs(amount) < redrX.g_h)
		{
			W_ScrollGrect(wr, 0, (_WORD)(-amount), drawRedrGr, func, para);
			return FALSE;
		}
	}

	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _WORD window_arrow(WINDOW_DEF *wr, _WORD arrow, _LONG delta, CONST GRECT *redraw, REDRAW_FUNC func)
{
	_WORD dir = 0;
	
	switch (arrow)
	{
	case WA_UPLINE:
	case WA_DNLINE:
	case WA_UPPAGE:
	case WA_DNPAGE:
	case WA_VSLIDE:
		dir = VERTICAL;
		break;
	case WA_LFLINE:
	case WA_RTLINE:
	case WA_LFPAGE:
	case WA_RTPAGE:
	case WA_HSLIDE:
		dir = HORIZONTAL;
		break;
	}
	if (delta != 0 && dir != 0)
	{
		if (Wind_Scroll_Area(wr, dir, delta, !(wr->wr_state & WR_NOSCROLL), redraw, &wr->wr_show, wr->wr_xfac, wr->wr_yfac, func, wr))
			Wind_Redraw_Rect(wr, redraw, FALSE);
		Wind_Set_Sliders(wr, dir, SLPOS);
	}
	return dir;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Wind_Arrow(WINDOW_DEF *wr, _WORD arrow, _LONG amount, CONST GRECT *redraw, REDRAW_FUNC func)
{
	_WORD dir = 0;

	if (wr != NULL)
	{
		switch (arrow)
		{
		case WA_UPLINE:
			amount *= -(wr->wr_yfac * wr->wr_yunits);
			if (wr->wr_flags & WAT_VERSLIDE)
				dir = VERTICAL;
			break;
		case WA_DNLINE:
			amount *= wr->wr_yfac * wr->wr_yunits;
			if (wr->wr_flags & WAT_VERSLIDE)
				dir = VERTICAL;
			break;
		case WA_UPPAGE:
			amount *= -(redraw->g_h);
			if (wr->wr_flags & WAT_VERSLIDE)
				dir = VERTICAL;
			break;
		case WA_DNPAGE:
			amount *= redraw->g_h;
			if (wr->wr_flags & WAT_VERSLIDE)
				dir = VERTICAL;
			break;
		case WA_VSLIDE:
			if (wr->wr_flags & WAT_VERSLIDE)
				dir = VERTICAL;
			break;
		case WA_LFLINE:
			amount *= -(wr->wr_xfac * wr->wr_xunits);
			if (wr->wr_flags & WAT_HORSLIDE)
				dir = HORIZONTAL;
			break;
		case WA_RTLINE:
			amount *= wr->wr_xfac * wr->wr_xunits;
			if (wr->wr_flags & WAT_HORSLIDE)
				dir = HORIZONTAL;
			break;
		case WA_LFPAGE:
			amount *= -(redraw->g_w);
			if (wr->wr_flags & WAT_HORSLIDE)
				dir = HORIZONTAL;
			break;
		case WA_RTPAGE:
			amount *= redraw->g_w;
			if (wr->wr_flags & WAT_HORSLIDE)
				dir = HORIZONTAL;
			break;
		case WA_HSLIDE:
			if (wr->wr_flags & WAT_HORSLIDE)
				dir = HORIZONTAL;
			break;
		}
		if (dir != 0 && amount != 0)
			window_arrow(wr, arrow, amount, redraw, func);
	}
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Wind_Close_All(_BOOL delete)
{
	WINDOW_DEF *wr, *list;

	list = window_list;
	window_list = NULL;
	wr = list;
	while (wr != NULL)
	{
		Wind_Close(wr);
		wr = wr->wr_next;
	}
	/* do it once again in case the list has changed */
	wr = list;
	while (wr != NULL)
	{
		Wind_Close(wr);
		wr = wr->wr_next;
	}
	if (delete)
	{
		wr = list;
		while (wr != NULL)
		{
			list = wr->wr_next;
			OFREE(wr);
			wr = list;
		}
	} else
	{
		window_list = list;
	}
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Wind_Next_Top(_VOID)
{
	WINDOW_DEF *wr, *last;

	last = NULL;
	FOR_ALL_WINDS(wr)
	{
		if (ValidWin(wr))
			last = wr;
	}
	if (last != NULL)
		Wind_On_Top(last);
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL _VOID Wind_Menu_Enable(_WORD obj, _BOOL mode)
{
	if (GlMainHwnd != NO_WINDOW && GlMenuHandle != NO_MENU)
	{
		EnableMenuItem(GlMenuHandle, obj, MF_BYCOMMAND | (mode ? MF_ENABLED : (MF_DISABLED|MF_GRAYED)));
	}
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL Wind_Menu_Enabled(_WORD obj)
{
	if (GlMainHwnd != NO_WINDOW && GlMenuHandle != NO_MENU)
	{
		UINT state = GetMenuState(GlMenuHandle, obj, MF_BYCOMMAND);
		if (state != (UINT)-1)
			return (state & (MF_DISABLED | MF_ENABLED | MF_GRAYED)) == MF_ENABLED;
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Wind_Menu_Check(_WORD obj, _BOOL mode)
{
	if (GlMainHwnd != NO_WINDOW && GlMenuHandle != NO_MENU)
	{
		CheckMenuItem(GlMenuHandle, obj, MF_BYCOMMAND | (mode ? MF_CHECKED : MF_UNCHECKED));
	}
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL Wind_Menu_Checked(_WORD obj)
{
	if (GlMainHwnd != NO_WINDOW && GlMenuHandle != NO_MENU)
	{
		UINT state = GetMenuState(GlMenuHandle, obj, MF_BYCOMMAND);
		if (state != (UINT)-1)
			return (state & (MF_CHECKED | MF_UNCHECKED)) == MF_CHECKED;
	}
	return FALSE;
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL LRESULT CALLBACK useWndProc(HWND hwnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
	/* LOCAL _BOOL doMaus = TRUE; */

	switch (message)
	{
	case WM_CREATE:
		{
			LPCREATESTRUCT cr = (LPCREATESTRUCT) lParam;
			WINDOW_DEF *wi;

			if (GetParent(hwnd) == MdiClientHwnd)
				wi = (WINDOW_DEF *) (((MDICREATESTRUCT *) (cr->lpCreateParams))->lParam);
			else
				wi = (WINDOW_DEF *)(cr->lpCreateParams);
			setWiPtr(hwnd, wi);
		}
		break;

	case WM_ERASEBKGND:
		return 0;

	case WM_PAINT:
		{
			WINDOW_DEF *wr = getWiPtr(hwnd);

			if (wr != NULL)
				doUpdate(wr);
		}
		return 0;

	case WM_VSCROLL:
	case WM_HSCROLL:
		{
			_WORD what = -1;
			_LONG amount = 1;
			WINDOW_DEF *wr;
			GRECT scroll;

			UpdateWindow(hwnd);

			wr = getWiPtr(hwnd);
			if (wr == NULL)
			{
				return 0;
			}
			Wind_GetScroll(wr, &scroll);

			if (message == WM_HSCROLL)
			{
				switch (LOWORD(wParam))
				{
				case SB_TOP:
					amount = (-wr->wr_show.xx);
					what = WA_HSLIDE;
					break;

				case SB_BOTTOM:
					amount = wr->wr_show.ww - scroll.g_w - wr->wr_show.xx;
					what = WA_HSLIDE;
					break;

				case SB_LINEUP:
					what = WA_LFLINE;
					break;
				case SB_LINEDOWN:
					what = WA_RTLINE;
					break;
				case SB_PAGEUP:
					what = WA_LFPAGE;
					break;
				case SB_PAGEDOWN:
					what = WA_RTPAGE;
					break;

				case SB_THUMBTRACK:
				case SB_THUMBPOSITION:
					{
						_ULONG npos;

#ifdef __WIN32__
						npos = (_ULONG)HIWORD(wParam);
#else
						npos = (_ULONG)LOWORD(lParam);
#endif
						amount = (_LONG) npos - wr->wr_show.xx;
					}
					what = WA_HSLIDE;
					break;
				}
			} else
			{
				switch (LOWORD(wParam))
				{
				case SB_TOP:
					Wind_Arrow(wr, WA_HSLIDE, -wr->wr_show.xx, &scroll, update_area);
					amount = -(wr->wr_show.yy);
					what = WA_VSLIDE;
					break;

				case SB_BOTTOM:
					Wind_Arrow(wr, WA_HSLIDE, -wr->wr_show.xx, &scroll, update_area);
					amount = wr->wr_show.hh - scroll.g_h - wr->wr_show.yy;
					what = WA_VSLIDE;
					break;

				case SB_LINEUP:
					what = WA_UPLINE;
					break;
				case SB_LINEDOWN:
					what = WA_DNLINE;
					break;
				case SB_PAGEUP:
					what = WA_UPPAGE;
					break;
				case SB_PAGEDOWN:
					what = WA_DNPAGE;
					break;

				case SB_THUMBTRACK:
				case SB_THUMBPOSITION:
					{
						_ULONG npos;

#ifdef __WIN32__
						npos = (_ULONG)HIWORD(wParam);
#else
						npos = (_ULONG)LOWORD(lParam);
#endif
						amount = npos - wr->wr_show.yy;
					}
					what = WA_VSLIDE;
					break;
				}
			}

			if (what < 0)
				return 0;
			Wind_Arrow(wr, what, amount, &scroll, update_area);
		}
		return 0;

	case WM_KEYDOWN:
		return 0;

	case WM_SETFOCUS:
		{
			WINDOW_DEF *wr = getWiPtr(hwnd);

			if (wr != NULL)
			{
				topped_wind(wr);
			}
		}
		break;

	case WM_KILLFOCUS:
		{
			WINDOW_DEF *wr = getWiPtr(hwnd);

			if (wr != NULL)
			{
				untopped_wind(wr);
			}
		}
		break;

	case WM_MOUSEACTIVATE:
		{
			_UWORD hittest = LOWORD(lParam);
			WINDOW_DEF *wr = getWiPtr(hwnd);

			if (wr != NULL)
			{
				Wind_On_Top(wr);
				if (hittest == HTCLIENT && HIWORD(lParam) == WM_LBUTTONDOWN)
				{
					if (wr->wr_state & WR_NOTOP)
						return MA_NOACTIVATE;
					return MA_ACTIVATEANDEAT;
				}
			}
		}
		break;

	case WM_ACTIVATE:
		break;

	case WM_MDIACTIVATE:
		break;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
		hwnd = hndl_button(hwnd, message, lParam);
		if (hwnd == NO_WINDOW)
			return 0;
		break;

	case WM_MOUSEMOVE:
		/* handle_move(hwnd, lParam); */
		break;

	case WM_MOVE:
	case WM_SIZE:
		if (hwnd != MdiClientHwnd && hwnd != GlMainHwnd)
		{
			WINDOW_DEF *wr;

			wr = getWiPtr(hwnd);
			if (wr != NULL)
			{
				get_current_pos(wr, hwnd);
				if (message == WM_SIZE)
				{
					switch (wParam)
					{
					case SIZE_MAXIMIZED:
						wr->wr_state |= WR_FULLED;
						wr->wr_state &= ~WR_ICONIFIED;
						break;
					case SIZE_MINIMIZED:
						if (!(wr->wr_state & WR_ICONIFIED))
							iconify_window(wr);
						wr->wr_state |= WR_ICONIFIED;
						break;
					case SIZE_RESTORED:
						if (wr->wr_state & WR_ICONIFIED)
							uniconify_window(wr);
						wr->wr_state &= ~(WR_FULLED|WR_ICONIFIED);
						break;
					}
					get_work(wr);
					wr->scrollvsize = -1;
					wr->scrollhsize = -1;
					Wind_Set_Sliders(wr, HORIZONTAL | VERTICAL, SLSIZE);
				} else
				{
					Wind_Set_Sliders(wr, HORIZONTAL | VERTICAL, SLPOS);
					get_work(wr);
				}
			}
		}
		break;

	case WM_GETMINMAXINFO:
		break;
	
	case WM_CLOSE:
		{
			WINDOW_DEF *wr = getWiPtr(hwnd);

			if (wr != NULL)
			{
				if (Wind_Close(wr) == PROGRAM_EXIT)
					close_mainwind();
			}
		}
		return 0;

	case WM_DESTROY:
		if (hwnd != MdiClientHwnd && hwnd != GlMainHwnd)
		{
			WINDOW_DEF *wr = getWiPtr(hwnd);

			if (ValidWin(wr))
			{
				get_current_pos(wr, hwnd);
				if (wr->wr_state & WR_ICONIFIED)
					rc_copy(&wr->lastpos, &wr->wr_currpos);
				rc_copy(&wr->wr_currpos, &wr->wr_open_size);
				if (hwnd == GetCapture())
				{
					ReleaseCapture();
				}
				wr->hwnd = NO_WINDOW;
				setWiPtr(hwnd, NULL);
			}
		}
		break;
	}

	if (GetParent(hwnd) == MdiClientHwnd)
		return DefMDIChildProc(hwnd, message, wParam, lParam);
	return DefWindowProc(hwnd, message, wParam, lParam);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID Wind_Zero(WINDOW_DEF *wr)
{
	wr->wr_show.xx = 0;
	wr->wr_show.yy = 0;
	wr->wr_show.ww = 0;
	wr->wr_show.hh = 0;
	W_GetFontSize(wr, FONT_SIZE_NORMAL, &wr->wr_xfac, &wr->wr_yfac);
	wr->wr_xunits = 1;
	wr->wr_yunits = 1;
	wr->wr_buf = NULL;
	wr->wr_title[0] = '\0';
	wr->scrollhsize = -1;
	wr->scrollvsize = -1;
	wr->scrollhpos = 0;
	wr->scrollvpos = 0;
}

/*** ---------------------------------------------------------------------- ***/

WINDOW_DEF *Wind_Open(_WORD type, _UWORD flags, WINDOW_PROC windowProc, _UBYTE *title, _VOID *buf)
{
	HWND hwnd;
	GRECT gr;
	DWORD wiStyle = WS_CLIPCHILDREN;
	WINDOW_DEF *wr;
	LOCAL _WORD groupCount = 1000;

	if (type <= 0)
	{
		type = groupCount;
		groupCount++;
	}

	FOR_ALL_WINDS(wr)
	{
		if (wr->wr_type == type)
		{
			if (wr->hwnd == NO_WINDOW)
				break;
		}
	}

	if (wr == NULL)
	{
		if ((wr = NEW(WINDOW_DEF, "Wind_Create")) == NULL)
			return NULL;
		MemSetZeroStruct(*wr);
		wr->wr_next = window_list;
		window_list = wr;
	}

	Wind_Zero(wr);
	wr->hwnd = NO_WINDOW;
	wr->wr_type = type;
	wr->wr_flags = flags;
	wr->wr_state = WR_NONE;
	wr->wr_proc = windowProc;
	wr->wr_buf = buf;
	
#define GR_USEDEFAULT -32767
	
	if (title != NULL && title[0] != '\0')
	{
		strBcpy(wr->wr_title, title);
		wr->wr_flags |= WAT_NAME;
	}

	alluniconify();

	if (wr->wr_flags & WAT_VERSLIDE)
		wiStyle |= WS_VSCROLL;
	if (wr->wr_flags & WAT_HORSLIDE)
		wiStyle |= WS_HSCROLL;
	if (wr->wr_flags & WAT_FULLER)
		wiStyle |= WS_MAXIMIZEBOX;
	if (wr->wr_flags & WAT_SMALLER)
		wiStyle |= WS_MINIMIZEBOX;
	if (wr->wr_flags & WAT_SIZER)
		wiStyle |= WS_THICKFRAME;
	else
		wiStyle |= WS_BORDER;
	if (wr->wr_flags & WAT_NAME)
		wiStyle |= WS_SYSMENU | WS_CAPTION;

	rc_copy(&wr->wr_open_size, &gr);
#if 0
	{
	RECT rect;
	GrectToRect(&rect, &gr);
	AdjustWindowRect(&rect, wiStyle, FALSE);
	RectToGrect(&gr, &rect);
	}
#endif
	if (wr->wr_open_size.g_x == 0)
		gr.g_x = GR_USEDEFAULT; /* CW_USEDEFAULT */
	if (wr->wr_open_size.g_y == 0)
		gr.g_y = GR_USEDEFAULT; /* CW_USEDEFAULT */
	if (wr->wr_open_size.g_w == 0)
		gr.g_w = GR_USEDEFAULT; /* CW_USEDEFAULT */
	if (wr->wr_open_size.g_h == 0)
		gr.g_h = GR_USEDEFAULT; /* CW_USEDEFAULT */

	if (gr.g_x < 0 && gr.g_y < 0 && gr.g_x != GR_USEDEFAULT)
	{
		gr.g_x = gr.g_y = 0;
		gr.g_w = gr.g_h = GR_USEDEFAULT; /* CW_USEDEFAULT */
		wiStyle |= WS_MAXIMIZE;
	}

	if (gr.g_w < WI_MIN_SIZE || gr.g_h < WI_MIN_SIZE)
	{
		gr.g_w = gr.g_h = GR_USEDEFAULT; /* CW_USEDEFAULT */
	}

	{
		MDICREATESTRUCT mdiCreate;
		
		mdiCreate.szClass = WCN_MDI_Client;
		mdiCreate.szTitle = wr->wr_title;
		mdiCreate.hOwner = GetInstance();
		mdiCreate.style = wiStyle | WS_CHILD | WS_CLIPSIBLINGS;
		mdiCreate.x = gr.g_x == GR_USEDEFAULT ? CW_USEDEFAULT : gr.g_x;
		mdiCreate.y = gr.g_y == GR_USEDEFAULT ? CW_USEDEFAULT : gr.g_y;
		mdiCreate.cx = gr.g_w == GR_USEDEFAULT ? CW_USEDEFAULT : gr.g_w;
		mdiCreate.cy = gr.g_h == GR_USEDEFAULT ? CW_USEDEFAULT : gr.g_h;
		mdiCreate.lParam = (_LONG) wr;

		if (MdiClientHwnd != NO_WINDOW && !(wr->wr_state & WR_OVERLAPPED))
		{
			hwnd = (HWND) SendMessage(MdiClientHwnd, WM_MDICREATE, 0, (LPARAM) &mdiCreate);
		} else
		{
			CREATESTRUCT cr;
			
			cr.hInstance = GetInstance();
			cr.hMenu = NULL;
			cr.hwndParent = GlMainHwnd;
			cr.cx = mdiCreate.cx;
			cr.cy = mdiCreate.cy;
			cr.x = mdiCreate.x;
			cr.y = mdiCreate.y;
			cr.style = wiStyle | WS_OVERLAPPED | WS_POPUP;
			cr.lpszName = mdiCreate.szTitle;
			cr.lpszClass = mdiCreate.szClass;
			cr.dwExStyle = 0;
			cr.lpCreateParams = wr;
			hwnd = CreateWindow(
				cr.lpszClass,
				cr.lpszName,
				cr.style,
				cr.x, cr.y, cr.cx, cr.cy,
				cr.hwndParent,
				cr.hMenu,
				cr.hInstance,
				cr.lpCreateParams);
		}

		if (hwnd == NO_WINDOW)
		{
			abbruch(MSG_NO_WINDOW);

			Wind_Zero(wr);
			return NULL;
		}
	}
	wr->hwnd = hwnd;

	if (GetParent(hwnd) == MdiClientHwnd && !(wiStyle & WS_MAXIMIZE))
	{
		RECT pos;
		RECT client;
		_WORD move = 0;
		
		GetClientRect(MdiClientHwnd, &client);
		GetWindowRect(hwnd, &pos);
		pos.right -= pos.left;
		pos.bottom -= pos.top;
		pos.left = pos.top = 0;
		MapWindowPoints(hwnd, MdiClientHwnd, (POINT *)&pos, 2);
		if (pos.right > client.right)
		{
			move = pos.right - client.right;
			pos.left -= move;
			pos.right -= move;
		}
		if (pos.left < 0)
		{
			move = pos.left;
			pos.left = 0;
			pos.right -= move;
		}
		if (pos.bottom > client.bottom)
		{
			move = pos.bottom - client.bottom;
			pos.top -= move;
			pos.bottom -= move;
		}
		if (pos.top < 0)
		{
			move = pos.top;
			pos.top = 0;
			pos.bottom -= move;
		}
		if (move != 0)
		{
			MoveWindow(hwnd, pos.left, pos.top, pos.right - pos.left, pos.bottom - pos.top, IsWindowVisible(hwnd));
		}
	}
	ShowWindow(hwnd, wiStyle & WS_MAXIMIZE ? SW_SHOWMAXIMIZED : SW_RESTORE);
	SetFocus(hwnd);

	if (hwnd != NO_WINDOW)
	{
		WINDOW_DEF *top_window;

		top_window = Wind_Top();
		put_on_top(wr);
		if (top_window != wr)
			untopped_wind(top_window);
		/*
		 * WR_OPENING is set while calling the wr_open() function
		 * before the wind_open() is called, in case the function
		 * calls Wind_Close() to abort on error
		 */
		wr->wr_state |= WR_ONTOP | WR_OPENING;
		if (!wr->wr_proc(WMY_OPEN, wr, wr->wr_buf))
		{
			Wind_Close(wr);
			return NULL;
		}
		windowProc(WMY_TOP, wr, buf);
		get_work(wr);
		Wind_Set_Sliders(wr, HORIZONTAL | VERTICAL, SLPOS | SLSIZE);
		get_work(wr);
		wr->wr_state &= ~WR_OPENING;
	}
	return wr;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID destroy_window(HWND hwnd)
{
	if (GetParent(hwnd) == MdiClientHwnd)
		SendMessage(MdiClientHwnd, WM_MDIDESTROY, (WPARAM) hwnd, 0);
	else
		DestroyWindow(hwnd);
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL Wind_Close(WINDOW_DEF *wr)
{
	_BOOL retV;
	HWND wh;

	if (wr == NULL)
		return TRUE;

	wh = wr->hwnd;

	if (wh == NO_WINDOW)
	{
		/* moeglicherweise schon von callback auf NO_WINDOW gesetzt */
		return TRUE;
	}

	retV = wr->wr_proc(WMY_CLOSE, wr, wr->wr_buf);
	if (retV == FALSE)
		return FALSE;

	if (GetCapture() != NO_WINDOW)
	{
		ReleaseCapture();
	}

	if (wr->hwnd != NO_WINDOW)
	{
		destroy_window(wh);
		wr->hwnd = NO_WINDOW;
		wr->wr_state &= ~WR_ONTOP;
		/* don't put on top, this would change the list when closing all windows */
		/* put_on_top(wr); */
	}
	return retV;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID *Wind_Buf_Ptr(WINDOW_DEF *wr)
{
	if (ValidWin(wr))
		return wr->wr_buf;
	return NULL;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL WINDOW_PROC Wind_Proc_Ptr(WINDOW_DEF *wr)
{
	if (ValidWin(wr))
		return wr->wr_proc;
	return NULL;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Wind_On_Top(WINDOW_DEF *wr)
{
	if (ValidWin(wr))
	{
		if (GetParent(wr->hwnd) == MdiClientHwnd)
			SendMessage(MdiClientHwnd, WM_MDIACTIVATE, (WPARAM) wr->hwnd, 0L);
		else
			BringWindowToTop(wr->hwnd);
	}
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Wind_On_Bottom(WINDOW_DEF *wr)
{
	if (ValidWin(wr))
	{
		if (GetParent(wr->hwnd) == MdiClientHwnd)
			SendMessage(MdiClientHwnd, WM_MDINEXT, (WPARAM) wr->hwnd, 0L);
		else
			SetWindowPos(wr->hwnd, HWND_DESKTOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
	}
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Wind_SetDoc(WINDOW_DEF *window, CONST L_GRECT *show)
{
	if (ValidWin(window))
	{
		window->wr_show = *show;
		Wind_Set_Sliders(window, HORIZONTAL | VERTICAL, SLPOS | SLSIZE);
	}
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Wind_GetDoc(WINDOW_DEF *window, L_GRECT *show)
{
	if (ValidWin(window))
	{
		*show = window->wr_show;
	}
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Wind_SetFac(WINDOW_DEF *window, _WORD xfac, _WORD yfac, _WORD xunits, _WORD yunits)
{
	if (xfac > 0)
		window->wr_xfac = xfac;
	if (yfac > 0)
		window->wr_yfac = yfac;
	if (xunits > 0)
		window->wr_xunits = xunits;
	if (yunits > 0)
		window->wr_yunits = yunits;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Wind_GetFac(WINDOW_DEF *window, _WORD *xfac, _WORD *yfac, _WORD *xunits, _WORD *yunits)
{
	if (xfac != NULL)
		*xfac = window->wr_xfac;
	if (yfac != NULL)
		*yfac = window->wr_yfac;
	if (xunits != NULL)
		*xunits = window->wr_xunits;
	if (yunits != NULL)
		*yunits = window->wr_yunits;
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

LOCAL LRESULT CALLBACK my_mdiclient_proc(HWND hwnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		{
			LRESULT result;
			
			result = mdiclient_proc(hwnd, message, wParam, lParam);
			MdiClientHwnd = NO_WINDOW;
			SetWindowLong(hwnd, GWL_WNDPROC, (LPARAM)mdiclient_proc);
			mdiclient_proc = FUNK_NULL;
			return result;
		}
	}
	return mdiclient_proc(hwnd, message, wParam, lParam);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL BOOL WINAPI closeEnumProc(HWND hwnd, LPARAM lParam)
{
	UNUSED(lParam);

	if (GetWindow(hwnd, GW_OWNER))
		return 1;

	if (!SendMessage(hwnd, WM_QUERYENDSESSION, 0, 0))
		return 1;
	destroy_window(hwnd);
	return 1;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID wind_exit(_VOID)
{
	/* CloseWindow( GlMainHwnd ); */
	if (GlMainHwnd != NO_WINDOW)
	{
		if (!DestroyWindow(GlMainHwnd))
			ErrorOut(EO_ERROR, "Destroy Main Window");
		GlMainHwnd = NO_WINDOW;
	}
	if (GlMenuHandle != NO_MENU)
	{
		DestroyMenu(GlMenuHandle);
		GlMenuHandle = NO_MENU;
	}
	if (GlAccelTable != NO_ACCEL)
	{
		/* DestroyAcceleratorTable(GlAccelTable); */
		GlAccelTable = NO_ACCEL;
	}
	if (!GetPrevInstance())
		register_classes(FALSE);
	Wind_Close_All(TRUE);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL LRESULT CALLBACK mainWndProc(HWND hwnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_KEYDOWN:
	case WM_CHAR:
	case WM_DEADCHAR:
	case WM_SYSCHAR:
	case WM_SYSDEADCHAR:
		break;

	case WM_MOVE:
	case WM_SIZE:
		if (message == WM_SIZE)
		{
			switch (wParam)
			{
			case SIZE_MAXIMIZED:
				GlCmdShow = SW_SHOWMAXIMIZED;
				break;
			case SIZE_MINIMIZED:
				alliconify();
				GlCmdShow = SW_SHOWMINIMIZED;
				break;
			case SIZE_RESTORED:
				alluniconify();
				GlCmdShow = SW_SHOW;
				break;
			}
			if (!all_iconified)
			{
				win_do_resize(hwnd);
				/* Wind_Set_Sliders(wr, HORIZONTAL | VERTICAL, SLSIZE); */
				return 0;
			}
		}
		break;

	case WM_CREATE:
		{
			PAINTSTRUCT ps;

			BeginPaint(hwnd, &ps);
			EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_ERASEBKGND:
		return 0;

	case WM_PAINT:
		break;

	case WM_MOUSEMOVE:
		/* handle_move(hwnd, lParam); */
		break;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
#if 0
		hwnd = hndl_button(hwnd, message, lParam);
		if (hwnd == NO_WINDOW)
			return 0;
#endif
		break;

	case WM_DESTROY:
		GlMainHwnd = NO_WINDOW;
		setWiPtr(hwnd, NULL);
		PostQuitMessage(0);
		return 0;

	case WM_QUIT:
		return 0;
	
	case WM_COMMAND:
		if (gl_event_menu != FUNK_NULL)
		{
			_WORD index = LOWORD(wParam);

			switch (index)
			{
			case 1000:
				help_contents();
				break;
			case 1001:
				help_index();
				break;
			case 1002:
				help_using_help();
				break;
			default:
				if (!gl_event_menu(index))
					return close_mainwind();
				break;
			}
		}
		break;

	case WM_SYSCOMMAND:
		if (GlMainHwnd != NO_WINDOW)
		{
			switch (LOWORD(wParam) & 0xfff0)
			{
#if 0
			case SC_ABOUT:
				{
					FARPROC proc;
			
					proc = MakeProcInstance((FARPROC)dialog_proc, GetInstance());
					DialogBox(GetInstance(), "ABOUT", wp->hwnd, (DLGPROC)proc);
					(void)FreeProcInstance(proc);
				}
				break;
#endif

#if 0
			case SC_FONT:
				{
					CHOOSEFONT cf;
					LOGFONT lf;
					
					memset(&lf, 0, sizeof(lf));
					
					memset(&cf, 0, sizeof(cf));
					cf.lStructSize = sizeof(cf);
					cf.hwndOwner = hwnd;
					cf.hDC = NO_DC;
					cf.lpLogFont = &lf;
					cf.Flags = CF_APPLY | CF_SCREENFONTS;
					ChooseFont(&cf);
				}
				break;
#endif
			}
		}
		break;

	case WM_QUERYENDSESSION:
	case WM_CLOSE:
		if (GlMainHwnd != NO_WINDOW)
		{
			if (!Wind_Hide_Show_Windows(TRUE))
				return 0;
			{
				FARPROC lpfnEnum;

				lpfnEnum = MakeProcInstance ((FARPROC) closeEnumProc, GetInstance());
				if (MdiClientHwnd != NO_WINDOW)
					EnumChildWindows(MdiClientHwnd, (WNDENUMPROC) lpfnEnum, 0);
				if (GlMainHwnd != NO_WINDOW)
					EnumChildWindows(GlMainHwnd, (WNDENUMPROC) lpfnEnum, 0);
				(void) FreeProcInstance(lpfnEnum);
			}

			if (MdiClientHwnd == NO_WINDOW || GetWindow(MdiClientHwnd, GW_CHILD) == NO_WINDOW)
			{
				wind_exit();
			}
			return 0;
		}
		break;
	}
	if (MdiClientHwnd != NO_WINDOW)
		return DefFrameProc(hwnd, MdiClientHwnd, message, wParam, lParam);
	return DefWindowProc(hwnd, message, wParam, lParam);
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL open_main_window(_VOID)
{
	HWND hwnd;
	DWORD wiStyle = WS_CLIPCHILDREN | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME | WS_SYSMENU | WS_CAPTION;

	if (GlCmdShow == SW_SHOWMAXIMIZED)
		wiStyle |= WS_MAXIMIZE;
	else if (GlCmdShow == SW_SHOWMINIMIZED)
		wiStyle |= WS_MINIMIZE;
	
	{
		CREATESTRUCT cr;
		
		wiStyle |= WS_OVERLAPPED;
		cr.hInstance = GetInstance();
		cr.hMenu = GlMenuHandle;
		cr.hwndParent = NO_WINDOW;
		cr.x = CW_USEDEFAULT;
		cr.y = CW_USEDEFAULT;
		cr.cx = CW_USEDEFAULT;
		cr.cy = CW_USEDEFAULT;
		cr.lpszName = ProgramName;
		cr.lpszClass = WCN_Main;
		cr.style = wiStyle;
		cr.dwExStyle = 0;
		cr.lpCreateParams = NULL;
		hwnd = CreateWindow(
			cr.lpszClass,
			cr.lpszName,
			cr.style,
			cr.x, cr.y, cr.cx, cr.cy,
			cr.hwndParent,
			cr.hMenu,
			cr.hInstance,
			cr.lpCreateParams);
		if (hwnd == NO_WINDOW)
		{
			return FALSE;
		}
	}

	GlMainHwnd = hwnd;

	{
		CLIENTCREATESTRUCT clientcreate;

		clientcreate.hWindowMenu = NULL;
		clientcreate.idFirstChild = 150;		/* START_MENUE_NUM; */
		MdiClientHwnd = CreateWindow(
			"MDICLIENT", NULL,
			/* WS_VSCROLL | WS_HSCROLL | */ WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | WS_CLIPSIBLINGS | MDIS_ALLCHILDSTYLES,
			0, 40, 0, 0,
			hwnd,
			(HMENU) 1,
			GetInstance(),
			(LPSTR) &clientcreate);

		if (MdiClientHwnd == NO_WINDOW)
		{
			ErrorOut(EO_FATAL, "Create MDICLIENT window %ld", (long)GetLastError());
			return FALSE;
		}
		mdiclient_proc = (WNDPROC)GetWindowLong(MdiClientHwnd, GWL_WNDPROC);
		SetWindowLong(MdiClientHwnd, GWL_WNDPROC, (LPARAM)my_mdiclient_proc);
	}

	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL do_register_class(_BOOL do_register, WNDCLASS *wndclass, CONST _UBYTE *name)
{
	wndclass->lpszClassName = name;
	
	if (do_register)
	{
		if (RegisterClass(wndclass) == 0)
		{
			ErrorOut(EO_TRACE, "RegisterClass %s %ld", name, (long)GetLastError());
			return FALSE;
		}
	} else
	{
		if (UnregisterClass(name, wndclass->hInstance) == 0)
			ErrorOut(EO_TRACE, "UnregisterClass %s %ld", name, (long)GetLastError());
	}
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL register_classes(_BOOL do_register)
{
	WNDCLASS wndclass;
	HINSTANCE hinst;
	
	hinst = GetInstance();
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = (sizeof(_LONG) * 2);		/* Magic Number + WINDOW_DEF ptr */
	wndclass.hInstance = hinst;
#if 1
	wndclass.hIcon = LoadIcon(hinst, MAKEINTRESOURCE(1) /* "AAA_ICON_1" */ /* IDI_APPLICATION */ );
	if (wndclass.hIcon == 0)
		wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
#else
	wndclass.hIcon = 0;
#endif
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE + 1);
	wndclass.lpszMenuName = NULL;

	wndclass.style = CS_BYTEALIGNCLIENT | CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclass.lpfnWndProc = mainWndProc;
	if (!do_register_class(do_register, &wndclass, WCN_Main))
		return FALSE;
	
	wndclass.style = CS_DBLCLKS | CS_PARENTDC | CS_BYTEALIGNCLIENT | CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = useWndProc;
	wndclass.hIcon = LoadIcon(hinst, MAKEINTRESOURCE(1) /* "AAA_ICON_1" */ /* IDI_APPLICATION */ );
	if (wndclass.hIcon == 0)
		wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hbrBackground = NULL;	/* GetStockObject(WHITE_BRUSH); */
	wndclass.lpszMenuName = NULL;
	if (!do_register_class(do_register, &wndclass, WCN_MDI_Client))
		return FALSE;
	
	return TRUE;
}

/*** ---------------------------------------------------------------------- ***/

HDC W_GetDC(_VOID *wr)
{
	return ((WINDOW_DEF *)wr)->hDC;
}

/*** ---------------------------------------------------------------------- ***/

HWND W_GetHwnd(_VOID *wr)
{
	return ((WINDOW_DEF *)wr)->hwnd;
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID GetMouseState(_WORD *gr_mkmx, _WORD *gr_mkmy, _UWORD *gr_mkmstate)
{
	POINT   point;

	GetCursorPos(&point);
	*gr_mkmx = point.x;
	*gr_mkmy = point.y;

	*gr_mkmstate = 0;
	if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
		*gr_mkmstate += MOB_LEFT;
	if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
		*gr_mkmstate += MOB_RIGHT;
	if (GetAsyncKeyState(VK_MBUTTON) & 0x8000)
		*gr_mkmstate += MOB_MIDDLE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _UWORD GetKeyboardStates (_VOID)
{
	_UWORD  retV = 0;

	if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
		retV += K_SHIFT;
	if (GetAsyncKeyState(VK_CAPITAL) & 0x8000)
		retV += K_CAPSLOCK;
	if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
		retV += K_CTRL;
	if (GetAsyncKeyState(VK_MENU) & 0x8000)
		retV += K_ALT;
	return retV;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL WindowsTasteToAtari(_WORD *taste, _UWORD *keystate, unsigned int message, WPARAM wParam, LPARAM lParam)
{
	_BOOL alt, ctrl, shift;
	UNUSED(lParam);
	
	alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
	ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
	shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
	
	*keystate = 0;
	if (alt) *keystate |= K_ALT;
	if (ctrl) *keystate |= K_CTRL;
	if (shift) *keystate |= K_SHIFT;
	
#if 0
	switch (message)
	{
		case WM_SYSDEADCHAR: ErrorOut(EO_TRACE, "WM_SYSDEADCHAR $%04x, $%08lx, alt=%d, ctrl=%d, shift=%d", wParam, lParam, alt, ctrl, shift); break;
		case WM_SYSKEYDOWN:  ErrorOut(EO_TRACE, "WM_SYSKEYDOWN  $%04x, $%08lx, alt=%d, ctrl=%d, shift=%d", wParam, lParam, alt, ctrl, shift); break;
		case WM_KEYDOWN:     ErrorOut(EO_TRACE, "WM_KEYDOWN     $%04x, $%08lx, alt=%d, ctrl=%d, shift=%d", wParam, lParam, alt, ctrl, shift); break;
		case WM_DEADCHAR:    ErrorOut(EO_TRACE, "WM_DEADCHAR    $%04x, $%08lx, alt=%d, ctrl=%d, shift=%d", wParam, lParam, alt, ctrl, shift); break;
		case WM_SYSCHAR:     ErrorOut(EO_TRACE, "WM_SYSCHAR     $%04x, $%08lx, alt=%d, ctrl=%d, shift=%d", wParam, lParam, alt, ctrl, shift); break;
		case WM_CHAR:        ErrorOut(EO_TRACE, "WM_CHAR        $%04x, $%08lx, alt=%d, ctrl=%d, shift=%d", wParam, lParam, alt, ctrl, shift); break;
	}
#endif
	switch (message)
	{
	case WM_SYSKEYDOWN:
		switch (wParam)
		{
			case VK_F10: *taste = shift ? SHFT_F10 : F10; return TRUE;
		}
		return FALSE;
	
	case WM_SYSDEADCHAR:
	case WM_KEYDOWN:
		if (alt) /* K_ALT */
		{
			switch (wParam)
			{
				case '0': *taste = ALT_0; break;
				case '1': *taste = ALT_1; break;
				case '2': *taste = ALT_2; break;
				case '3': *taste = ALT_3; break;
				case '4': *taste = ALT_4; break;
				case '5': *taste = ALT_5; break;
				case '6': *taste = ALT_6; break;
				case '7': *taste = ALT_7; break;
				case '8': *taste = ALT_8; break;
				case '9': *taste = ALT_9; break;
				case 'A': *taste = ALT_A; break;
				case 'B': *taste = ALT_B; break;
				case 'C': *taste = ALT_C; break;
				case 'D': *taste = ALT_D; break;
				case 'E': *taste = ALT_E; break;
				case 'F': *taste = ALT_F; break;
				case 'G': *taste = ALT_G; break;
				case 'H': *taste = ALT_H; break;
				case 'I': *taste = ALT_I; break;
				case 'J': *taste = ALT_J; break;
				case 'K': *taste = ALT_K; break;
				case 'L': *taste = ALT_L; break;
				case 'M': *taste = ALT_M; break;
				case 'N': *taste = ALT_N; break;
				case 'O': *taste = ALT_O; break;
				case 'P': *taste = ALT_P; break;
				case 'Q': *taste = ALT_Q; break;
				case 'R': *taste = ALT_R; break;
				case 'S': *taste = ALT_S; break;
				case 'T': *taste = ALT_T; break;
				case 'U': *taste = ALT_U; break;
				case 'V': *taste = ALT_V; break;
				case 'W': *taste = ALT_W; break;
				case 'X': *taste = ALT_X; break;
				case 'Y': *taste = ALT_Y; break;
				case 'Z': *taste = ALT_Z; break;
				default: return FALSE;
			}
			return TRUE;
		}

		if (ctrl)
		{
			switch (wParam)
			{
				case 0xe2: *taste = TASTE_HIDE_DIALOG; break;
				case '0': *taste = CNTL_0; break;
				case '1': *taste = CNTL_1; break;
				case '2': *taste = CNTL_2; break;
				case '3': *taste = CNTL_3; break;
				case '4': *taste = CNTL_4; break;
				case '5': *taste = CNTL_5; break;
				case '6': *taste = CNTL_6; break;
				case '7': *taste = CNTL_7; break;
				case '8': *taste = CNTL_8; break;
				case '9': *taste = CNTL_9; break;
				case 'A': *taste = CNTL_A; break;
				case 'B': *taste = CNTL_B; break;
				case 'C': *taste = CNTL_C; break;
				case 'D': *taste = CNTL_D; break;
				case 'E': *taste = CNTL_E; break;
				case 'F': *taste = CNTL_F; break;
				case 'G': *taste = CNTL_G; break;
				case 'H': *taste = CNTL_H; break;
				case 'I': *taste = CNTL_I; break;
				case 'J': *taste = CNTL_J; break;
				case 'K': *taste = CNTL_K; break;
				case 'L': *taste = CNTL_L; break;
				case 'M': *taste = CNTL_M; break;
				case 'N': *taste = CNTL_N; break;
				case 'O': *taste = CNTL_O; break;
				case 'P': *taste = CNTL_P; break;
				case 'Q': *taste = CNTL_Q; break;
				case 'R': *taste = CNTL_R; break;
				case 'S': *taste = CNTL_S; break;
				case 'T': *taste = CNTL_T; break;
				case 'U': *taste = CNTL_U; break;
				case 'V': *taste = CNTL_V; break;
				case 'W': *taste = CNTL_W; break;
				case 'X': *taste = CNTL_X; break;
				case 'Y': *taste = CNTL_Y; break;
				case 'Z': *taste = CNTL_Z; break;
	
				case VK_F12: *taste = UNDO; break;
	
				case 0xBB:
				case VK_ADD: *taste = CNTL_NUM_PLUS; break;
				case 0xBD:
				case VK_SUBTRACT: *taste = CNTL_NUM_MINUS; break;
	
				case VK_LEFT: *taste = CNTL_CUR_LEFT; break;
				case VK_RIGHT: *taste = CNTL_CUR_RIGHT; break;
	
				case VK_RETURN: *taste = RETURN; break;
	
				case VK_HOME: *taste = CNTL_HOME; break;
				case VK_END: *taste = CNTL_END; break;
	
				default: return FALSE;
			}
			return TRUE;
		}

		if (shift)
		{
			switch (wParam)
			{
				case '0': *taste = SHFT_0; break;
				case '1': *taste = SHFT_1; break;
				case '2': *taste = SHFT_2; break;
				case '3': *taste = SHFT_3; break;
				case '4': *taste = SHFT_4; break;
				case '5': *taste = SHFT_5; break;
				case '6': *taste = SHFT_6; break;
				case '7': *taste = SHFT_7; break;
				case '8': *taste = SHFT_8; break;
				case '9': *taste = SHFT_9; break;
	
				case VK_BACK: *taste = BACKSPACE; break;
				case VK_TAB: *taste = BTAB; break;
				case VK_ESCAPE: *taste = ESC; break;
				case VK_RETURN: *taste = RETURN; break;
				case VK_HOME: *taste = SHFT_HOME; break;
				case VK_LEFT: *taste = SHFT_CL; break;
				case VK_UP: *taste = SHFT_CU; break;
				case VK_RIGHT: *taste = SHFT_CR; break;
				case VK_DOWN: *taste = SHFT_CD; break;
				case VK_INSERT: *taste = SHFT_INS; break;
				case VK_DELETE: *taste = ZEI_DELETE; break;
				case VK_HELP: *taste = HELP; break;
	
				case VK_F1: *taste = SHFT_F1; break;
				case VK_F2: *taste = SHFT_F2; break;
				case VK_F3: *taste = SHFT_F3; break;
				case VK_F4: *taste = SHFT_F4; break;
				case VK_F5: *taste = SHFT_F5; break;
				case VK_F6: *taste = SHFT_F6; break;
				case VK_F7: *taste = SHFT_F7; break;
				case VK_F8: *taste = SHFT_F8; break;
				case VK_F9: *taste = SHFT_F9; break;
				case VK_F10: *taste = SHFT_F10; break;
				case VK_F12: *taste = HELP; break;
	
				default: return FALSE;
			}
			return TRUE;
		}

		switch (wParam)
		{
			case VK_BACK: *taste = BACKSPACE; break;
			case VK_TAB: *taste = TAB; break;
			case VK_ESCAPE: *taste = ESC; break;
			case VK_RETURN: *taste = RETURN; break;
			case VK_HOME: *taste = HOME; break;
			case VK_LEFT: *taste = CUR_LEFT; break;
			case VK_UP: *taste = CUR_UP; break;
			case VK_RIGHT: *taste = CUR_RIGHT; break;
			case VK_DOWN: *taste = CUR_DOWN; break;
			case VK_INSERT: *taste = INSERT; break;
			case VK_DELETE: *taste = ZEI_DELETE; break;
			case VK_HELP: *taste = HELP; break;
			case VK_END: *taste = END; break;
			case VK_PRIOR: *taste = PAGE_UP; break;
			case VK_NEXT: *taste = PAGE_DOWN; break;
	
			case VK_F1: *taste = F1; break;
			case VK_F2: *taste = F2; break;
			case VK_F3: *taste = F3; break;
			case VK_F4: *taste = F4; break;
			case VK_F5: *taste = F5; break;
			case VK_F6: *taste = F6; break;
			case VK_F7: *taste = F7; break;
			case VK_F8: *taste = F8; break;
			case VK_F9: *taste = F9; break;
			case VK_F10: *taste = F10; break;
			case VK_F12: *taste = HELP; break;
	
			default: return FALSE;
		}
		return TRUE;

	case WM_CHAR:
	case WM_DEADCHAR:
		switch (wParam)
		{
			case 0xF6: *taste = ZEI_oe; break;
			case 0xD6: *taste = ZEI_OE; break;
			case 0xE4: *taste = ZEI_ae; break;
			case 0xC4: *taste = ZEI_AE; break;
			case 0xFC: *taste = ZEI_ue; break;
			case 0xDC: *taste = ZEI_UE; break;
			case 0xDF: *taste = ZEI_sz; break;
	
			case '0': *taste = ZEI_0; break;
			case '1': *taste = ZEI_1; break;
			case '2': *taste = ZEI_2; break;
			case '3': *taste = ZEI_3; break;
			case '4': *taste = ZEI_4; break;
			case '5': *taste = ZEI_5; break;
			case '6': *taste = ZEI_6; break;
			case '7': *taste = ZEI_7; break;
			case '8': *taste = ZEI_8; break;
			case '9': *taste = ZEI_9; break;
	
			case 'a': *taste = ZEI_a; break;
			case 'b': *taste = ZEI_b; break;
			case 'c': *taste = ZEI_c; break;
			case 'd': *taste = ZEI_d; break;
			case 'e': *taste = ZEI_e; break;
			case 'f': *taste = ZEI_f; break;
			case 'g': *taste = ZEI_g; break;
			case 'h': *taste = ZEI_h; break;
			case 'i': *taste = ZEI_i; break;
			case 'j': *taste = ZEI_j; break;
			case 'k': *taste = ZEI_k; break;
			case 'l': *taste = ZEI_l; break;
			case 'm': *taste = ZEI_m; break;
			case 'n': *taste = ZEI_n; break;
			case 'o': *taste = ZEI_o; break;
			case 'p': *taste = ZEI_p; break;
			case 'q': *taste = ZEI_q; break;
			case 'r': *taste = ZEI_r; break;
			case 's': *taste = ZEI_s; break;
			case 't': *taste = ZEI_t; break;
			case 'u': *taste = ZEI_u; break;
			case 'v': *taste = ZEI_v; break;
			case 'w': *taste = ZEI_w; break;
			case 'x': *taste = ZEI_x; break;
			case 'y': *taste = ZEI_y; break;
			case 'z': *taste = ZEI_z; break;
	
			case 'A': *taste = ZEI_A; break;
			case 'B': *taste = ZEI_B; break;
			case 'C': *taste = ZEI_C; break;
			case 'D': *taste = ZEI_D; break;
			case 'E': *taste = ZEI_E; break;
			case 'F': *taste = ZEI_F; break;
			case 'G': *taste = ZEI_G; break;
			case 'H': *taste = ZEI_H; break;
			case 'I': *taste = ZEI_I; break;
			case 'J': *taste = ZEI_J; break;
			case 'K': *taste = ZEI_K; break;
			case 'L': *taste = ZEI_L; break;
			case 'M': *taste = ZEI_M; break;
			case 'N': *taste = ZEI_N; break;
			case 'O': *taste = ZEI_O; break;
			case 'P': *taste = ZEI_P; break;
			case 'Q': *taste = ZEI_Q; break;
			case 'R': *taste = ZEI_R; break;
			case 'S': *taste = ZEI_S; break;
			case 'T': *taste = ZEI_T; break;
			case 'U': *taste = ZEI_U; break;
			case 'V': *taste = ZEI_V; break;
			case 'W': *taste = ZEI_W; break;
			case 'X': *taste = ZEI_X; break;
			case 'Y': *taste = ZEI_Y; break;
			case 'Z': *taste = ZEI_Z; break;
	
			case ' ': *taste = ZEI_LEER; break;
			case '/': *taste = NUM_DURCH; break;
			case '*': *taste = NUM_MAL; break;
			case '-': *taste = NUM_MINUS; break;
			case '+': *taste = NUM_PLUS; break;
	
			case 0xB4:				/* Vorzeichen fuer Sonderzeichen */
				return FALSE;
	
			case 0xE9:				/* e acute */
				*taste = 0x82;
				break;
	
			case 0xff:
				*taste = UNDO;
				break;
			
			default:
				{
					_UBYTE buf[2];
	
					buf[0] = (_UBYTE) wParam;
					buf[1] = '\0';
					AnsiToOem(buf, buf);
					*taste = buf[0];
				}
				break;
		}
		return TRUE;

	default:
		break;
	}
	return FALSE;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _UBYTE scancode_to_ascii(_UWORD keycode, _UWORD kstate)
{
	_UBYTE ch = 0;

	if (kstate & K_ALT)
	{
		switch (keycode)
		{
		case ALT_A & 0xff00:
			ch = 'a';
			break;
		case ALT_B & 0xff00:
			ch = 'b';
			break;
		case ALT_C & 0xff00:
			ch = 'c';
			break;
		case ALT_D & 0xff00:
			ch = 'd';
			break;
		case ALT_E & 0xff00:
			ch = 'e';
			break;
		case ALT_F & 0xff00:
			ch = 'f';
			break;
		case ALT_G & 0xff00:
			ch = 'g';
			break;
		case ALT_H & 0xff00:
			ch = 'h';
			break;
		case ALT_I & 0xff00:
			ch = 'i';
			break;
		case ALT_J & 0xff00:
			ch = 'j';
			break;
		case ALT_K & 0xff00:
			ch = 'k';
			break;
		case ALT_L & 0xff00:
			ch = 'l';
			break;
		case ALT_M & 0xff00:
			ch = 'm';
			break;
		case ALT_N & 0xff00:
			ch = 'n';
			break;
		case ALT_O & 0xff00:
			ch = 'o';
			break;
		case ALT_P & 0xff00:
			ch = 'p';
			break;
		case ALT_Q & 0xff00:
			ch = '@';
			break;
		case ALT_R & 0xff00:
			ch = 'r';
			break;
		case ALT_S & 0xff00:
			ch = 's';
			break;
		case ALT_T & 0xff00:
			ch = 't';
			break;
		case ALT_U & 0xff00:
			ch = 'u';
			break;
		case ALT_V & 0xff00:
			ch = 'v';
			break;
		case ALT_W & 0xff00:
			ch = 'w';
			break;
		case ALT_X & 0xff00:
			ch = 'x';
			break;
		case ALT_Y & 0xff00:
			ch = 'y';
			break;
		case ALT_Z & 0xff00:
			ch = 'z';
			break;
		case ALT_AE & 0xff00:
			ch = 0x84;
			break;
		case ALT_OE & 0xff00:
			ch = 0x94;
			break;
		case ALT_UE & 0xff00:
			ch = 0x81;
			break;
		case ALT_SZ & 0xff00:
			ch = 0x9e;
			break;
		case ALT_1 & 0xff00:
			ch = '1';
			break;
		case ALT_2 & 0xff00:
			ch = '2';
			break;
		case ALT_3 & 0xff00:
			ch = '3';
			break;
		case ALT_4 & 0xff00:
			ch = '4';
			break;
		case ALT_5 & 0xff00:
			ch = '5';
			break;
		case ALT_6 & 0xff00:
			ch = '6';
			break;
		case ALT_7 & 0xff00:
			ch = '{';
			break;
		case ALT_8 & 0xff00:
			ch = '[';
			break;
		case ALT_9 & 0xff00:
			ch = ']';
			break;
		case ALT_0 & 0xff00:
			ch = '}';
			break;
		case ALT_HOCH:
			ch = '\'';
			break;
		}
		if (kstate & K_SHIFT)
			ch = ucase(ch);
	}
	return ch;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _VOID mk_decode(MKINFO *mk, _WORD keycode, _WORD keystate)
{
	mk->key = keycode;
	mk->state = keystate;
	GetMouseState(&mk->mouse_x, &mk->mouse_y, &mk->button2);
	mk->state = keystate = GetKeyboardStates();
	mk->xx = mk->mouse_x;
	mk->yy = mk->mouse_y;
	mk->shift = keystate & K_SHIFT;
	mk->ctrl = keystate & K_CTRL;
	mk->alt = keystate & K_ALT;
	mk->ascii_code = keycode & 0xff;
	mk->scan_code = (keycode >> 8) & 0xff;
	if (mk->ascii_code == 0)
	{
		mk->ascii_code = scancode_to_ascii(keycode, keystate);
	}
	mk->clicks = 0;
	mk->button = 0;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL Wind_Std_Key(WINDOW_DEF *wr, MKINFO *mk, _BOOL *quit)
{
	_BOOL ok = TRUE;

	if (!ValidWin(wr))
		return FALSE;

	switch (mk->key)
	{
	case CNTL_HOME:
		do_arrow(wr, WA_VSLIDE, -wr->wr_show.yy);
		break;
	case HOME:
		do_arrow(wr, WA_HSLIDE, -wr->wr_show.xx);
		break;
	case CNTL_END:
		do_arrow(wr, WA_VSLIDE, wr->wr_show.hh - wr->wr_show.yy);
		break;
	case END:
		do_arrow(wr, WA_HSLIDE, wr->wr_show.ww - wr->wr_show.xx);
		break;

	case PAGE_UP:
		do_arrow(wr, WA_UPPAGE, 1);
		break;
	case PAGE_DOWN:
		do_arrow(wr, WA_DNPAGE, 1);
		break;
	case CUR_UP:
		do_arrow(wr, WA_UPLINE, 1);
		break;
	case CUR_DOWN:
		do_arrow(wr, WA_DNLINE, 1);
		break;

	case CUR_LEFT:
		do_arrow(wr, WA_LFLINE, 1);
		break;
	case CUR_RIGHT:
		do_arrow(wr, WA_RTLINE, 1);
		break;

	case CNTL_U:
		if (Wind_Close(wr) == PROGRAM_EXIT)
			*quit = TRUE;
		break;
	case CNTL_W:
		Wind_Next_Top();
		break;

	default:
		ok = FALSE;
		break;
	}
	return ok;
}

/*** ---------------------------------------------------------------------- ***/

LOCAL _BOOL hndl_keybd(HWND hwnd, MKINFO *mk, _BOOL *found)
{
	WINDOW_DEF *wr;
	_BOOL ok;
	_BOOL quit;

	ok = FALSE;
	quit = FALSE;
	if ((wr = getWiPtr(hwnd)) != NULL)
	{
		quit = !wr->wr_proc(WMY_KEY, wr, mk);
		ok = mk->key == 0;
		if (ok == FALSE)
			ok = Wind_Std_Key(wr, mk, &quit);
	}
	*found = ok;
	return quit;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _VOID Evnt_Multi(MENU_FUNC fu_event_menu, _WORD menue_id)
{
	MSG MainMsg;
	_BOOL quit = FALSE;
	_BOOL found;
	
	gl_event_menu = fu_event_menu;
	
	if (menue_id != 0)
	{
		GlMenuHandle = LoadMenu(GetInstance(), MAKEINTRESOURCE(menue_id));
		if (GlMenuHandle == NO_MENU)
			return;
		GlAccelTable = LoadAccelerators(GetInstance(), MAKEINTRESOURCE(menue_id));
	}
	if (!open_main_window())
		return;
	Wind_Hide_Show_Windows(FALSE);

	MainMsg.message = 0;
	while (!quit)
	{
		if (!GetMessage(&MainMsg, NULL, 0, 0))
			break;

		found = FALSE;

		switch (MainMsg.message)
		{
		case WM_TIMER:
			break;

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		case WM_CHAR:
		case WM_DEADCHAR:
		case WM_SYSCHAR:
		case WM_SYSDEADCHAR:
			{
				_WORD taste;
				_UWORD kstate;
				
				if (GetCapture() != NO_WINDOW)
				{
					ReleaseCapture();
				}

				if (WindowsTasteToAtari(&taste, &kstate, MainMsg.message, MainMsg.wParam, MainMsg.lParam))
				{
					MKINFO mk;

					mk_decode(&mk, taste, kstate);

					quit |= hndl_keybd(MainMsg.hwnd, &mk, &found);
				}
			}
			break;
		}

		if (!found)
		{
			if (MdiClientHwnd == NO_WINDOW || !TranslateMDISysAccel(MdiClientHwnd, &MainMsg))
			{
				if (GlAccelTable == NO_ACCEL || !TranslateAccelerator(GlMainHwnd, GlAccelTable, &MainMsg))
				{
					TranslateMessage(&MainMsg);
					DispatchMessage(&MainMsg);
				}
			}
		}

		if (quit)
			close_mainwind();
	}
}

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

#include <path_max.h>
#include <file_io.h>

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL Wind_Pexec(_UBYTE *name, _UBYTE *para)
{
	_UBYTE  oldPath[PATH_MAX];
	_BOOL   retV = TRUE;
	_UBYTE *buf;
	size_t len;
	
	F_Path_Get(oldPath);
	{
		_UBYTE path[PATH_MAX];

		strBcpy(path, para);
		F_Path_Dirname(path);
		F_Path_Set(path);
	}
	
	len = strlen(name) + 1;
	if (para != NULL && *para != '\0')
		len += 1 + strlen(para);
	buf = MALLOC(len, "Wind_Pexec");
	if (buf == NULL)
		return FALSE;
	strcpy(buf, name);
	if (para != NULL && *para != '\0')
	{
		strcat(buf, " ");
		strcat(buf, para);
	}
	
#if defined(__WIN32__) || defined(__CYGWIN32__)
	{
		STARTUPINFO StartupInfo;
		PROCESS_INFORMATION ProcessInformation;
		
		StartupInfo.cb = sizeof(StartupInfo);
		GetStartupInfo(&StartupInfo);
		StartupInfo.wShowWindow = SW_SHOWNORMAL;
		StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
		
		if (CreateProcess(
							  NULL, /* pointer to name of executable module */
							  buf, /* pointer to command line string */
							  NULL, /* pointer to process security attributes */
							  NULL, /* pointer to thread security attributes */
							  FALSE, /* handle inheritance flag */
							  (DWORD)NULL, /* creation flags */
							  NULL, /* pointer to new environment block */
							  NULL, /* pointer to current directory name */
							  & StartupInfo, /* pointer to STARTUPINFO */
							  & ProcessInformation /*  pointer to PROCESS_INFORMATION */
			))
		{
		} else
		{
			retV = FALSE;
		}
	}

#else /* !__WIN32__ */

	{
		UINT handle;
		
#if 1
		{
			UINT oldMode = SetErrorMode(SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS);
			handle = WinExec(buf, SW_SHOWNORMAL);
			SetErrorMode(oldMode);
		}
#else
		{
			WORD awShow[2];
			struct {
				WORD segEnv;
				LPSTR lpszCmdLine;
				LPWORD lpwShow;
				LPWORD lpwReserved;
			} loadParams;
			
			awShow[0] = 2;
			awShow[1] = SW_SHOWNORMAL;
			loadParams.segEnv = 0;
			loadParams.lpszCmdLine = (LPSTR)(para ? para : "");
			loadParams.lpwShow = awShow;
			loadParams.lpwReserved = NULL;
			handle = (UINT)LoadModule(buf, &loadParams);
			if (handle >= (UINT)HINSTANCE_ERROR)
			{
				WinExec((LPCSTR) handle, SW_SHOWNORMAL);
				FreeModule((HINSTANCE) handle);
			}
		}
#endif

		if (handle < (UINT)HINSTANCE_ERROR)
		{
			retV = FALSE;
		}
	}
#endif /* __WIN32__ */

	FREE(buf, len);
	F_Path_Set(oldPath);
	return retV;
}
