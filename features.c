#include <features.h>
#include <rolldice.h>

LOCAL FEATURE_PTR _Nothing = { "Nothing", Nothing };

LOCAL FEATURE_PTR _Section0 = { "Section0", Section0 };
LOCAL FEATURE_PTR _Section1 = { "Section1", Section1 };
LOCAL FEATURE_PTR _Section2 = { "Section2", Section2 };
LOCAL FEATURE_PTR _Section3 = { "Section3", Section3 };
		
LOCAL FEATURE_PTR _Dead_End = { "Dead-End", Dead_End };
LOCAL FEATURE_PTR _Corner = { "Corner", Corner };
LOCAL FEATURE_PTR _T_Junction = { "T-Junction", T_Junction };
LOCAL FEATURE_PTR _Right_Turn = { "Right-Turn", Right_Turn };
LOCAL FEATURE_PTR _Left_Turn = { "Left-Turn", Left_Turn };
LOCAL FEATURE_PTR _Stairs_Down = { "Stairs-Down", Stairs_Down };
LOCAL FEATURE_PTR _Stairs_Out = { "Stairs-Out", Stairs_Out };

LOCAL FEATURE_PTR _Small = { "Small", Small };
LOCAL FEATURE_PTR _Normal = { "Normal", Normal };
LOCAL FEATURE_PTR _Hazard = { "Hazard", Hazard };
LOCAL FEATURE_PTR _Large = { "Large", Large };
LOCAL FEATURE_PTR _Lair = { "Lair", Lair };
LOCAL FEATURE_PTR _Quest = { "Quest", Quest };
LOCAL FEATURE_PTR _Big = { "Big", Big };
LOCAL FEATURE_PTR _Merscha = { "Merscha", Merscha };
		
LOCAL FEATURE_PTR _None = { "None", None };
LOCAL FEATURE_PTR _Door1 = { "Door1", Door1 };
LOCAL FEATURE_PTR _Door2 = { "Door2", Door2 };
		
LOCAL FEATURE_PTR _Room = { "Room", Room };
LOCAL FEATURE_PTR _Passage = { "Passage", Passage };

LOCAL FEATURE_PTR _Wandering_Monsters = { "Wandering-Monsters", Wandering_Monsters };
		
LOCAL FEATURE_PTR _Maiden = { "Maiden", Maiden };
LOCAL FEATURE_PTR _Witch = { "Witch", Witch };
LOCAL FEATURE_PTR _Man_at_Arms = { "Man-at-Arms", Man_at_Arms };
LOCAL FEATURE_PTR _Rogue = { "Rogue", Rogue };
LOCAL FEATURE_PTR _Chasm = { "Chasm", Chasm };
LOCAL FEATURE_PTR _Statue = { "Statue", Statue };
LOCAL FEATURE_PTR _Rats = { "Rats", Rats };
LOCAL FEATURE_PTR _Bats = { "Bats", Bats };
LOCAL FEATURE_PTR _Mould = { "Mould", Mould };
LOCAL FEATURE_PTR _Mushrooms = { "Mushrooms", Mushrooms };
LOCAL FEATURE_PTR _Grate = { "Grate", Grate };
LOCAL FEATURE_PTR _Pool = { "Pool", Pool };
LOCAL FEATURE_PTR _Magic_Circle = { "Magic-Circle", Magic_Circle };
LOCAL FEATURE_PTR _Trapdoor = { "Trapdoor", Trapdoor };
LOCAL FEATURE_PTR _Throne = { "Throne", Throne };
LOCAL FEATURE_PTR _Rockfall = { "Rockfall", Rockfall };
LOCAL FEATURE_PTR _Slime = { "Slime", Slime };
LOCAL FEATURE_PTR _Cess_Pit = { "Cess-Pit", Cess_Pit };
LOCAL FEATURE_PTR _Bridge = { "Bridge", Bridge };
LOCAL FEATURE_PTR _Well = { "Well", Well };
LOCAL FEATURE_PTR _Moths = { "Moths", Moths };
LOCAL FEATURE_PTR _Apparition = { "Apparition", Apparition };
LOCAL FEATURE_PTR _Chest = { "Chest", Chest };
LOCAL FEATURE_PTR _Stairs_and_Chest = { "Stairs_and_Chest", Stairs_and_Chest };
LOCAL FEATURE_PTR _Tomb = { "Tomb", Tomb };
LOCAL FEATURE_PTR _Weapons_Rack = { "Weapons-Rack", Weapons_Rack };
LOCAL FEATURE_PTR _Cupboard = { "Cupboard", Cupboard };
LOCAL FEATURE_PTR _Table = { "Table", Table };
LOCAL FEATURE_PTR _Fireplace = { "Fireplace", Fireplace };
LOCAL FEATURE_PTR _Bookcase = { "Bookcase", Bookcase };
LOCAL FEATURE_PTR _Rack = { "Rack", Rack };

FEATURE *Titel;

FEATURE_PTR EXP_PTR *EXP_PTR Feature_Table[] =
{
	&_None,
	&_Nothing,
	
	&_Section0,
	&_Section1,
	&_Section2,
	&_Section3,
	
	&_Dead_End,
	&_Corner,
	&_T_Junction,
	&_Right_Turn,
	&_Left_Turn,
	&_Stairs_Down,
	&_Stairs_Out,
	
	&_Small,
	&_Normal,
	&_Hazard,
	&_Large,
	&_Lair,
	&_Quest,
	&_Big,
	&_Merscha,
	
	&_None,
	&_Door1,
	&_Door2,
	
	&_Room,
	&_Passage,
	
	&_Wandering_Monsters,
	&_Maiden,
	&_Witch,
	&_Man_at_Arms,
	&_Rogue,
	&_Chasm,
	&_Statue,
	&_Rats,
	&_Bats,
	&_Mould,
	&_Mushrooms,
	&_Grate,
	&_Pool,
	&_Magic_Circle,
	&_Trapdoor,
	&_Throne,
	&_Rockfall,
	&_Slime,
	&_Cess_Pit,
	&_Bridge,
	&_Well,
	&_Moths,
	&_Apparition,
	&_Chest,
	&_Stairs_and_Chest,
	&_Tomb,
	&_Weapons_Rack,
	&_Cupboard,
	&_Table,
	&_Fireplace,
	&_Bookcase,
	&_Rack,
	&_Tomb,
	NULL
};

#define NO_FEATURES { &_None, &_Nothing, NULL}

TABLE *passage_length_table;
FEATURE_PTR *passage_length_ok[] =
{
	&_None,
	&_Nothing,
	&_Section0,
	&_Section1,
	&_Section2,
	&_Section3,
	NULL
};

TABLE *passage_end_table;
FEATURE_PTR *passage_end_ok[] =
{
	&_Dead_End,
	&_Left_Turn,
	&_Right_Turn,
	&_T_Junction,
	&_Corner,
	&_Stairs_Down,
	&_Stairs_Out,
	NULL,
};
	
TABLE *passage_feature_table;
FEATURE_PTR *passage_feature_ok[] =
{
	&_None,
	&_Nothing,
	&_Door1,
	&_Door2,
	&_Wandering_Monsters,
	NULL
};
	
TABLE *room_type_table;
FEATURE_PTR *room_type_ok[] =
{
	&_Small,
	&_Normal,
	&_Hazard,
	&_Large,
	&_Lair,
	&_Quest,
	&_Big,
	&_Merscha,
	NULL
};
	
TABLE *room_doors_table;
FEATURE_PTR *room_doors_ok[] =
{
	&_None,
	&_Nothing,
	&_Door1,
	&_Door2,
	NULL
};
	
TABLE *room_or_passage_table;
FEATURE_PTR *room_or_passage_ok[] =
{
	&_Room,
	&_Passage,
	NULL
};
	
TABLE *secret_door_table;
FEATURE_PTR *secret_door_ok[] =
{
	&_None,
	&_Nothing,
	&_Door1,
	NULL
};

TABLE *passage_table;
FEATURE_PTR *passage_ok[] = NO_FEATURES;
	
TABLE *dead_end_table;
FEATURE_PTR *dead_end_ok[] = NO_FEATURES;
	
TABLE *corner_table;
FEATURE_PTR *corner_ok[] = NO_FEATURES;
	
TABLE *t_junction_table;
FEATURE_PTR *t_junction_ok[] = NO_FEATURES;
	
TABLE *right_turn_table;
FEATURE_PTR *right_turn_ok[] = NO_FEATURES;
	
TABLE *left_turn_table;
FEATURE_PTR *left_turn_ok[] = NO_FEATURES;
	
TABLE *stairs_down_table;
FEATURE_PTR *stairs_down_ok[] = NO_FEATURES;
	
TABLE *stairs_out_table;
FEATURE_PTR *stairs_out_ok[] = NO_FEATURES;

TABLE *small_table;	
FEATURE_PTR	EXP_PTR *EXP_PTR small_ok[] =
{
	&_None,
	&_Nothing,
	&_Wandering_Monsters,
	&_Maiden,
	&_Witch,
	&_Man_at_Arms,
	&_Rogue,
	&_Chasm,
	&_Statue,
	&_Rats,
	&_Bats,
	&_Mould,
	&_Mushrooms,
	&_Grate,
	&_Pool,
	&_Magic_Circle,
	&_Trapdoor,
	&_Throne,
	&_Rockfall,
	&_Slime,
	&_Cess_Pit,
	&_Bridge,
	&_Well,
	&_Moths,
	&_Apparition,
	&_Chest,
	&_Stairs_and_Chest,
	&_Tomb,
	&_Weapons_Rack,
	&_Cupboard,
	&_Table,
	&_Fireplace,
	&_Bookcase,
	&_Rack,
	NULL
};

TABLE *normal_table;
FEATURE_PTR	EXP_PTR *EXP_PTR normal_ok[] =
{
	&_None,
	&_Nothing,
	&_Wandering_Monsters,
	&_Maiden,
	&_Witch,
	&_Man_at_Arms,
	&_Rogue,
	&_Chasm,
	&_Statue,
	&_Rats,
	&_Bats,
	&_Mould,
	&_Mushrooms,
	&_Grate,
	&_Pool,
	&_Magic_Circle,
	&_Trapdoor,
	&_Throne,
	&_Rockfall,
	&_Slime,
	&_Cess_Pit,
	&_Bridge,
	&_Well,
	&_Moths,
	&_Apparition,
	&_Chest,
	&_Stairs_and_Chest,
	&_Tomb,
	&_Weapons_Rack,
	&_Cupboard,
	&_Table,
	&_Fireplace,
	&_Bookcase,
	&_Rack,
	NULL
};

TABLE *hazard_table;
FEATURE_PTR	EXP_PTR *EXP_PTR hazard_ok[] =
{
	&_None,
	&_Nothing,
	&_Wandering_Monsters,
	&_Maiden,
	&_Witch,
	&_Man_at_Arms,
	&_Rogue,
	&_Chasm,
	&_Statue,
	&_Rats,
	&_Bats,
	&_Mould,
	&_Mushrooms,
	&_Grate,
	&_Pool,
	&_Magic_Circle,
	&_Trapdoor,
	&_Throne,
	&_Rockfall,
	&_Slime,
	&_Cess_Pit,
	&_Bridge,
	&_Well,
	&_Moths,
	&_Apparition,
	&_Chest,
	&_Stairs_and_Chest,
	&_Tomb,
	&_Weapons_Rack,
	&_Cupboard,
	&_Table,
	&_Fireplace,
	&_Bookcase,
	&_Rack,
	NULL
};

TABLE *large_table;
FEATURE_PTR *large_ok[] =
{
	&_None,
	&_Nothing,
	&_Chest,
	&_Stairs_and_Chest,
	&_Tomb,
	NULL
};
	
TABLE *lair_table;
FEATURE_PTR *lair_ok[] =
{
	&_None,
	&_Nothing,
	&_Chest,
	&_Stairs_and_Chest,
	&_Tomb,
	NULL
};
	
TABLE *quest_table;
FEATURE_PTR *quest_ok[] =
{
	&_None,
	&_Nothing,
	&_Chest,
	&_Stairs_and_Chest,
	&_Tomb,
	NULL
};

TABLE *big_table;
FEATURE_PTR *big_ok[] =
{
	&_None,
	&_Nothing,
	NULL
};
	
TABLE *merscha_table;
FEATURE_PTR *merscha_ok[] =
{
	&_None,
	&_Nothing,
	NULL
};

TABLE *wandering_monsters_table;
FEATURE_PTR *wandering_monsters_ok[] = NO_FEATURES;
