#ifndef __WINDOW_H__
#define __WINDOW_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif
#ifndef __GRECT_H__
#include <grect.h>
#endif

/* Window Attributes */

#define WAT_NAME        0x0001
#define WAT_CLOSER      0x0002
#define WAT_FULLER      0x0004
#define WAT_MOVER       0x0008
#define WAT_INFO        0x0010
#define WAT_SIZER       0x0020
#define WAT_VERSLIDE    (0x0040|0x0080|0x0100)
#define WAT_HORSLIDE    (0x0200|0x0400|0x0800)
#define WAT_SMALLER     0x4000

/* Erweiterte Window Attributes */
#define WAT_DONTBLANK   0x0001
#define WAT_MODAL       0x0002

/* arrow message */

#define WA_UPPAGE        0		/* Window Arrow Up Page    */
#define WA_DNPAGE        1		/* Window Arrow Down Page  */
#define WA_UPLINE        2		/* Window Arrow Up Line    */
#define WA_DNLINE        3		/* Window Arrow Down Line  */
#define WA_LFPAGE        4		/* Window Arrow Left Page  */
#define WA_RTPAGE        5		/* Window Arrow Right Page */
#define WA_LFLINE        6		/* Window Arrow Left Line  */
#define WA_RTLINE        7		/* Window Arrow Right Line */
#define WA_VSLIDE        8		/* Window vertical slider moved */
#define WA_HSLIDE        9		/* Window horizontal slider moved */


#define MOVED		0x01		/* Flags fuer hslid_wind/vslid_wind */
#define SIZED		0x02

#define HORIZONTAL	0x01
#define VERTICAL	0x02

#define SLPOS		0x01
#define SLSIZE		0x02

typedef enum
{
	WMY_OPEN,
	WMY_UPDATE,	/* Parameter ist GRECT * */
	WMY_CLOSE,
	WMY_HIT,	/* Parameter ist WIPR_HIT * */

	WMY_OUT,	/* Fuer Speichern, Parameter ist FILE * */
	WMY_PRINT,	/* Fuer Druckausgaben, Parameter ist FILE * */
	
	WMY_UNTOP,
	WMY_TOP,
	WMY_KEY,	/* Parameter ist MKINFO * */
} WIND_MESSAGE;

typedef struct
{
	_WORD xx;
	_WORD yy;
	_WORD clicks;
} WIPR_HIT;    /* Wird in wind_proc_ptr bei WM_HIT uebertragen */

typedef struct
{
	_WORD group;
	_LONG var_bez;
	GRECT pos;
	_WORD reOpen;
} WIND_RESTORE;

/* Event-Struktur */
typedef struct
{
	_WORD   mouse_x;
	_WORD   mouse_y;
	_WORD   xx, yy;
	_UWORD  button;
	_UWORD  button2;
	_UWORD  state;
	_WORD   key;
	_WORD   clicks;
	_WORD   ascii_code;
	_WORD   scan_code;
	_WORD   shift;
	_WORD   ctrl;
	_WORD   alt;
} MKINFO;

#define MOB_LEFT   0x01
#define MOB_RIGHT  0x02
#define MOB_MIDDLE 0x04

#define K_RSHIFT   0x01
#define K_LSHIFT   0x02
#define K_SHIFT    (K_LSHIFT | K_RSHIFT)
#define K_CTRL     0x04
#define K_ALT      0x08
#define K_CAPSLOCK 0x10

#ifndef __WINDOW_IMPLEMENTATION__
DUMMY_STRUCT(_window_def);
#endif

typedef struct _window_def WINDOW_DEF;

typedef _BOOL (*WINDOW_PROC)(WIND_MESSAGE wind_message, WINDOW_DEF *wind_ptr, _VOID *buf);
typedef _VOID (*REDRAW_FUNC)(_VOID *para, GRECT *area);

WINDOW_DEF *Wind_Top(_VOID);

_BOOL Wind_All_Ptr(_BOOL (*funk)(WINDOW_DEF *window));

_VOID Wind_Redraw(WINDOW_DEF *aktWind);
_VOID Wind_Redraw_Rect(WINDOW_DEF *wr, CONST GRECT *gr, _BOOL now);
_VOID Wind_All_Group_Redraw(_WORD group);
_VOID Wind_All_Redraw(_VOID);
_BOOL Wind_Scroll_Area(WINDOW_DEF *wr, _WORD dir, _LONG amount, _BOOL scroll, CONST GRECT *drawRedrGr, L_GRECT *show, _WORD xRaster, _WORD yRaster, REDRAW_FUNC func, _VOID *para);

typedef size_t (*WH_READWRITE)(_BOOL save, WIND_RESTORE *buf, size_t numElems);
typedef _BOOL (*WH_SAVERESTORE)(_BOOL save, _WORD group, _LONG *var_bez);

_BOOL Wind_Hide_Show_Init(WH_READWRITE readwrite, WH_SAVERESTORE saverestore);
_BOOL Wind_Hide_Show_Windows(_BOOL hide);
_BOOL Wind_Pexec(_UBYTE *name, _UBYTE *para);

WINDOW_DEF *Wind_Open(_WORD group, _UWORD flags, WINDOW_PROC windowProc, _UBYTE *titel, _VOID *buf);

_BOOL Wind_Close(WINDOW_DEF *aktWind);
_VOID Wind_Close_All(_BOOL delete);

_VOID Wind_Title(WINDOW_DEF *aktWind, _UBYTE *titel);

_VOID Wind_On_Top(WINDOW_DEF *aktWind);
_VOID Wind_On_Bottom(WINDOW_DEF *aktWind);
_VOID Wind_Next_Top(_VOID);

_VOID *Wind_Buf_Ptr(WINDOW_DEF *window);
WINDOW_PROC Wind_Proc_Ptr(WINDOW_DEF *window);
_VOID Wind_GetScroll(WINDOW_DEF *wind, GRECT *gr);
_VOID Wind_GetWork(WINDOW_DEF *wind, GRECT *gr);
_VOID Wind_GetSize(WINDOW_DEF *wr, GRECT *gr);

_VOID Wind_Menu_Enable(_WORD id, _BOOL enable);
_BOOL Wind_Menu_Enabled(_WORD id);

_VOID Wind_Menu_Check(_WORD id, _BOOL check);
_BOOL Wind_Menu_Checked(_WORD id);

_VOID Wind_Set_Sliders(WINDOW_DEF *windAkt, _WORD which, _WORD mode);
_VOID Wind_SetDoc(WINDOW_DEF *window, CONST L_GRECT *show);
_VOID Wind_GetDoc(WINDOW_DEF *window, L_GRECT *show);
_VOID Wind_SetFac(WINDOW_DEF *window, _WORD xfac, _WORD yfac, _WORD xunits, _WORD yunits);
_VOID Wind_GetFac(WINDOW_DEF *window, _WORD *xfac, _WORD *yfac, _WORD *xunits, _WORD *yunits);

_VOID Wind_Arrow(WINDOW_DEF *wr, _WORD arrow, _LONG amount, CONST GRECT *redraw, REDRAW_FUNC func);

typedef _BOOL (*MENU_FUNC)(_WORD menue_nr);
_VOID Evnt_Multi(MENU_FUNC fu_event_menu, _WORD menue_id);

#endif /* __WINDOW_H__ */
