#ifndef __GRECT_H__
#define __GRECT_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif

typedef struct _grect {
	_WORD g_x;
	_WORD g_y;
	_WORD g_w;
	_WORD g_h;
} GRECT;

typedef struct {
	_LONG xx, yy, ww, hh;
} L_GRECT;

_BOOL rc_intersect(CONST GRECT *gr1, GRECT *gr2);
_VOID rc_copy(CONST GRECT *src, GRECT *dst);
_VOID xywh2rect(_WORD x, _WORD y, _WORD w, _WORD h, GRECT *dst);

#define rc_copy(src, dst) *(dst) = *(src)
#define xywh2rect(x, y, w, h, gr) \
	((gr)->g_x = x, \
	 (gr)->g_y = y, \
	 (gr)->g_w = w, \
	 (gr)->g_h = h)

#endif /* __GRECT_H__ */
