#ifndef __TEST_H__
#define __TEST_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif
#ifndef __ROLLDICE_H__
#include <rolldice.h>
#endif

ENDS test_end(PICE *from, ENDS type);
SECTIONS test_passage(PICE *from, SECTIONS len);
ROOMS test_passage_room(PICE *from, SECTIONS len, ROOMS roomtype);
ROOMS test_room_room(PICE *from, ROOMS roomtype);
_BOOL test_room_passage(PICE *from);
_BOOL test_room_door(PICE *from);
_BOOL test_passage_door(PICE *from, SECTIONS len);
_BOOL test_door(PICE *from);

#endif /* __TEST_H__ */
