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
typedef struct {
    char title[160];
    int width, height, count, hero_count;
    HERO heroes[HERO_LIMIT];
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
int game_save(int save_as);
int game_load(int recovery);
int game_before_replace(void);
void game_new_dungeon(const char *title);
void game_discard_dungeon(void);
void game_changed(void);
void game_recover(void);
void game_attach(WINDOW_DEF *window,int player);
void game_draw(WINDOW_DEF *window,const unsigned char *visible,int zoom,int sx,int sy);
/* Fog belongs to the dungeon, including when its window is closed. */
unsigned char *game_fog(void);
void game_set_fog(const unsigned char *visible,int count);
void game_redraw(void);
void game_reset_fog(void);
void game_command(int command);
#endif
