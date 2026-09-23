#ifndef __LISTE_H__
#define __LISTE_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif

size_t draw_monster_liste(_UBYTE **mem);

/* Caller releases the returned buffer with free(). NULL means allocation failure. */
_UBYTE *room_contents_text(_UBYTE **lines);

/* Enumerate only unambiguous reference matches. Returns nonzero when manual
 * review is needed. Does not roll dice or alter generated contents. */
int room_monsters(_UBYTE **lines, void (*add)(const char *,const int *,int,void *),void *context);
/* Exact, unambiguous printed melee profile; no inferred weapon statistics. */
int room_melee(const char *name,int *dice,int hits[12]);
int room_ranged(const char *name,int *range,int *dice,int hits[5],int *kind);
#endif /* __LISTE_H__ */
