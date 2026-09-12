#ifndef __OPENWORK_H__
#define __OPENWORK_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif
#ifndef __WINDOWS__H__
#include <windows_.h>
#endif

extern int GlCmdShow;
extern HWND GlMainHwnd;
extern HWND MdiClientHwnd;

#define RectToGrect(gr, re) \
	((gr)->g_x = (re)->left, \
	 (gr)->g_y = (re)->top, \
	 (gr)->g_w = (re)->right - (re)->left, \
	 (gr)->g_h = (re)->bottom - (re)->top)
#define GrectToRect(re, gr) \
	((re)->left = (gr)->g_x, \
	 (re)->top = (gr)->g_y, \
	 (re)->right = (gr)->g_x + (gr)->g_w, \
	 (re)->bottom = (gr)->g_y + (gr)->g_h)

_BOOL register_classes(_BOOL init);
HDC W_GetDC(_VOID *wr);
HWND W_GetHwnd(_VOID *wr);
HFONT W_GetFont(HDC hdc, _WORD height, _WORD *w, _WORD *h);
HBITMAP get_mfdb_bitmap(_VOID /* MFDB */ *mfdb);

#endif /* __OPENWORK_H__ */
