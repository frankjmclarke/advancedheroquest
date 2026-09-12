#include <grect.h>
#include <routine.h>


/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

#undef rc_copy
GLOBAL _VOID rc_copy(CONST GRECT *src, GRECT *dst)
{
	dst->g_x = src->g_x;
	dst->g_y = src->g_y;
	dst->g_w = src->g_w;
	dst->g_h = src->g_h;
}

/*** ---------------------------------------------------------------------- ***/

#undef xywh2rect
GLOBAL _VOID xywh2rect(_WORD x, _WORD y, _WORD w, _WORD h, GRECT *dst)
{
	dst->g_x = x;
	dst->g_y = y;
	dst->g_w = w;
	dst->g_h = h;
}

/*** ---------------------------------------------------------------------- ***/

GLOBAL _BOOL rc_intersect(CONST GRECT *r1, GRECT *r2)
{
	_WORD   xl, yu, xr, yd;		/* left, upper, right, down */

	xl = max(r1->g_x, r2->g_x);
	yu = max(r1->g_y, r2->g_y);
	xr = min(r1->g_x + r1->g_w, r2->g_x + r2->g_w);
	yd = min(r1->g_y + r1->g_h, r2->g_y + r2->g_h);

	r2->g_x = xl;
	r2->g_y = yu;
	r2->g_w = xr - xl;
	r2->g_h = yd - yu;

	return xr > xl && yd > yu;
}
