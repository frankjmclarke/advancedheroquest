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

#endif /* __MAP_H__ */
