#ifndef __W_DRAW_H__
#define __W_DRAW_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif
#ifndef __WINDOW_H__
#include <window.H>
#endif
#ifndef __GRECT_H__
#include <grect.h>
#endif

#define FONT_SIZE_ICON 50
#define FONT_SIZE_SMALL 60
#define FONT_SIZE_NORMAL 120

#define W_PAL_BLACK PALETTERGB(0, 0, 0)
#define W_PAL_WHITE PALETTERGB(255, 255, 255)

_VOID GetMaxScreenSize(_WORD *w, _WORD *h);
_WORD GetNumPlanes(_VOID);
_WORD GetNumColors(_VOID);
_VOID GetScreenSize(_WORD *breite, _WORD *hoehe);

_VOID W_Clear_Rect(WINDOW_DEF *wr, CONST GRECT *area);
_VOID W_ScrollGrect(WINDOW_DEF *wr, _WORD diffX, _WORD diffY, CONST GRECT *to, REDRAW_FUNC func, _VOID *para);
_VOID W_GetFontSize(WINDOW_DEF *wr, _WORD size, _WORD *w, _WORD *h);
_VOID W_Text(WINDOW_DEF *window, _WORD x, _WORD y, _WORD size, CONST _UBYTE *text, size_t len);

#endif /* __W_DRAW_H__ */
