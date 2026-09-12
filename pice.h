#ifndef __PICE_H__
#define __PICE_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif
#ifndef __FEATURES_H__
#include <features.h>
#endif

typedef enum _direction { North=0, South, East, West, Illegal_Dir } DIRECTION;

typedef enum _type
{
	EMPTY		= ' ',
	INVALID 	= '?',

	PASSAGE		= 'P',

	DEAD_END	= 'E',
	LEFT_TURN	= 'L',
	RIGHT_TURN	= 'R',
	T_JUNCTION	= 'T',
	CORNER		= 'C',
	STAIRS_OUT	= 'O',
	STAIRS_DOWN	= 'D',

	SMALL_ROOM	= 'S',
	NORMAL_ROOM	= 'N',
	HAZARD		= 'H',
	LARGE_ROOM	= 'X',
	LAIR		= 'A',
	QUEST		= 'Q',
	BIG			= 'B',
	MERSCHA		= 'M',

	TEST		= '#',
	DOOR		= '*',
	SECRET		= '$',
	
} TYPE;

typedef struct pice
{
	TYPE type;
	FEATURES feature;
	_UBYTE **text;
	_WORD x, y, w, h;
	DIRECTION pos;
} PICE;

typedef struct size { _WORD w, h; } SIZE;

extern _WORD MAX_PICE;
extern PICE *Pice;

extern CONST SIZE Section;
extern CONST SIZE End;
extern CONST SIZE Ultra_Room;
extern CONST SIZE Big_Room;
extern CONST SIZE Large_Room;
extern CONST SIZE Small_Room;

extern CONST SIZE Border;
extern CONST SIZE Door;
extern CONST SIZE Void;

#define copy_pice(d, s) (*(d) = *(s))

_BOOL init_pice(PICE *ptr, TYPE type, _WORD x, _WORD y, DIRECTION pos);
_BOOL init_at(PICE *new, TYPE type, PICE *from);
_BOOL init_straight(PICE *new, TYPE type, PICE *from);
_BOOL init_back(PICE *new, TYPE type, PICE *from);
_BOOL init_left(PICE *new, TYPE type, PICE *from);
_BOOL init_right(PICE *new, TYPE type, PICE *from);

_BOOL clr_pice(PICE *ptr);
_BOOL delete_pice(PICE *ptr);
_BOOL insert_pice(PICE *dat);
_BOOL test_pice(PICE *ptr);

_VOID pice_empty(_VOID);

_BOOL pice_init(_WORD max_pice);
_VOID pice_exit(_VOID);

#endif /* __PICE_H__ */
