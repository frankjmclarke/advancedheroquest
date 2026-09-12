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

_VOID draw_mfdb(MFDB *src, _WORD x, _WORD y, _WORD w, _WORD h, MFDB *dest, _WORD dx, _WORD dy, _WORD mode);
_VOID free_mfdb(MFDB *ptr);
MFDB *get_mfdb(_WORD w, _WORD h, _WORD planes, _VOID *data);
_VOID get_mfdb_info(MFDB *mfdb, _WORD *w, _WORD *h, _VOID **data);
MFDB *get_mfdb_from_bitmap(_WORD id);
_BOOL mfdb_start_paint(MFDB *mfdb);
_VOID mfdb_end_paint(MFDB *mfdb);
_VOID W_Draw_Bitmap(_VOID /* WINDOW_DEF */ *window, MFDB *mfdb, _WORD x, _WORD y, _WORD w, _WORD h, _WORD dx, _WORD dy, _WORD zoom);

#endif /* __MFDB_H__ */
