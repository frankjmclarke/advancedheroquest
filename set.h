#ifndef __SET_H__
#define __SET_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif
#ifndef __PICE_H__
#include <pice.h>
#endif
#ifndef __ROLLDICE_H__
#include <rolldice.h>
#endif

_BOOL set_end(PICE *from, ENDS type);
_BOOL set_passage(PICE *from, SECTIONS len, FEATURES feature);
_BOOL set_room(PICE *ptr, ROOMS roomtype);
_BOOL set_door(PICE *ptr, TYPE doortype);
_BOOL set_room_door(PICE *ptr, TYPE doortype);
_BOOL set_passage_door(PICE *ptr, SECTIONS len, TYPE doortype);

#endif /* __SET_H__ */
