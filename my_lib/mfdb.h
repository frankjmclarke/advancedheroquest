#ifndef __MFDB_H__
#define __MFDB_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif

#ifndef __MFDB_IMPLEMENTATION__
DUMMY_STRUCT(_mfdb);
#endif
typedef struct _mfdb MFDB;

#define MD_REPLACE 1
#define MD_TRANS   2
#define MD_XOR     3
#define MD_ERASE   4

/*
 * Colour used by the next draw_mfdb(). The tile bitmaps are 1 bit deep, so
 * GDI expands them when blitting into a colour destination: set bits take
 * the paper colour, clear bits take the ink colour. That turns the existing
 * monochrome artwork into a stencil which can be tinted per feature, with no
 * change to the bitmaps themselves.
 *
 * Has no effect when the destination is also 1 bit deep, which is what keeps
 * the saved and printed maps identical to before.
 */
#define W_RGB(r, g, b) ((_LONG)(((_ULONG)(_UBYTE)(r)) | (((_ULONG)(_UBYTE)(g)) << 8) | (((_ULONG)(_UBYTE)(b)) << 16)))

_VOID set_mfdb_colors(_LONG ink, _LONG paper);
_VOID reset_mfdb_colors(_VOID);

_VOID draw_mfdb(MFDB *src, _WORD x, _WORD y, _WORD w, _WORD h, MFDB *dest, _WORD dx, _WORD dy, _WORD mode);
_VOID free_mfdb(MFDB *ptr);
MFDB *get_mfdb(_WORD w, _WORD h, _WORD planes, _VOID *data);
_VOID get_mfdb_info(MFDB *mfdb, _WORD *w, _WORD *h, _VOID **data);
MFDB *get_mfdb_from_bitmap(_WORD id);
_BOOL mfdb_start_paint(MFDB *mfdb);
_VOID mfdb_end_paint(MFDB *mfdb);
_VOID W_Draw_Bitmap(_VOID /* WINDOW_DEF */ *window, MFDB *mfdb, _WORD x, _WORD y, _WORD w, _WORD h, _WORD dx, _WORD dy, _WORD zoom);

/* Scale a complete colour tile to its on-screen bounds. */
_VOID W_Draw_Tile(_VOID *window, MFDB *tile, _WORD dx, _WORD dy, _WORD w, _WORD h);

#endif /* __MFDB_H__ */
