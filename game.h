#ifndef HQ_GAME_H
#define HQ_GAME_H
#include <map.h>
#include <window.h>
#define HERO_LIMIT 16
#define HERO_STATS 9
#define HERO_CLASS_COUNT 9
/* Zero values mean not configured; presets use printed or labelled suggested rows. */
/* reach: 1 normal (edge-adjacent), 2 long (also diagonal; long-reach death zone). */
typedef struct { char weapon[40]; int dice, hit[12]; int critical, fumble, reach; } MELEE_PROFILE;
/* kind: none, bow, crossbow, thrown, other/manual. Hit bands: 1-3,4-12,13-24,25-36,37+. */
typedef struct { char weapon[40]; int kind,range,dice,hit[5],critical,fumble; } RANGED_PROFILE;
typedef struct {
    char name[40];
    int kind, stats[HERO_STATS], wounds, fate, x, y;
    int condition; /* 0 active, 1 KO'd at zero Wounds, 2 dead below zero */
    MELEE_PROFILE melee;
    RANGED_PROFILE ranged;
    char profile_id[64],class_name[64],class_rules[2048];
    int fired; /* normal ranged attack used this turn */
    int moved,focus; /* focus: opposing roster index + 1, or zero */
    int move_spent,attacked,run_bonus; /* guided combat phase state */
} HERO;
#define HERO_ACTIVE(h) ((h)->condition==0 && (h)->wounds>0)
#define HERO_KO(h) ((h)->condition==1)
#define HERO_DEAD(h) ((h)->condition==2 || ((h)->kind==4 && (h)->wounds==0))
#define MONSTER_LIMIT 512
#define CHARACTER_LIMIT 256
#define ENCOUNTER_LIMIT 8192
typedef struct {
    char name[64];
    char profile_id[64],character_id[64];
    int stats[HERO_STATS], wounds, x, y, room, unique;
    MELEE_PROFILE melee;
    RANGED_PROFILE ranged;
    int moved,focus; /* focus: opposing roster index + 1, or zero */
    int move_spent,attacked,run_bonus; /* guided combat phase state */
} MONSTER;
typedef struct {
    int count, dead_count;
    MONSTER tokens[MONSTER_LIMIT];
    char dead[CHARACTER_LIMIT][64];
    unsigned char seen[ENCOUNTER_LIMIT];
} MONSTER_STATE;
typedef struct {
    struct character_pack *pack; /* owned by decoded games, borrowed by capture */
    char title[160];
    int width, height, count, hero_count;
    int player_view; /* -1: legacy save with no recorded view; 0: GM; 1: player */
    int turn_phase; /* 0 free play, 1 Hero phase, 2 GM phase */
    int gm_override;
    HERO heroes[HERO_LIMIT];
    MONSTER_STATE monsters;
    PICE *pieces;
    int *cells;
    unsigned char *visible;
} GAME_DATA;
extern const char *hero_classes[HERO_CLASS_COUNT];
const char *hero_class_rules(int kind);
void hero_defaults(HERO *hero,int kind,int number);
void hero_melee_defaults(MELEE_PROFILE *profile,int kind);
void hero_ranged_defaults(RANGED_PROFILE *profile,int kind);
int monster_ranged_defaults(RANGED_PROFILE *profile,const char *name);
int game_valid_square(const GAME_DATA *data,int hero,int x,int y);
int game_encode(const GAME_DATA *data,unsigned char **bytes,size_t *size);
GAME_DATA *game_decode(const unsigned char *bytes,size_t size);
void game_data_free(GAME_DATA *data);
const char *monster_identity(const MONSTER *monster);
void game_initialize(void);
void game_shutdown(void);
void game_party(void);
void game_monsters(void);
int game_save(int save_as);
/* 0: chosen file; 1: explicit recovery; 2: quiet automatic startup recovery. */
int game_load(int recovery);
int game_before_replace(void);
void game_new_dungeon(const char *title);
void game_discard_dungeon(void);
void game_changed(void);
int game_recover(void);
int game_has_loaded_session(void);
void game_attach(WINDOW_DEF *window,int player);
void game_draw(WINDOW_DEF *window,const unsigned char *visible,int zoom,int sx,int sy);
/* Fog belongs to the dungeon, including when its window is closed. */
unsigned char *game_fog(void);
void game_set_fog(const unsigned char *visible,int count);
void game_redraw(void);
int game_player_view(void);
void game_toggle_view(void);
void game_view_selected(int player);
void game_view_pause(int pause);
void game_restore_view(void);
void game_reset_fog(void);
void game_command(int command);
#endif
