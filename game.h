#ifndef HQ_GAME_H
#define HQ_GAME_H
#include <map.h>
#include <window.h>
#define HERO_LIMIT 16
#define HERO_STATS 9
#define HERO_CLASS_COUNT 9
typedef struct {
    char name[40];
    int kind, stats[HERO_STATS], wounds, fate, x, y;
} HERO;
#define MONSTER_LIMIT 512
#define CHARACTER_LIMIT 256
#define ENCOUNTER_LIMIT 8192
typedef struct {
    char name[64];
    int stats[HERO_STATS], wounds, x, y, room, unique;
} MONSTER;
typedef struct {
    int count, dead_count;
    MONSTER tokens[MONSTER_LIMIT];
    char dead[CHARACTER_LIMIT][64];
    unsigned char seen[ENCOUNTER_LIMIT];
} MONSTER_STATE;
typedef struct {
    char title[160];
    int width, height, count, hero_count;
    int player_view; /* -1: legacy save with no recorded view; 0: GM; 1: player */
    HERO heroes[HERO_LIMIT];
    MONSTER_STATE monsters;
    PICE *pieces;
    int *cells;
    unsigned char *visible;
} GAME_DATA;
extern const char *hero_classes[HERO_CLASS_COUNT];
const char *hero_class_rules(int kind);
void hero_defaults(HERO *hero,int kind,int number);
int game_valid_square(const GAME_DATA *data,int hero,int x,int y);
int game_encode(const GAME_DATA *data,unsigned char **bytes,size_t *size);
GAME_DATA *game_decode(const unsigned char *bytes,size_t size);
void game_data_free(GAME_DATA *data);
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
void game_view_selected(int player);
void game_view_pause(int pause);
void game_restore_view(void);
void game_reset_fog(void);
void game_command(int command);
#endif
