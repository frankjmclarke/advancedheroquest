#ifndef __LISTE_H__
#define __LISTE_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif

size_t draw_monster_liste(_UBYTE **mem);

/* Caller releases the returned buffer with free(). NULL means allocation failure. */
_UBYTE *room_contents_text(_UBYTE **lines);

#endif /* __LISTE_H__ */
