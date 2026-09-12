#ifndef __FEATURES_H__
#define __FEATURES_H__

#ifndef __PORTAB_H__
#include <portab.h>
#endif

typedef enum _features {
	Nothing = 0,
	Door1,
	Door2,
	Wandering_Monsters,
	Maiden,
	Witch,
	Man_at_Arms,
	Rogue,
	Chasm,
	Statue,
	Rats,
	Bats,
	Mould,
	Mushrooms,
	Grate,
	Pool,
	Magic_Circle,
	Trapdoor,
	Throne,
	Wight,
	Rockfall,
	Slime,
	Cess_Pit,
	Bridge,
	Well,
	Moths,
	Apparition,
	Chest,
	Stairs_and_Chest,
	Tomb,
	Weapons_Rack,
	Cupboard,
	Table,
	Fireplace,
	Bookcase,
	Rack,
	Illegal_Feature
} FEATURES;
				 

enum {SUBTABLE, TEXTFEATURE, ROOMFEATURE};

typedef struct feature_ptr
{
	_UBYTE *name;
	FEATURES feature;
}	FEATURE_PTR;

typedef struct feature
{
	_WORD type;
	struct feature *next;
	union
	{
		_VOID *subtable;
		_UBYTE *text;
		FEATURE_PTR *room;
	} ptr;
} FEATURE;


typedef struct entry
{
	_WORD roll;
	struct entry *next;
	struct feature *ptr;
} ENTRY;


typedef struct table
{
	_UBYTE *name;
	struct table *next;
	struct entry *entry;
	_WORD n, dice;
} TABLE;


typedef struct define
{
	_UBYTE *name;
	struct define *next;
} DEFINE;

extern FEATURE *Titel;

extern FEATURE_PTR EXP_PTR *EXP_PTR Feature_Table[];

extern TABLE *passage_length_table;
extern FEATURE_PTR *passage_length_ok[];
extern TABLE *passage_end_table;
extern FEATURE_PTR *passage_end_ok[];
extern TABLE *passage_feature_table;
extern FEATURE_PTR *passage_feature_ok[];
extern TABLE *room_type_table;
extern FEATURE_PTR *room_type_ok[];
extern TABLE *room_doors_table;
extern FEATURE_PTR *room_doors_ok[];
extern TABLE *room_or_passage_table;
extern FEATURE_PTR *room_or_passage_ok[];
extern TABLE *secret_door_table;
extern FEATURE_PTR *secret_door_ok[];

extern TABLE *passage_table;
extern FEATURE_PTR *passage_ok[];
extern TABLE *dead_end_table;
extern FEATURE_PTR *dead_end_ok[];
extern TABLE *corner_table;
extern FEATURE_PTR *corner_ok[];
extern TABLE *t_junction_table;
extern FEATURE_PTR *t_junction_ok[];
extern TABLE *right_turn_table;
extern FEATURE_PTR *right_turn_ok[];
extern TABLE *left_turn_table;
extern FEATURE_PTR *left_turn_ok[];
extern TABLE *stairs_down_table;
extern FEATURE_PTR *stairs_down_ok[];
extern TABLE *stairs_out_table;
extern FEATURE_PTR *stairs_out_ok[];

extern TABLE *small_table;
extern FEATURE_PTR EXP_PTR* EXP_PTR small_ok[];
extern TABLE *normal_table;
extern FEATURE_PTR EXP_PTR *EXP_PTR normal_ok[];
extern TABLE *hazard_table;
extern FEATURE_PTR EXP_PTR *EXP_PTR hazard_ok[];
extern TABLE *large_table;
extern FEATURE_PTR *large_ok[];
extern TABLE *lair_table;
extern FEATURE_PTR *lair_ok[];
extern TABLE *quest_table;
extern FEATURE_PTR *quest_ok[];
extern TABLE *big_table;
extern FEATURE_PTR *big_ok[];
extern TABLE *merscha_table;
extern FEATURE_PTR *merscha_ok[];

extern TABLE *wandering_monsters_table;
extern FEATURE_PTR *wandering_monsters_ok[];

#endif /* __FEATURES_H__ */
