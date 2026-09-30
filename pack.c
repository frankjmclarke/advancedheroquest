/* Runtime character registry. External resources and save snapshots use the
 * same bounded decoder; no executable rules or pointers are loaded from data. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "pack.h"
#include "pack-fantasy-data.h"
static CHARACTER_PACK *fantasy,*selected_pack,*active_pack;
typedef struct { const unsigned char *p; size_t size,pos; int ok; } PACK_READER;
static int number(PACK_READER *r,int max)
{
    unsigned long n;
    if(!r->ok || r->size-r->pos<4) { r->ok=0; return 0; }
    n=(unsigned long)r->p[r->pos] | ((unsigned long)r->p[r->pos+1]<<8) |
      ((unsigned long)r->p[r->pos+2]<<16) | ((unsigned long)r->p[r->pos+3]<<24);
    r->pos+=4; if(n>(unsigned long)max) { r->ok=0; return 0; } return (int)n;
}
static void string(PACK_READER *r,char *s,size_t cap)
{
    int n=number(r,(int)cap-1);
    if(!r->ok || (size_t)n>r->size-r->pos || memchr(r->p+r->pos,0,n)) { r->ok=0; return; }
    memcpy(s,r->p+r->pos,n); s[n]=0; r->pos+=n;
}
void pack_free(CHARACTER_PACK *p)
{
    if(p) { free(p->heroes); free(p->monsters); free(p->components); free(p->spells); free(p->books); free(p->bytes); free(p); }
}
CHARACTER_PACK *pack_decode(const unsigned char *bytes,size_t size)
{
    CHARACTER_PACK *p; PACK_READER r; int i,j,k,total,format2,format3,format4,format5; PACK_PROFILE *q;
    if(!bytes || size<24 || size>PACK_BYTES_LIMIT) return NULL;
    format5=!memcmp(bytes,"HQPACK5\n",8); format4=format5 || !memcmp(bytes,"HQPACK4\n",8); format3=format4 || !memcmp(bytes,"HQPACK3\n",8); format2=format3 || !memcmp(bytes,"HQPACK2\n",8);
    if(!format2 && memcmp(bytes,"HQPACK1\n",8)) return NULL;
    r.p=bytes; r.size=size; r.pos=8; r.ok=1;
    p=(CHARACTER_PACK*)calloc(1,sizeof(*p)); if(!p) return NULL;
    string(&r,p->id,sizeof(p->id)); string(&r,p->name,sizeof(p->name));
    string(&r,p->nonmonsters,sizeof(p->nonmonsters));
    p->legacy_references=number(&r,1);
    p->hero_count=number(&r,PACK_LIMIT); p->monster_count=number(&r,PACK_LIMIT);
    if(!r.ok || !*p->id || !*p->name || !p->hero_count) goto bad;
    p->heroes=(PACK_PROFILE*)calloc(p->hero_count,sizeof(PACK_PROFILE));
    p->monsters=(PACK_PROFILE*)calloc(p->monster_count+1,sizeof(PACK_PROFILE));
    if(!p->heroes || !p->monsters) goto bad;
    total=p->hero_count+p->monster_count;
    for(i=0;i<total && r.ok;i++) {
        q=i<p->hero_count?&p->heroes[i]:&p->monsters[i-p->hero_count];
        string(&r,q->id,sizeof(q->id)); string(&r,q->name,sizeof(q->name));
        string(&r,q->aliases,sizeof(q->aliases)); string(&r,q->text,sizeof(q->text));
        for(j=0;j<HERO_STATS;j++) q->stats[j]=number(&r,j==8 && i>=p->hero_count?9999:99);
        q->kind=number(&r,HERO_CLASS_COUNT-1); q->fate=number(&r,99); q->unique=number(&r,1);
        string(&r,q->melee.weapon,sizeof(q->melee.weapon)); q->melee.dice=number(&r,99);
        for(j=0;j<12;j++) q->melee.hit[j]=number(&r,12);
        q->melee.critical=12; q->melee.fumble=1; /* Preserve legacy pack behavior. */
        if(format2) {
            q->melee.critical=number(&r,12); q->melee.fumble=number(&r,12);
            q->melee.reach=format3?number(&r,2):number(&r,1)+1;
            if(q->melee.critical<1 || q->melee.fumble<1 || q->melee.fumble>=q->melee.critical) goto bad;
        }
        string(&r,q->ranged.weapon,sizeof(q->ranged.weapon)); q->ranged.kind=number(&r,4);
        q->ranged.range=number(&r,480); q->ranged.dice=number(&r,99);
        for(j=0;j<5;j++) q->ranged.hit[j]=number(&r,12);
        q->ranged.critical=format3?number(&r,12):12; q->ranged.fumble=format3?number(&r,12):1;
        if(q->ranged.critical<1 || q->ranged.fumble<1 || q->ranged.fumble>=q->ranged.critical) goto bad;
        if(!*q->name || !*q->aliases || strncmp(q->id,p->id,strlen(p->id)) || q->id[strlen(p->id)]!=':') goto bad;
        if((i<p->hero_count || !p->legacy_references) && !q->stats[7]) goto bad;
        if(i<p->hero_count && strlen(q->text)>=sizeof(((HERO*)0)->class_rules)) goto bad;
        if(strcspn(q->aliases,"|")>63) goto bad;
        for(k=0;k<i;k++) {
            PACK_PROFILE *prior=k<p->hero_count?&p->heroes[k]:&p->monsters[k-p->hero_count];
            if(!strcmp(q->id,prior->id)) goto bad;
        }
    }
    if(format4 && r.ok) {
        p->magic_offset=r.pos;
        p->component_count=number(&r,SPELL_COMPONENT_LIMIT); p->spell_count=number(&r,SPELL_LIMIT); p->book_count=number(&r,SPELL_BOOK_LIMIT);
        if(!r.ok) goto bad;
        p->components=(PACK_COMPONENT*)calloc(p->component_count+1,sizeof(PACK_COMPONENT));
        p->spells=(PACK_SPELL*)calloc(p->spell_count+1,sizeof(PACK_SPELL));
        p->books=(PACK_SPELLBOOK*)calloc(p->book_count+1,sizeof(PACK_SPELLBOOK));
        if(!p->components || !p->spells || !p->books) goto bad;
        for(i=0;i<p->component_count;i++) {
            string(&r,p->components[i].id,64); string(&r,p->components[i].name,64); p->components[i].price=number(&r,100000);
        }
        for(i=0;i<p->spell_count;i++) {
            PACK_SPELL *s=&p->spells[i]; string(&r,s->id,64); string(&r,s->name,64); string(&r,s->text,sizeof(s->text));
            s->effect=number(&r,11); s->target=number(&r,10); s->range=number(&r,480); s->dice=number(&r,99);
            s->test=number(&r,1); s->failed_dice=number(&r,99); s->stationary=number(&r,1); s->cost=number(&r,100000);
            if(s->target==2) s->template_width=s->template_height=2; /* original tile for HQPACK4 snapshots */
            k=0; for(j=0;j<p->component_count;j++) { s->components[j]=(unsigned short)number(&r,9999); k+=s->components[j]; }
            if(k>=2 && !s->stationary) goto bad;
            { const int target[]={0,0,0,4,6,7,8,1,10,9,5,4};
              if(s->effect>=3 && s->target!=target[s->effect]) goto bad;
              if(s->effect==SPELL_RESURRECT && !s->test) goto bad; }
            if((s->effect==SPELL_DAMAGE && (!s->dice || (s->target!=1 && s->target!=2))) || (s->effect==SPELL_HEAL && (s->target!=3 || s->test))) goto bad;
        }
        for(i=0;i<p->book_count;i++) {
            PACK_SPELLBOOK *book=&p->books[i]; string(&r,book->id,64); string(&r,book->name,64);
            for(j=0;j<p->spell_count;j++) book->spells[j]=(unsigned char)number(&r,1);
            for(j=0;j<p->spell_count;j++) { book->starting[j]=(unsigned char)number(&r,1); if(book->starting[j] && !book->spells[j]) goto bad; }
        }
        /* One namespace for profiles, spells, books and components. */
        for(i=0;i<total+p->component_count+p->spell_count+p->book_count;i++) {
            const char *id=i<total?(i<p->hero_count?p->heroes[i].id:p->monsters[i-p->hero_count].id):
                i<total+p->component_count?p->components[i-total].id:
                i<total+p->component_count+p->spell_count?p->spells[i-total-p->component_count].id:p->books[i-total-p->component_count-p->spell_count].id;
            const char *name=i<total?"profile":i<total+p->component_count?p->components[i-total].name:
                i<total+p->component_count+p->spell_count?p->spells[i-total-p->component_count].name:p->books[i-total-p->component_count-p->spell_count].name;
            if(!*name || strncmp(id,p->id,strlen(p->id)) || id[strlen(p->id)]!=':') goto bad;
            for(k=0;k<i;k++) {
                const char *prior=k<total?(k<p->hero_count?p->heroes[k].id:p->monsters[k-p->hero_count].id):
                    k<total+p->component_count?p->components[k-total].id:
                    k<total+p->component_count+p->spell_count?p->spells[k-total-p->component_count].id:p->books[k-total-p->component_count-p->spell_count].id;
                if(!strcmp(id,prior)) goto bad;
            }
        }
        p->profile_magic_offset=r.pos;
        for(i=0;i<total;i++) {
            int member=0; q=i<p->hero_count?&p->heroes[i]:&p->monsters[i-p->hero_count];
            for(j=0;j<p->book_count;j++) { q->books[j]=(unsigned char)number(&r,1); member|=q->books[j]; }
            q->starting_components=number(&r,9999); if(q->starting_components && !member) goto bad;
            for(j=0;j<p->component_count;j++) { q->components[j]=(unsigned short)number(&r,9999); if(q->components[j] && !member) goto bad; }
        }
    }
    if(format5) for(i=0;i<p->spell_count;i++) {
        PACK_SPELL *s=&p->spells[i];
        s->template_width=number(&r,12); s->template_height=number(&r,12);
        if(s->target==2?(!s->template_width || !s->template_height):(s->template_width || s->template_height)) goto bad;
    }
    if(!r.ok || r.pos!=r.size) goto bad;
    p->bytes=(unsigned char*)malloc(size); if(!p->bytes) goto bad;
    memcpy(p->bytes,bytes,size); p->size=size; return p;
bad: pack_free(p); return NULL;
}
const CHARACTER_PACK *pack_fantasy(void)
{
    if(!fantasy) fantasy=pack_decode(fantasy_pack_bytes,sizeof(fantasy_pack_bytes));
    return fantasy;
}
const CHARACTER_PACK *pack_current(void) { return active_pack?active_pack:pack_fantasy(); }
typedef struct { unsigned char *bytes; size_t size,pos; int ok; } PACK_WRITER;
static void pack_write_number(PACK_WRITER *w,int value)
{
    int i; if(!w->ok || w->size-w->pos<4) { w->ok=0; return; }
    for(i=0;i<4;i++) w->bytes[w->pos++]=(unsigned char)((unsigned int)value>>(8*i));
}
static void pack_write_string(PACK_WRITER *w,const char *text)
{
    size_t n=strlen(text); pack_write_number(w,(int)n);
    if(!w->ok || n>w->size-w->pos) { w->ok=0; return; }
    memcpy(w->bytes+w->pos,text,n); w->pos+=n;
}
static CHARACTER_PACK *pack_upgrade_profile_layout(const CHARACTER_PACK *old)
{
    PACK_WRITER w; CHARACTER_PACK *next; int i,j,total=old->hero_count+old->monster_count;
    w.size=old->size+20*total; if(w.size>PACK_BYTES_LIMIT) return NULL;
    w.bytes=(unsigned char*)malloc(w.size); if(!w.bytes) return NULL; w.pos=8; w.ok=1; memcpy(w.bytes,"HQPACK3\n",8);
    pack_write_string(&w,old->id); pack_write_string(&w,old->name); pack_write_string(&w,old->nonmonsters);
    pack_write_number(&w,old->legacy_references); pack_write_number(&w,old->hero_count); pack_write_number(&w,old->monster_count);
    for(i=0;i<total;i++) {
        const PACK_PROFILE *q=i<old->hero_count?&old->heroes[i]:&old->monsters[i-old->hero_count];
        pack_write_string(&w,q->id); pack_write_string(&w,q->name); pack_write_string(&w,q->aliases); pack_write_string(&w,q->text);
        for(j=0;j<HERO_STATS;j++) pack_write_number(&w,q->stats[j]);
        pack_write_number(&w,q->kind); pack_write_number(&w,q->fate); pack_write_number(&w,q->unique);
        pack_write_string(&w,q->melee.weapon); pack_write_number(&w,q->melee.dice); for(j=0;j<12;j++) pack_write_number(&w,q->melee.hit[j]);
        pack_write_number(&w,q->melee.critical); pack_write_number(&w,q->melee.fumble); pack_write_number(&w,q->melee.reach?q->melee.reach:1);
        pack_write_string(&w,q->ranged.weapon); pack_write_number(&w,q->ranged.kind); pack_write_number(&w,q->ranged.range); pack_write_number(&w,q->ranged.dice);
        for(j=0;j<5;j++) pack_write_number(&w,q->ranged.hit[j]); pack_write_number(&w,q->ranged.critical); pack_write_number(&w,q->ranged.fumble);
    }
    next=w.ok?pack_decode(w.bytes,w.pos):NULL; free(w.bytes); return next;
}
/* Upgrade only previously manual, built-in Bright handlers. Keep slot order,
 * ingredient references and every saved character definition unchanged. */
static CHARACTER_PACK *pack_upgrade_bright(const CHARACTER_PACK *old,const CHARACTER_PACK *current)
{
    PACK_READER r={old->bytes,old->size,old->magic_offset+12,1}; PACK_WRITER w;
    char ignored[2048],id[64]; int i,j,k; CHARACTER_PACK *next; size_t size=old->size;
    const PACK_SPELL *replacement[SPELL_LIMIT]={0};
    for(i=0;i<old->spell_count;i++) if(old->spells[i].effect==SPELL_MANUAL) {
        for(j=0;j<current->spell_count;j++) if(!strcmp(old->spells[i].id,current->spells[j].id) && current->spells[j].effect>=SPELL_ARMOUR) {
            replacement[i]=&current->spells[j]; size-=strlen(old->spells[i].text); size+=strlen(replacement[i]->text); break;
        }
    }
    if(size>PACK_BYTES_LIMIT) return NULL;
    w.bytes=(unsigned char*)malloc(size); if(!w.bytes) return NULL; w.size=size; w.ok=1;
    for(i=0;i<old->component_count;i++) { string(&r,ignored,64); string(&r,ignored,64); number(&r,100000); }
    w.pos=r.pos; memcpy(w.bytes,old->bytes,w.pos);
    for(i=0;i<old->spell_count;i++) {
        const PACK_SPELL *saved=&old->spells[i],*updated=replacement[i];
        string(&r,id,64); string(&r,ignored,64); string(&r,ignored,sizeof(ignored));
        pack_write_string(&w,saved->id); pack_write_string(&w,saved->name); pack_write_string(&w,updated?updated->text:saved->text);
        for(k=0;k<8+old->component_count;k++) {
            int value=number(&r,100000);
            if(updated && k<3) value=k==0?updated->effect:k==1?updated->target:updated->range;
            pack_write_number(&w,value);
        }
    }
    if(r.ok && w.ok && old->size-r.pos<=w.size-w.pos) {
        memcpy(w.bytes+w.pos,old->bytes+r.pos,old->size-r.pos); w.pos+=old->size-r.pos;
    } else w.ok=0;
    next=w.ok?pack_decode(w.bytes,w.pos):NULL; free(w.bytes); return next;
}
/* Add the current spell catalogue to pre-magic fantasy snapshots, preserving
 * every saved hero/monster definition. No ingredient stock is reconstructed. */
CHARACTER_PACK *pack_upgrade_magic(const CHARACTER_PACK *old)
{
    const CHARACTER_PACK *f=pack_fantasy(); CHARACTER_PACK *next; unsigned char *bytes; size_t defs,stride,size,pos; int i,j,k;
    if(!old || strcmp(old->id,"fantasy") || !f) return pack_clone(old);
    if(old->magic_offset) return pack_upgrade_bright(old,f);
    if(memcmp(old->bytes,"HQPACK3\n",8)) {
        CHARACTER_PACK *layout=pack_upgrade_profile_layout(old); if(!layout) return NULL;
        next=pack_upgrade_magic(layout); pack_free(layout); return next;
    }
    defs=f->profile_magic_offset-f->magic_offset; stride=4*(f->book_count+1+f->component_count);
    size=old->size+defs+stride*(old->hero_count+old->monster_count)+8*f->spell_count; if(size>PACK_BYTES_LIMIT) return NULL;
    bytes=(unsigned char*)calloc(size,1); if(!bytes) return NULL;
    memcpy(bytes,old->bytes,old->size); memcpy(bytes,"HQPACK5\n",8);
    memcpy(bytes+old->size,f->bytes+f->magic_offset,defs); pos=old->size+defs;
    for(i=0;i<old->hero_count+old->monster_count;i++,pos+=stride) {
        const char *id=i<old->hero_count?old->heroes[i].id:old->monsters[i-old->hero_count].id;
        for(j=0;j<f->hero_count+f->monster_count;j++) {
            const char *match=j<f->hero_count?f->heroes[j].id:f->monsters[j-f->hero_count].id;
            if(!strcmp(id,match)) { memcpy(bytes+pos,f->bytes+f->profile_magic_offset+j*stride,stride); break; }
        }
        /* Legacy monsters have unknown remaining stocks. */
        if(i>=old->hero_count) for(k=f->book_count;k<f->book_count+1+f->component_count;k++) memset(bytes+pos+4*k,0,4);
    }
    for(i=0;i<f->spell_count;i++) {
        int dimensions[2]={f->spells[i].template_width,f->spells[i].template_height};
        for(j=0;j<2;j++) for(k=0;k<4;k++) bytes[pos++]=(unsigned char)((unsigned int)dimensions[j]>>(8*k));
    }
    next=pack_decode(bytes,size); free(bytes); return next;
}
const CHARACTER_PACK *pack_legacy_save(void) { return selected_pack?selected_pack:pack_fantasy(); }
CHARACTER_PACK *pack_clone(const CHARACTER_PACK *p) { return p?pack_decode(p->bytes,p->size):NULL; }
void pack_adopt(CHARACTER_PACK *p) { pack_free(active_pack); active_pack=p; }
int pack_activate(const CHARACTER_PACK *p)
{
    CHARACTER_PACK *next=pack_clone(p); if(!next) return 0;
    pack_free(active_pack); active_pack=next; return 1;
}
int pack_start_dungeon(void) { return pack_activate(selected_pack?selected_pack:pack_fantasy()); }
/* Quest-relative paths only. A pack is a self-contained data file. */
static int relative(const char *s)
{
    const char *p=s; size_t n;
    if(!*s || *s=='/' || *s=='\\' || strchr(s,':')) return 0;
    while(*p) {
        n=strcspn(p,"/\\");
        if(!n || (n==1 && *p=='.') || (n==2 && p[0]=='.' && p[1]=='.')) return 0;
        p+=n; if(*p) p++;
    }
    return 1;
}
CHARACTER_PACK *pack_read_campaign(const char *quest,char error[256])
{
    char line[1024],ref[260]="",path[520],*s,*end; FILE *fp; size_t base; long len;
    unsigned char *bytes; CHARACTER_PACK *p=NULL;
    error[0]=0; fp=fopen(quest,"rb");
    if(!fp) { strcpy(error,"Cannot read campaign entry."); return NULL; }
    while(fgets(line,sizeof(line),fp)) {
        s=line; while(*s==' ' || *s=='\t') s++;
        if(strncmp(s,";character-pack",15) || !isspace((unsigned char)s[15])) continue;
        s+=15; while(*s==' ' || *s=='\t') s++;
        end=s+strlen(s); while(end>s && isspace((unsigned char)end[-1])) *--end=0;
        if(*ref || strlen(s)>=sizeof(ref) || !relative(s)) { fclose(fp); strcpy(error,"Invalid or duplicate character-pack path."); return NULL; }
        strcpy(ref,s);
    }
    if(ferror(fp)) { fclose(fp); strcpy(error,"Cannot read campaign entry."); return NULL; } fclose(fp);
    if(!*ref) return pack_clone(pack_fantasy());
    base=strlen(quest); while(base && quest[base-1]!='/' && quest[base-1]!='\\') base--;
    if(base+strlen(ref)>=sizeof(path)) { strcpy(error,"Character-pack path is too long."); return NULL; }
    memcpy(path,quest,base); strcpy(path+base,ref); fp=fopen(path,"rb");
    if(!fp) { snprintf(error,256,"Character pack unavailable: %.200s",ref); return NULL; }
    if(fseek(fp,0,SEEK_END) || (len=ftell(fp))<24 || len>PACK_BYTES_LIMIT) { fclose(fp); strcpy(error,"Invalid character-pack size."); return NULL; }
    rewind(fp); bytes=(unsigned char*)malloc(len);
    if(bytes && fread(bytes,1,len,fp)==(size_t)len) p=pack_decode(bytes,len);
    free(bytes); fclose(fp); if(!p) strcpy(error,"Character pack is damaged or unsupported."); return p;
}
int pack_select_campaign(const char *quest,char error[256])
{
    CHARACTER_PACK *p=pack_read_campaign(quest,error),*active;
    if(!p) return 0;
    active=pack_clone(p);
    if(!active) { pack_free(p); strcpy(error,"Not enough memory for character profiles."); return 0; }
    pack_free(selected_pack); selected_pack=p;
    /* Selecting a campaign changes the registry immediately. This keeps room
       inspection and Heroes & Reserve aligned before map generation begins. */
    pack_adopt(active);
    return 1;
}
static int alias_equal(const char *a,size_t n,const char *b)
{
    size_t i; if(strlen(b)!=n) return 0;
    for(i=0;i<n;i++) if(tolower((unsigned char)(a[i]=='-'?' ':a[i]))!=tolower((unsigned char)(b[i]=='-'?' ':b[i]))) return 0;
    return 1;
}
const PACK_PROFILE *pack_find_monster(const CHARACTER_PACK *p,const char *name)
{
    const PACK_PROFILE *found=NULL; const char *a; int i,space=0; size_t n,used=0;
    char clean[1024]; const unsigned char *s=(const unsigned char*)name;
    if(!p || strlen(name)>=sizeof(clean)) return NULL;
    for(;*s;s++) {
        if(isspace(*s) || *s=='-') { space=used>0; continue; }
        if(space) clean[used++]=' '; clean[used++]=(char)tolower(*s); space=0;
    }
    clean[used]=0;
    for(i=0;i<p->monster_count;i++) {
        for(a=p->monsters[i].aliases;*a;a+=n+(* (a+n)?1:0)) {
            n=strcspn(a,"|");
            if(alias_equal(a,n,clean)) { if(found) return NULL; found=&p->monsters[i]; break; }
        }
    }
    return found;
}
const PACK_PROFILE *pack_monster(const char *name) { return pack_find_monster(pack_current(),name); }
int pack_nonmonster(const char *name)
{
    const CHARACTER_PACK *p=pack_current();
    const char *a; size_t n;
    if(!p) return 0;
    a=p->nonmonsters;
    while(*a) {
        n=strcspn(a,"|"); if(alias_equal(a,n,name)) return 1;
        a+=n; if(*a) a++;
    }
    return 0;
}
int pack_monster_stats(int index,int stats[HERO_STATS])
{
    const CHARACTER_PACK *p=pack_current(); const char *text;
    if(!p || index<0 || index>=p->monster_count) return 0;
    if(!p->legacy_references) {
        memcpy(stats,p->monsters[index].stats,sizeof(int)*HERO_STATS); return stats[7]>0;
    }
    /* Preserve the original fantasy transcription's strict conversion and
       ambiguous/special-profile behaviour, including fractional PV notation. */
    text=strstr(p->monsters[index].text,"   WS"); text=text?strchr(text,'\n'):NULL;
    return text && sscanf(text,"%d %d %d %d %d %d %d %d %d",&stats[0],&stats[1],&stats[2],&stats[3],&stats[4],&stats[5],&stats[6],&stats[7],&stats[8])==9 && stats[7]>0;
}
const PACK_PROFILE *pack_profile(const char *id)
{
    const CHARACTER_PACK *p=pack_current(); int i;
    if(!p) return NULL;
    for(i=0;i<p->hero_count;i++) if(!strcmp(p->heroes[i].id,id)) return &p->heroes[i];
    for(i=0;i<p->monster_count;i++) if(!strcmp(p->monsters[i].id,id)) return &p->monsters[i];
    return NULL;
}
void pack_hero(HERO *h,const PACK_PROFILE *p,int number)
{
    memset(h,0,sizeof(*h)); h->kind=p->kind;
    snprintf(h->name,sizeof(h->name),"%.30s %d",p->name,number);
    strcpy(h->profile_id,p->id); strcpy(h->class_name,p->name); strcpy(h->class_rules,p->text);
    memcpy(h->stats,p->stats,sizeof(h->stats)); h->melee=p->melee; h->ranged=p->ranged;
    h->wounds=h->stats[7]; h->fate=p->fate; h->x=h->y=-1;
    pack_caster_defaults(&h->caster,p,pack_current());
}
void pack_caster_defaults(CASTER_STATE *s,const PACK_PROFILE *q,const CHARACTER_PACK *p)
{
    int i,j; memset(s,0,sizeof(*s)); if(!p || !q) return;
    for(i=0;i<p->book_count;i++) if(q->books[i]) {
        s->books[i]=1; for(j=0;j<p->spell_count;j++) if(p->books[i].starting[j]) s->known[j]=1;
    }
    memcpy(s->components,q->components,sizeof(s->components));
    s->starting_allowance=q->starting_components; s->setup_pending=q->starting_components>0;
}
int pack_caster_valid(const CASTER_STATE *s,const CHARACTER_PACK *p)
{
    int i,j,member=0;
    if(!p || s->cast_used<0 || s->cast_used>1 || s->move_locked<0 || s->move_locked>1 ||
       s->setup_pending<0 || s->setup_pending>2 || s->starting_allowance<0 || s->starting_allowance>9999 ||
       (s->move_locked && !s->cast_used) || (s->setup_pending && !s->starting_allowance)) return 0;
    for(i=0;i<SPELL_BOOK_LIMIT;i++) { if(s->books[i]>1 || (i>=p->book_count && s->books[i])) return 0; member|=s->books[i]; }
    for(i=0;i<SPELL_LIMIT;i++) if(s->known[i]) {
        if(s->known[i]>1 || i>=p->spell_count) return 0;
        for(j=0;j<p->book_count;j++) if(s->books[j] && p->books[j].spells[i]) break;
        if(j==p->book_count) return 0;
    }
    for(i=0;i<SPELL_COMPONENT_LIMIT;i++) if(s->components[i]>9999 || (s->components[i] && (i>=p->component_count || !member))) return 0;
    return member || (!s->cast_used && !s->move_locked && !s->setup_pending && !s->starting_allowance);
}
void pack_shutdown(void)
{
    pack_free(active_pack); pack_free(selected_pack); pack_free(fantasy);
    active_pack=selected_pack=fantasy=NULL;
}
