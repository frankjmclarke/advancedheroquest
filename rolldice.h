#ifndef __ROLLDICE_H__
#define __ROLLDICE_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif
#ifndef __PICE_H__
#include <pice.h>
#endif

typedef enum _rooms { Invalid=0, Small, Normal, Hazard, Large, Lair, Quest, Big, Merscha } ROOMS;
typedef enum _roomdoors { None=0, Room_Door1, Room_Door2, Illegal_Room_Doors } ROOMDOORS;
typedef enum _sections { Section0=0, Section1, Section2, Section3, Illegal_Section } SECTIONS;
typedef enum _ends { Ok, Dead_End, Corner, T_Junction, Right_Turn, Left_Turn, Stairs_Down, Stairs_Out, Illegal_End } ENDS;
typedef enum _roomorpassage { Room, Passage, Illegal_Passage } ROOMORPASSAGE;



_BOOL init_table(_VOID);

SECTIONS passage_length(_VOID);
ENDS passage_end(_VOID);
FEATURES passage_feature(_VOID);

ROOMS room_type(_VOID);
ROOMDOORS room_doors(_VOID);
ROOMORPASSAGE room_or_passage(_VOID);
ROOMDOORS secret_door(_VOID);


_BOOL wandering_monsters(PICE *ptr);
_BOOL passage_features(PICE *ptr);
_BOOL dead_end_features(PICE *ptr);
_BOOL corner_features(PICE *ptr);
_BOOL t_junction_features(PICE *ptr);
_BOOL left_turn_features(PICE *ptr);
_BOOL right_turn_features(PICE *ptr);
_BOOL stairs_out_features(PICE *ptr);
_BOOL stairs_down_features(PICE *ptr);

_BOOL small_features(PICE *ptr);
_BOOL normal_features(PICE *ptr);
_BOOL hazard_features(PICE *ptr);
_BOOL large_features(PICE *ptr);
_BOOL lair_features(PICE *ptr);
_BOOL quest_features(PICE *ptr);
_BOOL big_features(PICE *ptr);
_BOOL merscha_features(PICE *ptr);

#endif /* __ROLLDICE_H__ */
