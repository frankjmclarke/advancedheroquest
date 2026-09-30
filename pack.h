#ifndef HQ_PACK_H
#define HQ_PACK_H
#include "game.h"
#define PACK_LIMIT 256
#define PACK_BYTES_LIMIT (2u*1024u*1024u)
/* Generic handlers, selected by the pack rather than the spell's display name. */
#define SPELL_MANUAL 0
#define SPELL_DAMAGE 1
#define SPELL_HEAL 2
#define SPELL_ARMOUR 3
#define SPELL_SPY 4
#define SPELL_DOOR 5
#define SPELL_HAND 6
#define SPELL_FLIGHT 7
#define SPELL_SWIFT 8
#define SPELL_RESURRECT 9
#define SPELL_STILL 10
#define SPELL_COURAGE 11
#define SPELL_LIFE 12
#define SPELL_STRENGTH 13
#define SPELL_CLOAK 14
#define SPELL_BLIND 15
#define SPELL_REGEN 16
#define SPELL_FEAR 17
#define SPELL_SLEEP 18
#define SPELL_RESTORE 19
#define SPELL_LEARNING 20
#define SPELL_BANISH 21
#define SPELL_ESCAPE 22
#define SPELL_VENOM 23
#define TRAIT_UNDEAD 1
#define TRAIT_LESSER_DAEMON 2
#define TRAIT_GREATER_DAEMON 4
typedef struct { char id[64],name[64]; int price; } PACK_COMPONENT;
typedef struct {
    char id[64],name[64],text[2048];
    int effect,target,range,dice,test,failed_dice,stationary,cost;
    int template_width,template_height;
    unsigned short components[SPELL_COMPONENT_LIMIT];
} PACK_SPELL;
/* target: 0 manual, 1 model, 2 template, 3 healing, 4 touch,
 * 5 visible section, 6 hidden section, 7 wall, 8 self, 9 corpse, 10 group. */
typedef struct {
    char id[64],name[64];
    unsigned char spells[SPELL_LIMIT],starting[SPELL_LIMIT];
} PACK_SPELLBOOK;
typedef struct {
    char id[64],name[64],aliases[1024],text[4096];
    int stats[HERO_STATS],kind,fate,unique;
    MELEE_PROFILE melee;
    RANGED_PROFILE ranged;
    unsigned char books[SPELL_BOOK_LIMIT];
    unsigned short components[SPELL_COMPONENT_LIMIT];
    int starting_components;
    int traits;
} PACK_PROFILE;
typedef struct character_pack {
    char id[64],name[96];
    char nonmonsters[8192];
    int hero_count,monster_count,legacy_references;
    PACK_PROFILE *heroes,*monsters;
    unsigned char *bytes;
    size_t size;
    int component_count,spell_count,book_count;
    PACK_COMPONENT *components;
    PACK_SPELL *spells;
    PACK_SPELLBOOK *books;
    size_t magic_offset,profile_magic_offset;
} CHARACTER_PACK;
const CHARACTER_PACK *pack_fantasy(void);
const CHARACTER_PACK *pack_current(void);
const CHARACTER_PACK *pack_legacy_save(void);
CHARACTER_PACK *pack_decode(const unsigned char *bytes,size_t size);
CHARACTER_PACK *pack_clone(const CHARACTER_PACK *pack);
CHARACTER_PACK *pack_extend_light(const CHARACTER_PACK *pack);
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
void pack_caster_defaults(CASTER_STATE *state,const PACK_PROFILE *profile,const CHARACTER_PACK *pack);
int pack_caster_valid(const CASTER_STATE *state,const CHARACTER_PACK *pack);
CHARACTER_PACK *pack_upgrade_magic(const CHARACTER_PACK *pack);
void pack_shutdown(void);
#endif
