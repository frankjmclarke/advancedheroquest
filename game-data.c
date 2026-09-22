/* Versioned, bounded session codec. No pointers or compiler struct layouts go
 * into save files. Parsing and validation finish before live state changes. */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "game.h"
#define MAX_SAVE (16u*1024u*1024u)
const char *hero_classes[HERO_CLASS_COUNT]={"Warrior","Dwarf","Elf","Wizard","Henchman","Warrior Priest","Rogue","Fighter","Mage"};
/* Reference summaries from ahqHeros.pdf, pages 1-4. Abilities are adjudicated
 * by players; Fate is not a remaining-healing or remaining-rerolls counter. */
const char *hero_class_rules(int kind)
{
    switch(kind) {
    case 5: return "Warrior Priest\n\nStarting adjustment: Strength -1, Bravery +1.\n\n"
        "Healing spells equal total Fate points (two for a beginning priest).\n\n"
        "Holding a holy symbol in one hand can frighten Undead in the priest's death zone: at the start of their turn they must pass a Bravery roll, modified by the priest's total Fate, or be unable to act. The other hand may hold a one-handed weapon or shield.\n\n"
        "Only crushing or bruising weapons; no blades.\n\nApply abilities manually during play.";
    case 6: return "Rogue\n\nStarting adjustments: WS -1, Bravery -1, Speed +1, BS +1.\n\n"
        "+1 to rolls for secret doors, treasure, spotting traps and surprise. With thieves' tools, add total Fate to disarm-trap rolls (initially +2).\n\n"
        "Tools also allow locking/unlocking doors or chests and setting traps, rolling against current Speed.\n\n"
        "No more than +1 from any single armour piece; shields allowed.\n\nApply abilities manually during play.";
    case 7: return "Fighter\n\nUse the racial profile. If WS is below 7, add 1; if Strength is below 4, add 1. Deduct the same total from Intelligence and/or Speed.\n\n"
        "Attack rerolls per expedition equal total Fate. Each rerolls one attack die just rolled.\n\n"
        "Optional berserker: Intelligence -2, BS -2, WS -1 only if this would not take WS below 7. Record the choice in the hero's name and edit the stats; use the rulebook's berserker rules.\n\n"
        "Any weapons and armour permitted. Apply abilities manually during play.";
    case 8: return "Mage\n\nUse the racial profile. If Intelligence is below 7, you may add up to 2, deducting the same total from Strength and/or WS. This preset uses a human suggestion with WS -1 and Intelligence +1.\n\n"
        "Human and Elf mages start with their college's first four spells. Dwarf mages start with one chosen spell and pay double to learn new spells.\n\n"
        "No armour; only daggers or a staff in combat.\n\nEdit stats for your chosen race and apply spells manually during play.";
    default: return "Editable starting suggestions for this hero type. Use your character-generation and equipment rules to adjust the profile. Class abilities are handled manually during play.";
    }
}
void hero_defaults(HERO *h,int kind,int number)
{
    /* Editable suggestions, not automatic character creation or class rules.
       WS BS S T Sp Br Int W(max) PV. */
    static const int values[HERO_CLASS_COUNT][HERO_STATS]={
        {7,6,6,6,8,8,6,4,0},{7,5,6,7,6,9,6,4,0},
        {6,8,5,5,9,8,8,4,0},{4,5,4,5,8,7,9,4,0},
        {5,5,5,5,8,7,5,2,0},
        /* Human suggestions: existing Warrior base plus ahqHeros adjustments.
           Mage trades one WS for one Int; Fighter needs no base adjustment. */
        {7,6,5,6,8,9,6,4,0},{6,7,6,6,9,7,6,4,0},
        {7,6,6,6,8,8,6,4,0},{6,6,6,6,8,8,7,4,0}};
    memset(h,0,sizeof(*h)); h->kind=kind;
    sprintf(h->name,"%s %d",hero_classes[kind],number);
    memcpy(h->stats,values[kind],sizeof(h->stats));
    h->wounds=h->stats[7]; h->fate=kind==4?0:2; h->x=h->y=-1;
}
static int valid_type(int t)
{
    return strchr(" PELRTCODSNHXAQBM*$#",t)!=NULL && t!=0;
}
int game_valid_square(const GAME_DATA *d,int hero,int x,int y)
{
    int i,index;
    if(x<0 || y<0 || x>=d->width || y>=d->height) return 0;
    index=d->cells[y*d->width+x];
    if(index<0 || index>=d->count || d->pieces[index].type==EMPTY || d->pieces[index].type==TEST) return 0;
    for(i=0;i<d->hero_count;i++) if(i!=hero && d->heroes[i].x==x && d->heroes[i].y==y) return 0;
    return 1;
}
static int valid_data(const GAME_DATA *d)
{
    int i,j;
    if(d->width<1 || d->height<1 || d->width>240 || d->height>240 || d->count<1 || d->count>8192 || d->hero_count<0 || d->hero_count>HERO_LIMIT) return 0;
    if(!d->pieces || !d->cells || !d->visible) return 0;
    for(i=0;i<d->count;i++) {
        PICE *p=&d->pieces[i];
        if(!valid_type(p->type) || d->visible[i]>1) return 0;
        if(p->type==EMPTY) continue;
        if(p->feature<0 || p->feature>=Illegal_Feature || p->pos<North || p->pos>West ||
            p->x<0 || p->y<0 || p->w<1 || p->h<1 || p->x+p->w>d->width || p->y+p->h>d->height) return 0;
    }
    for(i=0;i<d->width*d->height;i++) {
        int n=d->cells[i]; PICE *p;
        if(n < -1 || n>=d->count) return 0;
        if(n<0) continue;
        p=&d->pieces[n];
        if(p->type==EMPTY || i%d->width<p->x || i%d->width>=p->x+p->w || i/d->width<p->y || i/d->width>=p->y+p->h) return 0;
    }
    for(i=0;i<d->hero_count;i++) {
        const HERO *h=&d->heroes[i];
        if(!memchr(h->name,0,sizeof(h->name)) || !h->name[0] || h->kind<0 || h->kind>=HERO_CLASS_COUNT || h->wounds<0 || h->wounds>h->stats[7] || h->fate<0 || h->fate>99) return 0;
        for(j=0;j<HERO_STATS;j++) if(h->stats[j]<0 || h->stats[j]>99) return 0;
        if(h->x==-1 && h->y==-1) continue;
        if(!game_valid_square(d,i,h->x,h->y)) return 0;
    }
    return 1;
}
typedef struct { unsigned char *p; size_t n,cap,pos; int ok; } BYTES;
static void put(BYTES *b,const void *p,size_t n)
{
    unsigned char *next; size_t cap;
    if(!b->ok || n>MAX_SAVE || b->n>MAX_SAVE-n) { b->ok=0; return; }
    if(b->n+n>b->cap) {
        cap=(b->n+n)*2; if(cap>MAX_SAVE) cap=MAX_SAVE;
        next=(unsigned char*)realloc(b->p,cap); if(!next) { b->ok=0; return; }
        b->p=next; b->cap=cap;
    }
    memcpy(b->p+b->n,p,n); b->n+=n;
}
static void put32(BYTES *b,int value)
{
    uint32_t v=(uint32_t)value; unsigned char p[4]; int i;
    for(i=0;i<4;i++) p[i]=(unsigned char)(v>>(8*i)); put(b,p,4);
}
static int get32(BYTES *b)
{
    uint32_t v=0; int i;
    if(!b->ok || b->pos+4>b->n) { b->ok=0; return 0; }
    for(i=0;i<4;i++) v|=(uint32_t)b->p[b->pos++]<<(8*i);
    return (int32_t)v;
}
static uint32_t checksum(const unsigned char *p,size_t n)
{
    uint32_t crc=0xffffffff; size_t i; int j;
    for(i=0;i<n;i++) { crc^=p[i]; for(j=0;j<8;j++) crc=(crc>>1)^((crc&1)?0xedb88320:0); }
    return ~crc;
}
static void put_string(BYTES *b,const char *s,size_t limit)
{
    size_t n=s?strlen(s):0;
    if(n>limit) { b->ok=0; return; } put32(b,(int)n); put(b,s?s:"",n);
}
static char *get_string(BYTES *b,size_t limit)
{
    int n=get32(b); char *s;
    if(!b->ok || n<0 || (size_t)n>limit || b->pos+n>b->n) { b->ok=0; return NULL; }
    s=(char*)calloc(n+1,1); if(!s) { b->ok=0; return NULL; }
    memcpy(s,b->p+b->pos,n); b->pos+=n;
    if(memchr(s,0,n)) { free(s); b->ok=0; return NULL; } return s;
}
int game_encode(const GAME_DATA *d,unsigned char **bytes,size_t *size)
{
    BYTES b={0}; int i,j,n; uint32_t crc;
    *bytes=NULL; *size=0; if(!valid_data(d)) return 0; b.ok=1;
    put(&b,"HQGAME\r\n",8); put32(&b,1); put32(&b,0);
    put32(&b,d->width); put32(&b,d->height); put32(&b,d->count); put32(&b,d->hero_count);
    put_string(&b,d->title,159);
    for(i=0;i<d->hero_count;i++) {
        const HERO *h=&d->heroes[i]; put_string(&b,h->name,39); put32(&b,h->kind);
        for(j=0;j<HERO_STATS;j++) put32(&b,h->stats[j]);
        put32(&b,h->wounds); put32(&b,h->fate); put32(&b,h->x); put32(&b,h->y);
    }
    for(i=0;i<d->count;i++) {
        PICE *p=&d->pieces[i]; put32(&b,p->type); put32(&b,d->visible[i]);
        if(p->type==EMPTY) continue;
        put32(&b,p->feature); put32(&b,p->pos); put32(&b,p->x); put32(&b,p->y); put32(&b,p->w); put32(&b,p->h);
        n=0; if(p->text) while(p->text[n] && n<4096) ++n;
        if(n==4096) b.ok=0; put32(&b,n);
        for(j=0;j<n;j++) put_string(&b,p->text[j],65535);
    }
    for(i=0;i<d->width*d->height;i++) put32(&b,d->cells[i]);
    if(!b.ok) { free(b.p); return 0; }
    crc=checksum(b.p+16,b.n-16); for(i=0;i<4;i++) b.p[12+i]=(unsigned char)(crc>>(8*i));
    *bytes=b.p; *size=b.n; return 1;
}
void game_data_free(GAME_DATA *d)
{
    int i,j; if(!d) return;
    if(d->pieces) for(i=0;i<d->count;i++) {
        if(d->pieces[i].text) { for(j=0;d->pieces[i].text[j];j++) free(d->pieces[i].text[j]); free(d->pieces[i].text); }
    }
    free(d->pieces); free(d->cells); free(d->visible); free(d);
}
GAME_DATA *game_decode(const unsigned char *bytes,size_t size)
{
    BYTES b={0}; GAME_DATA *d; int i,j,n; char *s;
    if(size<32 || size>MAX_SAVE || memcmp(bytes,"HQGAME\r\n",8)) return NULL;
    b.p=(unsigned char*)bytes; b.n=size; b.pos=8; b.ok=1;
    if(get32(&b)!=1 || (uint32_t)get32(&b)!=checksum(bytes+16,size-16)) return NULL;
    d=(GAME_DATA*)calloc(1,sizeof(*d)); if(!d) return NULL;
    d->width=get32(&b); d->height=get32(&b); d->count=get32(&b); d->hero_count=get32(&b);
    if(d->width<1 || d->height<1 || d->width>240 || d->height>240 || d->count<1 || d->count>8192 || d->hero_count<0 || d->hero_count>HERO_LIMIT) { free(d); return NULL; }
    d->pieces=(PICE*)calloc(d->count,sizeof(PICE)); d->cells=(int*)calloc(d->width*d->height,sizeof(int)); d->visible=(unsigned char*)calloc(d->count,1);
    if(!d->pieces || !d->cells || !d->visible) goto bad;
    s=get_string(&b,159); if(!s) goto bad; strcpy(d->title,s); free(s);
    for(i=0;i<d->hero_count && b.ok;i++) {
        HERO *h=&d->heroes[i]; s=get_string(&b,39); if(!s) goto bad; strcpy(h->name,s); free(s);
        h->kind=get32(&b); for(j=0;j<HERO_STATS;j++) h->stats[j]=get32(&b);
        h->wounds=get32(&b); h->fate=get32(&b); h->x=get32(&b); h->y=get32(&b);
    }
    for(i=0;i<d->count && b.ok;i++) {
        PICE *p=&d->pieces[i]; p->type=get32(&b); n=get32(&b); if(n<0 || n>1) goto bad; d->visible[i]=(unsigned char)n;
        if(p->type==EMPTY) continue;
        p->feature=get32(&b); p->pos=get32(&b);
        n=get32(&b); if(n<0 || n>240) goto bad; p->x=(_WORD)n;
        n=get32(&b); if(n<0 || n>240) goto bad; p->y=(_WORD)n;
        n=get32(&b); if(n<1 || n>240) goto bad; p->w=(_WORD)n;
        n=get32(&b); if(n<1 || n>240) goto bad; p->h=(_WORD)n;
        n=get32(&b); if(n<0 || n>4095 || !b.ok) goto bad;
        /* A piece without text must retain the NULL representation used by
           generated maps and by the room-number renderer. */
        if(n==0) continue;
        p->text=(_UBYTE**)calloc(n+1,sizeof(char*)); if(!p->text) goto bad;
        for(j=0;j<n;j++) { p->text[j]=get_string(&b,65535); if(!b.ok) goto bad; }
    }
    for(i=0;i<d->width*d->height && b.ok;i++) d->cells[i]=get32(&b);
    if(!b.ok || b.pos!=b.n || !valid_data(d)) goto bad;
    return d;
bad: game_data_free(d); return NULL;
}
