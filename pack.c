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
    if(p) { free(p->heroes); free(p->monsters); free(p->bytes); free(p); }
}
CHARACTER_PACK *pack_decode(const unsigned char *bytes,size_t size)
{
    CHARACTER_PACK *p; PACK_READER r; int i,j,k,total; PACK_PROFILE *q,*prior;
    if(!bytes || size<24 || size>PACK_BYTES_LIMIT || memcmp(bytes,"HQPACK1\n",8)) return NULL;
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
        string(&r,q->ranged.weapon,sizeof(q->ranged.weapon)); q->ranged.kind=number(&r,4);
        q->ranged.range=number(&r,480); q->ranged.dice=number(&r,99);
        for(j=0;j<5;j++) q->ranged.hit[j]=number(&r,12);
        if(!*q->name || !*q->aliases || strncmp(q->id,p->id,strlen(p->id)) || q->id[strlen(p->id)]!=':') goto bad;
        if((i<p->hero_count || !p->legacy_references) && !q->stats[7]) goto bad;
        if(i<p->hero_count && strlen(q->text)>=sizeof(((HERO*)0)->class_rules)) goto bad;
        if(strcspn(q->aliases,"|")>63) goto bad;
        for(k=0;k<i;k++) {
            prior=k<p->hero_count?&p->heroes[k]:&p->monsters[k-p->hero_count];
            if(!strcmp(q->id,prior->id)) goto bad;
        }
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
    CHARACTER_PACK *p=pack_read_campaign(quest,error); if(!p) return 0;
    pack_free(selected_pack); selected_pack=p; return 1;
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
    const char *a=pack_current()->nonmonsters; size_t n;
    while(*a) {
        n=strcspn(a,"|"); if(alias_equal(a,n,name)) return 1;
        a+=n; if(*a) a++;
    }
    return 0;
}
int pack_monster_stats(int index,int stats[HERO_STATS])
{
    const CHARACTER_PACK *p=pack_current(); const char *text;
    if(index<0 || index>=p->monster_count) return 0;
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
}
void pack_shutdown(void)
{
    pack_free(active_pack); pack_free(selected_pack); pack_free(fantasy);
    active_pack=selected_pack=fantasy=NULL;
}
