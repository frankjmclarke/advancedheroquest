/* Versioned, bounded session codec. No pointers or compiler struct layouts go
 * into save files. Parsing and validation finish before live state changes. */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "game.h"
#include "liste.h"
#include "pack.h"
#define MAX_SAVE (16u*1024u*1024u)
const char *hero_class_rules(int kind)
{
    const CHARACTER_PACK *p=pack_fantasy();
    return kind>=0 && kind<p->hero_count?p->heroes[kind].text:"";
}
void hero_melee_defaults(MELEE_PROFILE *p,int kind)
{
    const CHARACTER_PACK *pack=pack_fantasy(); memset(p,0,sizeof(*p));
    if(kind>=0 && kind<pack->hero_count) *p=pack->heroes[kind].melee;
}
void hero_ranged_defaults(RANGED_PROFILE *p,int kind)
{
    const CHARACTER_PACK *pack=pack_fantasy(); memset(p,0,sizeof(*p));
    if(kind>=0 && kind<pack->hero_count) *p=pack->heroes[kind].ranged;
}
int monster_ranged_defaults(RANGED_PROFILE *p,const char *name)
{
    RANGED_PROFILE next={0};
    const PACK_PROFILE *entry=pack_profile(name);
    if(entry) { *p=entry->ranged; return 1; }
    if(!room_ranged(name,&next.range,&next.dice,next.hit,&next.kind)) return 0;
    strcpy(next.weapon,"Printed ranged profile"); next.critical=12; next.fumble=1; *p=next; return 1;
}
static int melee_blank(const MELEE_PROFILE *p)
{
    int i; if(p->weapon[0] || p->dice) return 0;
    for(i=0;i<12;i++) if(p->hit[i]) return 0;
    return 1;
}
void hero_defaults(HERO *h,int kind,int number)
{
    const CHARACTER_PACK *p=pack_fantasy();
    if(kind>=0 && kind<p->hero_count) pack_hero(h,&p->heroes[kind],number);
}
const char *monster_identity(const MONSTER *m) { return m->character_id[0]?m->character_id:m->name; }
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
    for(i=0;i<d->monsters.count;i++) if(d->monsters.tokens[i].x==x && d->monsters.tokens[i].y==y) return 0;
    return 1;
}
static int valid_ranged(const RANGED_PROFILE *p,int moved,int focus,int opponents)
{
    int i;
    if(!memchr(p->weapon,0,40) || p->kind<0 || p->kind>4 || p->range<0 || p->range>480 || p->dice<0 || p->dice>99 ||
       (p->critical && (p->critical<1 || p->critical>12)) || (p->fumble && (p->fumble<1 || p->fumble>= (p->critical?p->critical:12))) || moved<0 || moved>1 || focus<0 || focus>opponents) return 0;
    for(i=0;i<5;i++) if(p->hit[i]<0 || p->hit[i]>12) return 0;
    return 1;
}
static int valid_melee(const MELEE_PROFILE *p)
{
    int i,critical=p->critical?p->critical:12,fumble=p->fumble?p->fumble:1;
    if(!memchr(p->weapon,0,sizeof(p->weapon)) || p->dice<0 || p->dice>99 || critical<1 || critical>12 || fumble<1 || fumble>=critical || (p->reach && (p->reach<1 || p->reach>2))) return 0;
    if(p->fumble && p->critical && p->fumble>=p->critical) return 0;
    for(i=0;i<12;i++) if(p->hit[i]<0 || p->hit[i]>12) return 0;
    return 1;
}
static int valid_data(const GAME_DATA *d)
{
    int i,j;
    if(d->turn_phase<0 || d->turn_phase>2 || d->gm_override<0 || d->gm_override>1) return 0;
    if(d->width<1 || d->height<1 || d->width>240 || d->height>240 || d->count<1 || d->count>8192 || d->hero_count<0 || d->hero_count>HERO_LIMIT) return 0;
    if(!d->pieces || !d->cells || !d->visible || d->player_view < -1 || d->player_view > 1) return 0;
    if(d->monsters.count<0 || d->monsters.count>MONSTER_LIMIT || d->monsters.dead_count<0 || d->monsters.dead_count>CHARACTER_LIMIT) return 0;
    for(i=0;i<d->count;i++) if(d->monsters.seen[i]>2) return 0;
    for(i=0;i<d->monsters.dead_count;i++) {
        if(!memchr(d->monsters.dead[i],0,64) || !d->monsters.dead[i][0]) return 0;
        for(j=0;j<i;j++) if(!_stricmp(d->monsters.dead[i],d->monsters.dead[j])) return 0;
    }
    for(i=0;i<d->monsters.count;i++) {
        const MONSTER *m=&d->monsters.tokens[i]; int cell;
        if(!memchr(m->profile_id,0,sizeof(m->profile_id)) || !memchr(m->character_id,0,sizeof(m->character_id))) return 0;
        if(!valid_ranged(&m->ranged,m->moved,m->focus,d->hero_count) || !valid_melee(&m->melee) || !memchr(m->name,0,64) || !m->name[0] || m->room<0 || m->room>=d->count ||
           m->unique<0 || m->unique>1 || m->wounds<0 || m->wounds>m->stats[7] || m->stats[7]<1) return 0;
        if(m->move_spent<0 || m->move_spent>1000000 || m->attacked<0 || m->attacked>1 || m->run_bonus<0 || m->run_bonus>12) return 0;
        for(j=0;j<HERO_STATS;j++) if(m->stats[j]<0 || m->stats[j]>(j==8?9999:99)) return 0;
        if(!d->visible[m->room] || !d->monsters.seen[m->room] || d->pieces[m->room].type==EMPTY) return 0;
        for(j=0;j<d->monsters.dead_count;j++) if(m->wounds && !_stricmp(m->character_id[0]?m->character_id:m->name,d->monsters.dead[j])) return 0;
        for(j=0;j<i;j++) if(m->wounds && d->monsters.tokens[j].wounds && (m->unique || d->monsters.tokens[j].unique) && !_stricmp(monster_identity(m),monster_identity(&d->monsters.tokens[j]))) return 0;
        if(m->unique && !m->wounds) {
            for(j=0;j<d->monsters.dead_count;j++) if(!_stricmp(m->character_id[0]?m->character_id:m->name,d->monsters.dead[j])) break;
            if(j==d->monsters.dead_count) return 0;
        }
        if(m->x==-1 && m->y==-1) continue;
        if(!m->wounds || m->x<0 || m->y<0 || m->x>=d->width || m->y>=d->height) return 0;
        cell=d->cells[m->y*d->width+m->x];
        if(cell<0 || cell>=d->count || !d->visible[cell] || d->pieces[cell].type==EMPTY || d->pieces[cell].type==TEST) return 0;
        for(j=0;j<i;j++) if(d->monsters.tokens[j].x==m->x && d->monsters.tokens[j].y==m->y) return 0;
    }
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
        if(!memchr(h->profile_id,0,sizeof(h->profile_id)) || !memchr(h->class_name,0,sizeof(h->class_name)) || !memchr(h->class_rules,0,sizeof(h->class_rules))) return 0;
        if(h->fired<0 || h->fired>1 || h->condition<0 || h->condition>2 || !valid_ranged(&h->ranged,h->moved,h->focus,d->monsters.count) || !valid_melee(&h->melee) || !memchr(h->name,0,sizeof(h->name)) || !h->name[0] || h->kind<0 || h->kind>=HERO_CLASS_COUNT || h->wounds<0 || h->wounds>h->stats[7] || h->fate<0 || h->fate>99) return 0;
        if(h->move_spent<0 || h->move_spent>1000000 || h->attacked<0 || h->attacked>1 || h->run_bonus<0 || h->run_bonus>12) return 0;
        if((h->condition==1 && h->wounds!=0) || (h->condition==2 && h->wounds!=0)) return 0;
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
static void put_melee(BYTES *b,const MELEE_PROFILE *p)
{
    int j; put_string(b,p->weapon,39); put32(b,p->dice);
    for(j=0;j<12;j++) put32(b,p->hit[j]);
}
static void get_melee(BYTES *b,MELEE_PROFILE *p)
{
    int j; char *s=get_string(b,39); if(!s) return;
    strcpy(p->weapon,s); free(s); p->dice=get32(b);
    for(j=0;j<12;j++) p->hit[j]=get32(b);
    p->critical=12; p->fumble=1;
}
static void put_ranged(BYTES *b,const RANGED_PROFILE *p,int moved,int focus)
{
    int j; put_string(b,p->weapon,39); put32(b,p->kind); put32(b,p->range); put32(b,p->dice);
    for(j=0;j<5;j++) put32(b,p->hit[j]); put32(b,moved); put32(b,focus);
}
static void get_ranged(BYTES *b,RANGED_PROFILE *p,int *moved,int *focus)
{
    int j; char *s=get_string(b,39); if(!s) return; strcpy(p->weapon,s); free(s);
    p->kind=get32(b); p->range=get32(b); p->dice=get32(b);
    for(j=0;j<5;j++) p->hit[j]=get32(b); *moved=get32(b); *focus=get32(b);
}
int game_encode(const GAME_DATA *d,unsigned char **bytes,size_t *size)
{
    BYTES b={0}; int i,j,n; uint32_t crc;
    *bytes=NULL; *size=0; if(!valid_data(d)) return 0; b.ok=1;
    put(&b,"HQGAME\r\n",8); put32(&b,10); put32(&b,0);
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
    put32(&b,d->monsters.count); put32(&b,d->monsters.dead_count);
    for(i=0;i<d->count;i++) put32(&b,d->monsters.seen[i]);
    for(i=0;i<d->monsters.count;i++) {
        const MONSTER *m=&d->monsters.tokens[i]; put_string(&b,m->name,63);
        for(j=0;j<HERO_STATS;j++) put32(&b,m->stats[j]);
        put32(&b,m->wounds); put32(&b,m->x); put32(&b,m->y); put32(&b,m->room); put32(&b,m->unique);
    }
    for(i=0;i<d->monsters.dead_count;i++) put_string(&b,d->monsters.dead[i],63);
    put32(&b,d->player_view);
    for(i=0;i<d->hero_count;i++) put_melee(&b,&d->heroes[i].melee);
    for(i=0;i<d->monsters.count;i++) put_melee(&b,&d->monsters.tokens[i].melee);
    for(i=0;i<d->hero_count;i++) put_ranged(&b,&d->heroes[i].ranged,d->heroes[i].moved,d->heroes[i].focus);
    for(i=0;i<d->monsters.count;i++) put_ranged(&b,&d->monsters.tokens[i].ranged,d->monsters.tokens[i].moved,d->monsters.tokens[i].focus);
    for(i=0;i<d->hero_count;i++) put32(&b,d->heroes[i].fired);
    {
        const CHARACTER_PACK *p=d->pack?d->pack:pack_fantasy();
        put32(&b,(int)p->size); put(&b,p->bytes,p->size);
        for(i=0;i<d->hero_count;i++) {
            put_string(&b,d->heroes[i].profile_id,63);
            put_string(&b,d->heroes[i].class_name,63);
            put_string(&b,d->heroes[i].class_rules,2047);
        }
        for(i=0;i<d->monsters.count;i++) {
            put_string(&b,d->monsters.tokens[i].profile_id,63);
            put_string(&b,d->monsters.tokens[i].character_id,63);
        }
    }
    for(i=0;i<d->hero_count;i++) { const MELEE_PROFILE *p=&d->heroes[i].melee; put32(&b,p->critical?p->critical:12); put32(&b,p->fumble?p->fumble:1); put32(&b,p->reach?p->reach:1); }
    for(i=0;i<d->monsters.count;i++) { const MELEE_PROFILE *p=&d->monsters.tokens[i].melee; put32(&b,p->critical?p->critical:12); put32(&b,p->fumble?p->fumble:1); put32(&b,p->reach?p->reach:1); }
    for(i=0;i<d->hero_count;i++) { const HERO *h=&d->heroes[i]; put32(&b,!h->wounds?(h->kind==4?2:h->condition==2?2:1):h->condition); }
    /* Append v9 ranged thresholds after the complete v8 suffix for migration. */
    for(i=0;i<d->hero_count;i++) { const RANGED_PROFILE *p=&d->heroes[i].ranged; put32(&b,p->critical?p->critical:12); put32(&b,p->fumble?p->fumble:1); }
    for(i=0;i<d->monsters.count;i++) { const RANGED_PROFILE *p=&d->monsters.tokens[i].ranged; put32(&b,p->critical?p->critical:12); put32(&b,p->fumble?p->fumble:1); }
    /* Version 10 adds guided combat phases and per-model action state. */
    put32(&b,d->turn_phase); put32(&b,d->gm_override);
    for(i=0;i<d->hero_count;i++) { const HERO *h=&d->heroes[i]; put32(&b,h->move_spent); put32(&b,h->attacked); put32(&b,h->run_bonus); }
    for(i=0;i<d->monsters.count;i++) { const MONSTER *m=&d->monsters.tokens[i]; put32(&b,m->move_spent); put32(&b,m->attacked); put32(&b,m->run_bonus); }
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
    free(d->pieces); free(d->cells); free(d->visible); pack_free(d->pack); free(d);
}
/* Older saves recorded revealed rooms, but not individual opened doors.
 * Infer old openings only when both sides were explored. Version 5 retains
 * each door independently, including closed doors between explored rooms. */
static void legacy_open_doors(GAME_DATA *d)
{
    int i,j,x,y,nx,ny,a,b; PICE *door,*p;
    for(i=0;i<d->count;i++) {
        door=&d->pieces[i]; if(door->type!=DOOR && door->type!=SECRET) continue;
        x=door->x; y=door->y;
        nx=x+(door->pos==East?1:door->pos==West?-1:0);
        ny=y+(door->pos==North?1:door->pos==South?-1:0); a=b=0;
        for(j=0;j<d->count;j++) {
            p=&d->pieces[j]; if(!d->visible[j] || p->type==EMPTY || p->type==TEST || p->type==DOOR || p->type==SECRET) continue;
            if(x>=p->x && x<p->x+p->w && y>=p->y && y<p->y+p->h) a=1;
            if(nx>=p->x && nx<p->x+p->w && ny>=p->y && ny<p->y+p->h) b=1;
        }
        d->visible[i]=(unsigned char)(a && b);
    }
}
GAME_DATA *game_decode(const unsigned char *bytes,size_t size)
{
    BYTES b={0}; GAME_DATA *d; int i,j,n,version; char *s;
    if(size<32 || size>MAX_SAVE || memcmp(bytes,"HQGAME\r\n",8)) return NULL;
    b.p=(unsigned char*)bytes; b.n=size; b.pos=8; b.ok=1;
    version=get32(&b);
    if((version<1 || version>10) || (uint32_t)get32(&b)!=checksum(bytes+16,size-16)) return NULL;
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
        h->condition=0;
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
    if(version>=2) {
        d->monsters.count=get32(&b); d->monsters.dead_count=get32(&b);
        if(d->monsters.count<0 || d->monsters.count>MONSTER_LIMIT || d->monsters.dead_count<0 || d->monsters.dead_count>CHARACTER_LIMIT) goto bad;
        for(i=0;i<d->count;i++) { n=get32(&b); if(n<0 || n>2) goto bad; d->monsters.seen[i]=(unsigned char)n; }
        for(i=0;i<d->monsters.count && b.ok;i++) {
            MONSTER *m=&d->monsters.tokens[i]; s=get_string(&b,63); if(!s) goto bad; strcpy(m->name,s); free(s);
            for(j=0;j<HERO_STATS;j++) m->stats[j]=get32(&b);
            m->wounds=get32(&b); m->x=get32(&b); m->y=get32(&b); m->room=get32(&b); m->unique=get32(&b);
        }
        for(i=0;i<d->monsters.dead_count && b.ok;i++) { s=get_string(&b,63); if(!s) goto bad; strcpy(d->monsters.dead[i],s); free(s); }
    }
    d->player_view=version>=3?get32(&b):-1;
    if(version>=4) {
        for(i=0;i<d->hero_count && b.ok;i++) get_melee(&b,&d->heroes[i].melee);
        for(i=0;i<d->monsters.count && b.ok;i++) get_melee(&b,&d->monsters.tokens[i].melee);
    }
    if(version>=5) {
        for(i=0;i<d->hero_count && b.ok;i++) get_ranged(&b,&d->heroes[i].ranged,&d->heroes[i].moved,&d->heroes[i].focus);
        for(i=0;i<d->monsters.count && b.ok;i++) get_ranged(&b,&d->monsters.tokens[i].ranged,&d->monsters.tokens[i].moved,&d->monsters.tokens[i].focus);
    }
    if(version>=6) for(i=0;i<d->hero_count && b.ok;i++) d->heroes[i].fired=get32(&b);
    if(version>=7) {
        n=get32(&b); if(!b.ok || n<0 || n>PACK_BYTES_LIMIT || (size_t)n>b.n-b.pos) goto bad;
        d->pack=pack_decode(b.p+b.pos,n); if(!d->pack) goto bad; b.pos+=n;
        for(i=0;i<d->hero_count;i++) {
            s=get_string(&b,63); if(!s) goto bad; strcpy(d->heroes[i].profile_id,s); free(s);
            s=get_string(&b,63); if(!s) goto bad; strcpy(d->heroes[i].class_name,s); free(s);
            s=get_string(&b,2047); if(!s) goto bad; strcpy(d->heroes[i].class_rules,s); free(s);
        }
        for(i=0;i<d->monsters.count;i++) {
            s=get_string(&b,63); if(!s) goto bad; strcpy(d->monsters.tokens[i].profile_id,s); free(s);
            s=get_string(&b,63); if(!s) goto bad; strcpy(d->monsters.tokens[i].character_id,s); free(s);
        }
    } else {
        /* Pre-v7 saves carry no campaign identity. Honor the campaign the
           player explicitly selected for this load; otherwise retain the
           historical Old World fallback. */
        d->pack=pack_clone(pack_legacy_save()); if(!d->pack) goto bad;
        for(i=0;i<d->hero_count;i++) {
            HERO *h=&d->heroes[i]; const PACK_PROFILE *p;
            if(h->kind<0 || h->kind>=HERO_CLASS_COUNT) goto bad;
            p=&d->pack->heroes[h->kind];
            strcpy(h->profile_id,p->id); strcpy(h->class_name,p->name); strcpy(h->class_rules,p->text);
        }
    }
    if(version>=8) {
        for(i=0;i<d->hero_count && b.ok;i++) { d->heroes[i].melee.critical=get32(&b); d->heroes[i].melee.fumble=get32(&b); n=get32(&b); d->heroes[i].melee.reach=version>=9?n:n+1; }
        for(i=0;i<d->monsters.count && b.ok;i++) { d->monsters.tokens[i].melee.critical=get32(&b); d->monsters.tokens[i].melee.fumble=get32(&b); n=get32(&b); d->monsters.tokens[i].melee.reach=version>=9?n:n+1; }
        for(i=0;i<d->hero_count && b.ok;i++) d->heroes[i].condition=get32(&b);
    } else for(i=0;i<d->hero_count;i++) d->heroes[i].condition=d->heroes[i].wounds?0:d->heroes[i].kind==4?2:1;
    if(version>=9) {
        for(i=0;i<d->hero_count && b.ok;i++) { d->heroes[i].ranged.critical=get32(&b); d->heroes[i].ranged.fumble=get32(&b); }
        for(i=0;i<d->monsters.count && b.ok;i++) { d->monsters.tokens[i].ranged.critical=get32(&b); d->monsters.tokens[i].ranged.fumble=get32(&b); }
    }
    if(version>=10) {
        d->turn_phase=get32(&b); d->gm_override=get32(&b);
        for(i=0;i<d->hero_count && b.ok;i++) { HERO *h=&d->heroes[i]; h->move_spent=get32(&b); h->attacked=get32(&b); h->run_bonus=get32(&b); }
        for(i=0;i<d->monsters.count && b.ok;i++) { MONSTER *m=&d->monsters.tokens[i]; m->move_spent=get32(&b); m->attacked=get32(&b); m->run_bonus=get32(&b); }
    }
    for(i=0;i<d->hero_count;i++) {
        if(!d->heroes[i].melee.critical) d->heroes[i].melee.critical=12;
        if(!d->heroes[i].melee.fumble) d->heroes[i].melee.fumble=1;
        if(!d->heroes[i].melee.reach) d->heroes[i].melee.reach=1;
        if(!d->heroes[i].ranged.critical) d->heroes[i].ranged.critical=12;
        if(!d->heroes[i].ranged.fumble) d->heroes[i].ranged.fumble=1;
    }
    for(i=0;i<d->monsters.count;i++) {
        if(!d->monsters.tokens[i].melee.critical) d->monsters.tokens[i].melee.critical=12;
        if(!d->monsters.tokens[i].melee.fumble) d->monsters.tokens[i].melee.fumble=1;
        if(!d->monsters.tokens[i].melee.reach) d->monsters.tokens[i].melee.reach=1;
        if(!d->monsters.tokens[i].ranged.critical) d->monsters.tokens[i].ranged.critical=12;
        if(!d->monsters.tokens[i].ranged.fumble) d->monsters.tokens[i].ranged.fumble=1;
    }
    if(!b.ok || b.pos!=b.n || !valid_data(d)) goto bad;
    if(version<5) {
        legacy_open_doors(d);
        for(i=0;i<d->hero_count;i++) hero_ranged_defaults(&d->heroes[i].ranged,d->heroes[i].kind);
        for(i=0;i<d->monsters.count;i++) {
            const PACK_PROFILE *found=pack_find_monster(d->pack,d->monsters.tokens[i].name);
            if(found) d->monsters.tokens[i].ranged=found->ranged;
        }
    }
    /* Upgrade only completely absent equipment, including first v4 saves.
     * Partial/manual profiles and all existing hero statistics are preserved. */
    for(i=0;i<d->hero_count;i++) if((version<7 || !strncmp(d->heroes[i].profile_id,"fantasy:",8)) && melee_blank(&d->heroes[i].melee))
        hero_melee_defaults(&d->heroes[i].melee,d->heroes[i].kind);
    return d;
bad: game_data_free(d); return NULL;
}
