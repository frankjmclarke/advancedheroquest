#ifndef __MAP_H__
#define __MAP_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif
#ifndef __PICE_H__
#include <pice.h>
#endif

extern _WORD MAX_X;
extern _WORD MAX_Y;
extern _WORD Xsize, Ysize;

_BOOL get_square(_WORD x, _WORD y, PICE **ptr);
_BOOL set_square(_WORD x, _WORD y, PICE *ptr);

_BOOL init_map(_WORD x, _WORD y);

_BOOL map_init(_WORD max_x, _WORD max_y);
_VOID map_exit(_VOID);

#define clr_square(x,y) set_square((x), (y), NULL)

/* One visibility byte per Pice slot. Doors derive visibility from their sides. */
_VOID fog_init(_UBYTE *visible);
_BOOL fog_visible(CONST _UBYTE *visible, _WORD index);
_BOOL fog_open_door(_UBYTE *visible, _WORD pixel_x, _WORD pixel_y);

#endif /* __MAP_H__ */
