#ifndef __ICON_H__
#define __ICON_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif
#ifndef __MFDB_H__
#include <mfdb.h>
#endif

_BOOL draw_img_map(MFDB *image);
_BOOL init_icons(_VOID);
_VOID free_icons(_VOID);

#endif /* __ICON_H__ */
