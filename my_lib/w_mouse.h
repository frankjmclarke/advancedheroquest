/*****************************************************************************
 * W_MOUSE.H
 *****************************************************************************/

#ifndef __W_MOUSE_H__
#define __W_MOUSE_H__

#ifndef __PORTAB_H__
#  include <portab.h>
#endif

/* Maus-Funktionen */
#define MOUSE_UNKNOWN	-1
#define MOUSE_RESET		-2
#define MOUSE_RESTORE	-3
#define MOUSE_MENU		0		/* in menu line */
#define MOUSE_NORMAL	1		/* on desktop */
#define MOUSE_DIALOG	2		/* in dialogs */
#define MOUSE_BUSY		3		/* waiting */
#define MOUSE_MOVING	4		/* moving objects */
#define MOUSE_SIZE_NESW	5		/* resizing object */
#define MOUSE_SIZE_SWNE	6		/* resizing object */
#define MOUSE_SIZE_NWSE	7		/* resizing object */
#define MOUSE_SIZE_SENW	8		/* resizing object */
#define MOUSE_SIZE_NS	9		/* resizing object */
#define MOUSE_SIZE_SN	10		/* resizing object */
#define MOUSE_SIZE_WE	11		/* resizing object */
#define MOUSE_SIZE_EW	12		/* resizing object */
#define MOUSE_POINTING	13
#define MOUSE_MARKING	14
#define MOUSE_MOVE_NS   15
#define MOUSE_MOVE_SN   16
#define MOUSE_MOVE_WE   17
#define MOUSE_MOVE_EW   18

_VOID SetMouse(_WORD form);
_BOOL SetMouse_Shape(_UBYTE *data, _UBYTE *mask, _WORD w, _WORD h, _WORD color);
_VOID HideMouse(_VOID);
_VOID ShowMouse(_VOID);
_WORD GetMouseCalls(_VOID);
_VOID SetMouseCalls(_WORD);

#endif /* __W_MOUSE_H__ */
