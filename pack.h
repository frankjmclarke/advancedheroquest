#ifndef HQ_PACK_H
#define HQ_PACK_H
#include "game.h"
#define PACK_LIMIT 256
#define PACK_BYTES_LIMIT (2u*1024u*1024u)
typedef struct {
    char id[64],name[64],aliases[1024],text[4096];
    int stats[HERO_STATS],kind,fate,unique;
    MELEE_PROFILE melee;
    RANGED_PROFILE ranged;
} PACK_PROFILE;
typedef struct character_pack {
    char id[64],name[96];
    char nonmonsters[8192];
    int hero_count,monster_count,legacy_references;
    PACK_PROFILE *heroes,*monsters;
    unsigned char *bytes;
    size_t size;
} CHARACTER_PACK;
const CHARACTER_PACK *pack_fantasy(void);
const CHARACTER_PACK *pack_current(void);
CHARACTER_PACK *pack_decode(const unsigned char *bytes,size_t size);
CHARACTER_PACK *pack_clone(const CHARACTER_PACK *pack);
void pack_free(CHARACTER_PACK *pack);
void pack_adopt(CHARACTER_PACK *pack);
int pack_activate(const CHARACTER_PACK *pack);
int pack_start_dungeon(void);
/* A ;character-pack relative/path.hqp comment in the quest selects its pack. */
CHARACTER_PACK *pack_read_campaign(const char *quest,char error[256]);
int pack_select_campaign(const char *quest,char error[256]);
const PACK_PROFILE *pack_profile(const char *id);
int pack_nonmonster(const char *name);
int pack_monster_stats(int index,int stats[HERO_STATS]);
const PACK_PROFILE *pack_monster(const char *name);
const PACK_PROFILE *pack_find_monster(const CHARACTER_PACK *pack,const char *name);
void pack_hero(HERO *hero,const PACK_PROFILE *profile,int number);
void pack_shutdown(void);
#endif
