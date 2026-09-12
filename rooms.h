#ifndef __ROOMS_H__
#define __ROOMS_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif

typedef struct icon
{
	struct icon **ptr;
	_WORD rsc;
	DIRECTION (*position)(DIRECTION dir, _WORD *x, _WORD *y, _WORD w, _WORD h);
	MFDB *(*number)(DIRECTION dir, _WORD *x, _WORD *y, _WORD w, _WORD h, MFDB *x_mfdb, MFDB *y_mfdb);
	MFDB mfdb[4];
} ICON;


ICON *border;
ICON *normal;
ICON *hazard;
ICON *large;
ICON *lair;
ICON *quest;
ICON *big;
ICON *merscha;

ICON *passage;

ICON *deadend;
ICON *left_turn;
ICON *right_turn;
ICON *t_junction;
ICON *corner;
ICON *stairs_out;
ICON *stairs_down;

ICON *door;
ICON *secret;
ICON *test;

ICON *bridge;
ICON *chasm;

ICON *grate;
ICON *trap;
ICON *throne;
ICON *statue;
ICON *pool;
ICON *cess_pit;
ICON *well;
ICON *circle;
ICON *rats;
ICON *bats;
ICON *mould;
ICON *mushrooms;
ICON *apparition;
ICON *rockfall;
ICON *slime;
ICON *moths;
ICON *chest;


LOCAL ICON Icons[]=
{
	{&border, FBORDER, NULL, NULL},
	
	{&normal, FNORMAL, NULL, center_nr},
	{&hazard, FHAZARD, NULL, center_nr},
	{&large, FLARGE, room_pos, center_nr},
	{&lair, FLAIR, room_pos, center_nr},
	{&quest, FQUEST, room_pos, center_nr},
	{&big, FBIG, room_pos, center_nr},
	{&merscha, FMERSCHA, NULL, center_nr},
	
	{&passage, FPASSAGE, NULL, center_nr},
	
	{&deadend, FDEADEND, NULL, center_nr},
	{&left_turn, FLTURN, NULL, center_nr},
	{&right_turn, FRTURN, NULL, center_nr},
	{&t_junction, FTJUNCT, NULL, center_nr},
	{&corner, FCORNER, NULL, center_nr},
	{&stairs_out, FSTOUT, NULL, center_nr},
	{&stairs_down, FSTDOWN, NULL, center_nr},
	
	{&door, FDOOR, door_pos, NULL},
	{&secret, FSECRET, door_pos, NULL},
	{&test, FTEST, door_pos, NULL},
	
	{&bridge, FBRIDGE, NULL, top_nr},
	{&chasm, FCHASM, NULL, top_nr},
	{&throne, FTHRONE, NULL, bottom_nr},
	{&statue, FSTATUE, NULL, bottom_nr},
	{&cess_pit, FCESSPIT, NULL, south_nr},
	{&well, FWELL, NULL, south_nr},
	{&pool, FPOOL, NULL, south_nr},
	{&circle, FCIRCLE, NULL, south_nr},
	{&grate, FGRATE, NULL, south_nr},
	{&trap, FTRAP, NULL, south_nr},
	{&rats, FRAT, NULL, south_nr},
	{&bats, FBAT, NULL, south_nr},
	{&mould, FMOULD, NULL, south_nr},
	{&mushrooms, FMUSH, NULL, south_nr},
	{&apparition, FAPPARI, NULL, south_nr},
	{&rockfall, FROCK, NULL, bottom_nr},
	{&slime, FSLIME, NULL, bottom_nr},
	{&moths, FMOTH, NULL, south_nr},
	{&chest, FCHEST, NULL, center_nr},
	{NULL}
};

#endif /* __ROOMS_H__ */
