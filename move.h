#ifndef __MOVE_H__
#define __MOVE_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif
#ifndef __PICE_H__
#include <pice.h>
#endif

DIRECTION straight(PICE *from);
DIRECTION left(PICE *from);
DIRECTION right(PICE *from);
DIRECTION back(PICE *from);
_BOOL pos_north(PICE *new, PICE *from);
_BOOL pos_south(PICE *new, PICE *from);
_BOOL pos_east(PICE *new, PICE *from);
_BOOL pos_west(PICE *new, PICE *from);
_BOOL pos_at(PICE *new, PICE *from);
_BOOL pos_straight(PICE *new, PICE *from);
_BOOL pos_back(PICE *new, PICE *from);
_BOOL pos_left(PICE *new, PICE *from);
_BOOL pos_right(PICE *new, PICE *from);

_BOOL xy_north(PICE *new, PICE *from);
_BOOL xy_south(PICE *new, PICE *from);
_BOOL xy_east(PICE *new, PICE *from);
_BOOL xy_west(PICE *new, PICE *from);
_BOOL xy_at(PICE *new, PICE *from);
_BOOL xy_straight(PICE *new, PICE *from);
_BOOL xy_back(PICE *new, PICE *from);
_BOOL xy_left(PICE *new, PICE *from);
_BOOL xy_right(PICE *new, PICE *from);

_WORD length(PICE *from);
_WORD wide(PICE *from);

#endif /* __MOVE_H__ */
