/* Campaign discovery and copying are deliberately separate from the live map.
 * The application's real parser checks the staged result in a child process. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "campaign.h"
#include "pack.h"
#include "rsh/campaign.rh"

#define LIMIT 1024
#define TEXT_LIMIT (256 * 1024)
#define NAME_LIMIT 256
#define CAT_COUNT 6
typedef struct {
    char path[MAX_PATH], title[160];
    char *text, **defs, **refs, **includes;
    int ndefs, nrefs, nincludes, mask, origin;
} CBFile;
typedef struct {
    char root[MAX_PATH], error[2048], title[160], slug[48], description[16000];
    CBFile files[LIMIT];
    int count, base, selected[CAT_COUNT], furnish;
    int chosen[LIMIT], visiting[LIMIT], ordered[LIMIT], nordered;
    char directives[8192];
} Builder;
static const char *categories[] = {"Rooms/layout", "Monsters", "Hazards", "Furnishings", "Traps", "Treasure"};

static int fail(Builder *b, const char *message, const char *detail)
{
    _snprintf(b->error, sizeof(b->error), "%s%s%s", message, detail ? "\r\n" : "", detail ? detail : "");
    b->error[sizeof(b->error)-1] = 0;
    return 0;
}
static int join(char *out, const char *a, const char *c)
{
    if (strlen(a) + strlen(c) + 2 >= MAX_PATH) return 0;
    sprintf(out, "%s%s%s", a, *a && a[strlen(a)-1] != '\\' ? "\\" : "", c);
    return 1;
}
/* No absolute paths, traversal, device names, alternate streams or junctions. */
static int relative_path(const char *s)
{
    const char *p = s;
    if (!*p || *p == '\\' || *p == '/') return 0;
    while (*p) {
        char part[MAX_PATH]; int n = 0;
        while (*p && *p != '\\' && *p != '/') {
            if (!(isalnum((unsigned char)*p) || *p == '-' || *p == '_' || *p == '.')) return 0;
            if (n >= MAX_PATH-1) return 0;
            part[n++] = *p++;
        }
        part[n] = 0;
        if (!n || part[n-1] == '.' || !strcmp(part, ".") || !strcmp(part, "..")) return 0;
        { char *dot = strchr(part, '.'); if (dot) *dot = 0; }
        if (!_stricmp(part,"CON") || !_stricmp(part,"PRN") || !_stricmp(part,"AUX") || !_stricmp(part,"NUL") ||
            (strlen(part)==4 && (!_strnicmp(part,"COM",3) || !_strnicmp(part,"LPT",3)) && part[3]>='0' && part[3]<='9')) return 0;
        if (*p) { ++p; if (!*p) return 0; }
    }
    return 1;
}
static int safe_source(Builder *b, const char *rel, char *full)
{
    char path[MAX_PATH]; size_t i;
    if (!relative_path(rel) || !join(full,b->root,rel)) return fail(b,"Unsupported table path:",rel);
    strcpy(path,full);
    for (i=strlen(b->root)+1; ; ++i) {
        if (path[i]=='\\' || path[i]=='/' || !path[i]) {
            char c=path[i]; DWORD attrs;
            path[i]=0; attrs=GetFileAttributesA(path); path[i]=c;
            if (attrs==INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_REPARSE_POINT)) return fail(b,"Missing table or linked path:",rel);
            if (!c) break;
        }
    }
    return 1;
}
static char *read_text(const char *path)
{
    FILE *fp=fopen(path,"rb"); char *text; long n;
    if (!fp) return NULL;
    if (fseek(fp,0,SEEK_END) || (n=ftell(fp))<0 || n>TEXT_LIMIT) { fclose(fp); return NULL; }
    rewind(fp); text=(char*)malloc(n+1);
    if (!text) { fclose(fp); return NULL; }
    if (fread(text,1,n,fp)!=(size_t)n) { free(text); fclose(fp); return NULL; }
    text[n]=0; fclose(fp); return text;
}
static int add_name(char ***list,int *count,const char *name)
{
    char **next; int i;
    for(i=0;i<*count;i++) if (!strcmp((*list)[i],name)) return 1;
    next=(char**)realloc(*list,(*count+1)*sizeof(char*));
    if (!next) return 0;
    *list=next; next[*count]=_strdup(name);
    if (!next[*count]) return 0;
    ++*count; return 1;
}
static int has_name(char **list,int count,const char *name)
{
    int i; for(i=0;i<count;i++) if (!strcmp(list[i],name)) return 1; return 0;
}
/* Lexer for discovery only. Validation always uses table.c, including conditionals. */
static const char *token(const char *p,char *out)
{
    int n=0;
    for (;;) {
        while (*p && isspace((unsigned char)*p)) ++p;
        if (*p=='#' || *p==';' || *p=='*') { while(*p && *p!='\n') ++p; continue; }
        break;
    }
    if (*p=='"' || *p=='\'' || *p=='`') {
        char q=*p++; while(*p && *p!=q && *p!='\n') ++p; if (*p==q) ++p;
        strcpy(out,"<text>"); return p;
    }
    if (isalnum((unsigned char)*p) || *p=='_' || *p=='-') {
        while(isalnum((unsigned char)*p) || *p=='_' || *p=='-' || *p=='.') {
            if(n<NAME_LIMIT-1) out[n++]=*p; ++p;
        }
    } else if (*p) out[n++]=*p++;
    out[n]=0; return p;
}
static int include_line(const char *line,char *name)
{
    int n=0;
    while(*line==' ' || *line=='\t') ++line;
    if (_strnicmp(line,"include",7) || !isspace((unsigned char)line[7])) return 0;
    line+=7; while(*line==' ' || *line=='\t') ++line;
    while(*line && !isspace((unsigned char)*line)) { if(n<MAX_PATH-1) name[n++]=*line; ++line; }
    name[n]=0; return 1;
}
static int inspect_file(Builder *b,CBFile *f)
{
    char a[NAME_LIMIT],c[NAME_LIMIT],d[NAME_LIMIT],inc[MAX_PATH];
    const char *p=f->text,*q,*r;
    for(q=p;*q;) {
        if (include_line(q,inc) && !add_name(&f->includes,&f->nincludes,inc)) return fail(b,"Out of memory.",NULL);
        if (!*f->title) {
            r=q; while(*r==' ' || *r=='\t') ++r;
            if(*r=='#') {
                size_t n; ++r; while(*r==' ') ++r; n=strcspn(r,"\r\n");
                if(n && *r!='-') { if(n>=sizeof(f->title)) n=sizeof(f->title)-1; memcpy(f->title,r,n); f->title[n]=0; }
            }
        }
        q=strchr(q,'\n'); if(!q) break; ++q;
    }
    while(*p) {
        p=token(p,a); if(!*a) break;
        if(!isalpha((unsigned char)*a) && *a!='_') continue;
        q=token(p,c);
        if(strlen(c)==1 && strchr("([{",*c)) {
            token(q,d);
            if(strlen(d)==1 && strchr(")]}",*d)) {
                if(!add_name(&f->refs,&f->nrefs,a)) return fail(b,"Out of memory.",NULL);
            } else if(!add_name(&f->defs,&f->ndefs,a)) return fail(b,"Out of memory.",NULL);
        }
    }
    if(has_name(f->defs,f->ndefs,"Passage-Length") || has_name(f->defs,f->ndefs,"Room-Type")) f->mask|=1;
    if(has_name(f->defs,f->ndefs,"Wandering-Monsters") || has_name(f->defs,f->ndefs,"Lairs-Matrix")) f->mask|=2;
    if(has_name(f->defs,f->ndefs,"Hazards-Matrix")) f->mask|=4;
    if(has_name(f->defs,f->ndefs,"Room-Furnish")) f->mask|=8;
    if(has_name(f->defs,f->ndefs,"Room-Trap") || has_name(f->defs,f->ndefs,"Chest-Trap")) f->mask|=16;
    if(has_name(f->defs,f->ndefs,"Hidden-Treasure") || has_name(f->defs,f->ndefs,"Treasure-Chest")) f->mask|=32;
    return 1;
}
static void clear_catalog(Builder *b)
{
    int i,j; for(i=0;i<b->count;i++) {
        CBFile *f=&b->files[i]; free(f->text);
        for(j=0;j<f->ndefs;j++) free(f->defs[j]); free(f->defs);
        for(j=0;j<f->nrefs;j++) free(f->refs[j]); free(f->refs);
        for(j=0;j<f->nincludes;j++) free(f->includes[j]); free(f->includes);
    }
    memset(b->files,0,sizeof(b->files)); b->count=0;
}
static int scan_directory(Builder *b,const char *relative,int depth)
{
    char dir[MAX_PATH],pattern[MAX_PATH],rel[MAX_PATH],full[MAX_PATH];
    WIN32_FIND_DATAA fd; HANDLE h; int ok=1;
    if(depth>8) return fail(b,"Table folders are nested more than eight levels deep.",relative);
    if(!join(dir,b->root,relative) || !join(pattern,dir,"*")) return fail(b,"Table path is too long.",relative);
    h=FindFirstFileA(pattern,&fd); if(h==INVALID_HANDLE_VALUE) return fail(b,"Cannot read tables folder.",dir);
    do {
        if(fd.cFileName[0]=='.' || fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;
        if(!join(rel,relative,fd.cFileName)) { ok=fail(b,"Table path too long.",fd.cFileName); break; }
        if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { if(!(ok=scan_directory(b,rel,depth+1))) break; }
        else {
            const char *ext=strrchr(fd.cFileName,'.'); CBFile *f;
            if(!ext || _stricmp(ext,".tab")) continue;
            if(b->count==LIMIT) { ok=fail(b,"Too many table files (maximum 1024).",NULL); break; }
            if(!safe_source(b,rel,full)) { ok=0; break; }
            f=&b->files[b->count]; f->origin=b->count++; strcpy(f->path,rel); f->text=read_text(full);
            if(!f->text) { ok=fail(b,"Cannot read table (maximum 256 KB):",rel); break; }
            if(!(ok=inspect_file(b,f))) break;
        }
    } while(FindNextFileA(h,&fd));
    FindClose(h); return ok;
}
static int find_file(Builder *b,const char *name)
{
    int i; char normalized[MAX_PATH]; size_t j;
    if(strlen(name)>=MAX_PATH) return -1;
    strcpy(normalized,name); for(j=0;normalized[j];j++) if(normalized[j]=='/') normalized[j]='\\';
    for(i=0;i<b->count;i++) if(!_stricmp(b->files[i].path,normalized)) return i;
    return -1;
}
static int effective_mask(Builder *b,int index,int depth)
{
    int mask,i,j; if(index<0 || depth>8) return 0;
    mask=b->files[index].mask;
    for(i=0;i<b->files[index].nincludes;i++) {
        j=find_file(b,b->files[index].includes[i]); mask|=effective_mask(b,j,depth+1);
    }
    return mask;
}
static int choose_file(Builder *b,int index,int depth)
{
    int i,j; CBFile *f;
    if(index<0) return fail(b,"A required component file is missing.",NULL);
    if(depth>8 || b->visiting[index]) return fail(b,"Include cycle or too many nested includes:",b->files[index].path);
    if(b->chosen[index]) return 1;
    b->visiting[index]=1; f=&b->files[index];
    for(i=0;i<f->nincludes;i++) {
        j=find_file(b,f->includes[i]);
        if(j<0) return fail(b,"Cannot find included file:",f->includes[i]);
        if(!choose_file(b,j,depth+1)) return 0;
    }
    b->visiting[index]=0; b->chosen[index]=1; b->ordered[b->nordered++]=index; return 1;
}
static int same_folder(const char *a,const char *c)
{
    const char *p=strrchr(a,'\\'),*q=strrchr(c,'\\');
    return p && q && p-a==q-c && !_strnicmp(a,c,p-a);
}
/* Recover the original include context before falling back to filename proximity. */
static int contains_file(Builder *b,int entry,int target,int depth)
{
    int i;
    if(entry==target) return 1;
    if(entry<0 || depth>8) return 0;
    for(i=0;i<b->files[entry].nincludes;i++)
        if(contains_file(b,find_file(b,b->files[entry].includes[i]),target,depth+1)) return 1;
    return 0;
}
static int provider_rank(Builder *b,int referring,int provider)
{
    int i;
    referring=b->files[referring].origin;
    if(contains_file(b,b->base,referring,0) && contains_file(b,b->base,provider,0)) return 4;
    for(i=0;i<b->count;i++) if(!strchr(b->files[i].path,'\\') &&
        contains_file(b,i,referring,0) && contains_file(b,i,provider,0)) return 3;
    return same_folder(b->files[referring].path,b->files[provider].path) ? 2 : 1;
}
/* Copy only the missing named table, not the hazard/monster category surrounding
 * it. This keeps the user's dropdown choices intact and avoids duplicate tables.
 * Conditional files retain their complete original semantics via the old path. */
static int choose_support(Builder *b,int source,const char *name)
{
    CBFile *f=&b->files[source],*support; const char *p=f->text,*start,*q,*end;
    char a[NAME_LIMIT],c[NAME_LIMIT],d[NAME_LIMIT]; int depth,index;
    while(*p) {
        p=token(p,a);
        if(!_stricmp(a,"if") || !_stricmp(a,"define") || !_stricmp(a,"include")) return choose_file(b,source,0);
    }
    p=f->text;
    while(*p) {
        start=p; p=token(p,a);
        if(strcmp(a,name)) continue;
        q=token(p,c); token(q,d);
        if(strlen(c)!=1 || !strchr("([{",*c) || (strlen(d)==1 && strchr(")]}",*d))) continue;
        depth=1; end=q;
        while(*end && depth) {
            end=token(end,d);
            if(strlen(d)==1 && strchr("([{",*d)) ++depth;
            if(strlen(d)==1 && strchr(")]}",*d)) --depth;
        }
        if(depth) return fail(b,"Incomplete supporting table:",name);
        if(b->count==LIMIT) return fail(b,"Too many supporting tables.",NULL);
        index=b->count++;
        support=&b->files[index]; support->origin=source;
        {
            int suffix=index; char path[MAX_PATH];
            do { sprintf(path,"_support\\dependency-%03d.tab",suffix++); } while(find_file(b,path)>=0);
            strcpy(support->path,path);
        }
        support->text=(char*)malloc((end-start)+MAX_PATH+80);
        if(!support->text) return fail(b,"Out of memory.",NULL);
        sprintf(support->text,"; Supporting table copied from %s\r\n",f->path);
        strncat(support->text,start,end-start); strcat(support->text,"\r\n");
        if(!inspect_file(b,support)) return 0;
        return choose_file(b,index,0);
    }
    return fail(b,"Cannot extract supporting table:",name);
}
static int resolve_dependencies(Builder *b)
{
    int k,r,i,j,found,candidate,n,same;
    for(k=0;k<b->nordered;k++) {
        CBFile *f=&b->files[b->ordered[k]];
        for(r=0;r<f->nrefs;r++) {
            found=0;
            for(i=0;i<b->count;i++) if(b->chosen[i] && has_name(b->files[i].defs,b->files[i].ndefs,f->refs[r])) { found=1; break; }
            if(found) continue;
            candidate=-1; n=0; same=0;
            for(j=0;j<b->count;j++) {
                if(!strchr(b->files[j].path,'\\') || !has_name(b->files[j].defs,b->files[j].ndefs,f->refs[r])) continue;
                i=provider_rank(b,b->ordered[k],j);
                if(i>same) { same=i; n=0; }
                if(i==same) { candidate=j; ++n; }
            }
            if(n!=1) {
                char detail[640]; sprintf(detail,"%s references %s(). %s",f->path,f->refs[r],n ? "Several files define it; choose compatible components." : "No available file defines it.");
                return fail(b,"Unresolved table dependency:",detail);
            }
            if(!choose_support(b,candidate,f->refs[r])) return 0;
        }
    }
    return 1;
}
static int prepare(Builder *b)
{
    int i,j; const char *p; char line[2048],inc[MAX_PATH];
    /* Re-checking must not use support snippets from the previous selection. */
    while(b->count && b->files[b->count-1].origin!=b->count-1) {
        CBFile *f=&b->files[--b->count];
        free(f->text);
        for(i=0;i<f->ndefs;i++) free(f->defs[i]); free(f->defs);
        for(i=0;i<f->nrefs;i++) free(f->refs[i]); free(f->refs);
        for(i=0;i<f->nincludes;i++) free(f->includes[i]); free(f->includes);
        memset(f,0,sizeof(*f));
    }
    memset(b->chosen,0,sizeof(b->chosen)); memset(b->visiting,0,sizeof(b->visiting)); b->nordered=0; b->directives[0]=0;
    if(b->base<0) return fail(b,"Choose a starting campaign.",NULL);
    for(i=0;i<CAT_COUNT;i++) if(!choose_file(b,b->selected[i],0)) return 0;
    /* Keep unclassified support files and defines from the template. Never
       silently discard inline tables or conditional campaign wiring. */
    p=b->files[b->base].text;
    while(*p) {
        size_t n=strcspn(p,"\r\n"); char *s=line;
        if(n>=sizeof(line)) return fail(b,"Campaign entry line is too long.",NULL);
        memcpy(line,p,n); line[n]=0; p+=n; while(*p=='\r' || *p=='\n') ++p;
        while(*s==' ' || *s=='\t') ++s;
        if(!*s || strchr("#;*",*s)) continue;
        if(include_line(s,inc)) {
            j=find_file(b,inc);
            if(j<0) return fail(b,"Missing campaign include:",inc);
            if(!effective_mask(b,j,0) && !choose_file(b,j,0)) return 0;
        } else if(!_strnicmp(s,"define",6) && isspace((unsigned char)s[6])) {
            char symbol[NAME_LIMIT]; token(s+6,symbol);
            if(!strcmp(symbol,"ROOM-FURNISH")) continue;
            if(strlen(b->directives)+strlen(s)+3>=sizeof(b->directives)) return fail(b,"Too many campaign directives.",NULL);
            strcat(b->directives,s); strcat(b->directives,"\r\n");
        } else return fail(b,"This template has inline tables or conditional entry-file wiring. Use an include-based campaign as the starting point.",b->files[b->base].path);
    }
    return resolve_dependencies(b);
}
static int valid_name(Builder *b)
{
    size_t i,n=strlen(b->slug); char dest[MAX_PATH],entry[64];
    if(!*b->title || strpbrk(b->title,"\r\n")) return fail(b,"Enter a campaign title.",NULL);
    if(!n || n>40 || !isalpha((unsigned char)b->slug[0]) || !isalpha((unsigned char)b->slug[n-1])) return fail(b,"Folder name must start and end with a letter (maximum 40 characters).",NULL);
    for(i=0;i<n;i++) if(!((b->slug[i]>='a' && b->slug[i]<='z') || (b->slug[i]>='A' && b->slug[i]<='Z') || b->slug[i]=='-')) return fail(b,"Use only English letters and hyphens in the folder name.",NULL);
    if(!relative_path(b->slug)) return fail(b,"That folder name is reserved by Windows.",NULL);
    sprintf(entry,"%s.tab",b->slug);
    if(!join(dest,b->root,b->slug) || GetFileAttributesA(dest)!=INVALID_FILE_ATTRIBUTES || !join(dest,b->root,entry) || GetFileAttributesA(dest)!=INVALID_FILE_ATTRIBUTES) return fail(b,"That campaign or folder already exists. Choose a new name.",NULL);
    return 1;
}
static int make_parents(char *path)
{
    char *p; for(p=path+3;*p;p++) if(*p=='\\') {
        DWORD attr; *p=0; attr=GetFileAttributesA(path);
        if(attr==INVALID_FILE_ATTRIBUTES) { if(!CreateDirectoryA(path,NULL)) { *p='\\'; return 0; } }
        else if(!(attr & FILE_ATTRIBUTE_DIRECTORY) || (attr & FILE_ATTRIBUTE_REPARSE_POINT)) { *p='\\'; return 0; }
        *p='\\';
    }
    return 1;
}
static int write_text(const char *path,const char *text)
{
    HANDLE h=CreateFileA(path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL); DWORD n=0; int ok;
    if(h==INVALID_HANDLE_VALUE) return 0;
    ok=WriteFile(h,text,(DWORD)strlen(text),&n,NULL) && n==strlen(text);
    if(!CloseHandle(h)) ok=0; return ok;
}
/* Only called for a uniquely created staging directory owned by this operation. */
static void remove_stage(const char *dir)
{
    char pattern[MAX_PATH],path[MAX_PATH]; WIN32_FIND_DATAA fd; HANDLE h;
    if(!join(pattern,dir,"*")) return;
    h=FindFirstFileA(pattern,&fd);
    if(h!=INVALID_HANDLE_VALUE) {
        do {
            if(!strcmp(fd.cFileName,".") || !strcmp(fd.cFileName,"..") || !join(path,dir,fd.cFileName)) continue;
            if(fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;
            if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) remove_stage(path); else DeleteFileA(path);
        } while(FindNextFileA(h,&fd)); FindClose(h);
    }
    RemoveDirectoryA(dir);
}
static int stage_campaign(Builder *b,const char *stage)
{
    int i; char path[MAX_PATH],rel[MAX_PATH],inc[MAX_PATH]; FILE *fp;
    char *entry=(char*)calloc(1,TEXT_LIMIT); int ok=0;
    CHARACTER_PACK *pack=NULL; char pack_error[256];
    if(!entry) return fail(b,"Out of memory.",NULL);
    sprintf(entry,"#%s\r\n# Created with HQ-Map Campaign Builder\r\n\r\n%s%s",b->title,b->furnish ? "define ROOM-FURNISH\r\n" : "",b->directives);
    if(!join(path,b->root,b->files[b->base].path)) { fail(b,"Campaign path too long.",NULL); goto done; }
    pack=pack_read_campaign(path,pack_error);
    if(!pack) { fail(b,"Cannot copy character pack.",pack_error); goto done; }
    /* The base campaign defines the roster. Reject incompatible monster
       sources instead of creating encounters that cannot resolve. */
    for(i=0;i<b->count;i++) if(!strchr(b->files[i].path,'\\') && i!=b->base &&
        contains_file(b,i,b->selected[1],0) && !contains_file(b,b->base,b->selected[1],0)) {
        CHARACTER_PACK *other;
        if(!join(path,b->root,b->files[i].path)) continue;
        other=pack_read_campaign(path,pack_error);
        if(!other) { fail(b,"Cannot read monster source pack.",pack_error); goto done; }
        if(strcmp(other->id,pack->id)) {
            pack_free(other); fail(b,"Monster tables use a different character pack. Choose a starting campaign with that pack.",b->files[i].path); goto done;
        }
        pack_free(other);
    }
    sprintf(rel,"%s\\characters.hqp",b->slug);
    if(!join(path,stage,rel) || !make_parents(path)) { fail(b,"Cannot create character-pack folder.",NULL); goto done; }
    fp=fopen(path,"wb"); if(!fp) { fail(b,"Cannot copy character pack.",NULL); goto done; }
    { int bad=fwrite(pack->bytes,1,pack->size,fp)!=pack->size; if(fclose(fp)) bad=1;
      if(bad) { fail(b,"Cannot finish character pack.",NULL); goto done; } }
    strcat(entry,";character-pack "); strcat(entry,rel); strcat(entry,"\r\n");
    /* Copy each dependency once. Explicit nested includes retain their location
       and conditions; only roots are also listed in the campaign entry. */
    for(i=0;i<b->nordered;i++) {
        int j,nested=0; CBFile *f=&b->files[b->ordered[i]]; const char *p=f->text;
        for(j=0;j<b->nordered;j++) if(has_name(b->files[b->ordered[j]].includes,b->files[b->ordered[j]].nincludes,f->path)) nested=1;
        if(strlen(b->slug)+strlen(f->path)+2>=MAX_PATH) { fail(b,"Copied path is too long.",f->path); goto done; }
        sprintf(rel,"%s\\%s",b->slug,f->path);
        if(!join(path,stage,rel) || !make_parents(path)) { fail(b,"Cannot create component folder.",rel); goto done; }
        fp=fopen(path,"wb"); if(!fp) { fail(b,"Cannot write component.",rel); goto done; }
        while(*p) {
            size_t n=strcspn(p,"\r\n");
            if(include_line(p,inc)) fprintf(fp,"include %s\\%s\r\n",b->slug,inc);
            else { fwrite(p,1,n,fp); fputs("\r\n",fp); }
            p+=n; while(*p=='\r' || *p=='\n') ++p;
        }
        { int bad=ferror(fp); if(fclose(fp)) bad=1; if(bad) { fail(b,"Cannot finish writing component.",rel); goto done; } }
        if(!nested) {
            if(strlen(entry)+strlen(rel)+12>=TEXT_LIMIT) { fail(b,"Campaign entry too large.",NULL); goto done; }
            strcat(entry,"include "); strcat(entry,rel); strcat(entry,"\r\n");
        }
    }
    sprintf(rel,"%s.tab",b->slug);
    if(!join(path,stage,rel) || !write_text(path,entry)) { fail(b,"Cannot write campaign entry.",NULL); goto done; }
    sprintf(rel,"%s\\description.txt",b->slug);
    if(!join(path,stage,rel) || !write_text(path,b->description)) { fail(b,"Cannot write campaign description.",NULL); goto done; }
    ok=1;
done: pack_free(pack); free(entry); return ok;
}
static int validate_stage(Builder *b,const char *stage)
{
    char exe[MAX_PATH],command[2*MAX_PATH+128],log[MAX_PATH];
    STARTUPINFOA si; PROCESS_INFORMATION pi; DWORD code,wait; char *errors;
    if(!GetModuleFileNameA(NULL,exe,sizeof(exe))) return fail(b,"Cannot locate HQ-Map for validation.",NULL);
    sprintf(command,"\"%s\" --check-campaign %s.tab validation.txt",exe,b->slug);
    memset(&si,0,sizeof(si)); si.cb=sizeof(si); si.dwFlags=STARTF_USESHOWWINDOW; si.wShowWindow=SW_HIDE;
    memset(&pi,0,sizeof(pi));
    if(!CreateProcessA(exe,command,NULL,NULL,FALSE,CREATE_NO_WINDOW,NULL,stage,&si,&pi)) return fail(b,"Cannot start campaign validation.",NULL);
    wait=WaitForSingleObject(pi.hProcess,10000);
    if(wait!=WAIT_OBJECT_0) { TerminateProcess(pi.hProcess,2); WaitForSingleObject(pi.hProcess,2000); code=2; }
    else if(!GetExitCodeProcess(pi.hProcess,&code)) code=2;
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
    if(code==0) return 1;
    join(log,stage,"validation.txt"); errors=read_text(log);
    fail(b,code==2 ? "Validation could not complete." : "These tables do not work together:",errors ? errors : "The parser could not load this campaign."); free(errors); return 0;
}
static int run_campaign(Builder *b,int create)
{
    char stage[MAX_PATH],name[64],from[MAX_PATH],to[MAX_PATH],folder[MAX_PATH]; int i,ok=0;
    if(!valid_name(b) || !prepare(b)) return 0;
    for(i=0;i<100;i++) {
        sprintf(name,".hq-builder-%lu-%lu-%d",GetCurrentProcessId(),GetTickCount(),i);
        if(!join(stage,b->root,name)) return fail(b,"Tables folder path is too long.",NULL);
        if(CreateDirectoryA(stage,NULL)) break;
        if(GetLastError()!=ERROR_ALREADY_EXISTS) return fail(b,"Cannot create files in the tables folder.",b->root);
    }
    if(i==100) return fail(b,"Cannot create a staging folder.",NULL);
    if(!stage_campaign(b,stage) || !validate_stage(b,stage)) goto done;
    if(create) {
        join(from,stage,b->slug); join(folder,b->root,b->slug);
        if(!MoveFileA(from,folder)) { fail(b,"Cannot save campaign folder; nothing was overwritten.",folder); goto done; }
        sprintf(name,"%s.tab",b->slug); join(from,stage,name); join(to,b->root,name);
        if(!MoveFileA(from,to)) {
            /* Roll back the owned directory, never a pre-existing campaign. */
            join(from,stage,b->slug);
            if(!MoveFileA(folder,from)) fail(b,"Could not publish campaign entry. Copied files remain here:",folder);
            else fail(b,"Cannot save campaign entry; nothing was overwritten.",to);
            goto done;
        }
    }
    ok=1;
done: remove_stage(stage); return ok;
}

static INT_PTR CALLBACK preview_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
    if(msg==WM_INITDIALOG) {
        const char *text=(const char*)lp; char *display; size_t i,n=0;
        display=(char*)malloc(strlen(text)*2+1);
        if(display) { for(i=0;text[i];i++) { if(text[i]=='\n' && (!i || text[i-1]!='\r')) display[n++]='\r'; display[n++]=text[i]; } display[n]=0; }
        SendDlgItemMessage(hwnd,CBPREVIEWTEXT,EM_LIMITTEXT,0,0);
        SendDlgItemMessage(hwnd,CBPREVIEWTEXT,WM_SETFONT,(WPARAM)GetStockObject(ANSI_FIXED_FONT),FALSE);
        SetDlgItemTextA(hwnd,CBPREVIEWTEXT,display ? display : text); free(display); return TRUE;
    }
    if(msg==WM_CLOSE || (msg==WM_COMMAND && (LOWORD(wp)==IDOK || LOWORD(wp)==IDCANCEL))) { EndDialog(hwnd,0); return TRUE; }
    return FALSE;
}
static void preview(HWND hwnd,const char *text)
{
    DialogBoxParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FCBPREVIEW),hwnd,preview_proc,(LPARAM)text);
}
static int combo_value(HWND hwnd,int id)
{
    LRESULT n=SendDlgItemMessage(hwnd,id,CB_GETCURSEL,0,0);
    return n==CB_ERR ? -1 : (int)SendDlgItemMessage(hwnd,id,CB_GETITEMDATA,n,0);
}
static void combo_select(HWND hwnd,int id,int value)
{
    int i,n=(int)SendDlgItemMessage(hwnd,id,CB_GETCOUNT,0,0);
    SendDlgItemMessage(hwnd,id,CB_SETCURSEL,(WPARAM)-1,0);
    for(i=0;i<n;i++) if(SendDlgItemMessage(hwnd,id,CB_GETITEMDATA,i,0)==value) { SendDlgItemMessage(hwnd,id,CB_SETCURSEL,i,0); break; }
}
static void base_changed(HWND hwnd,Builder *b)
{
    int i,j,k,index; CBFile *f; char path[MAX_PATH],folder[MAX_PATH],relative[MAX_PATH],title[160],*desc,*dot;
    b->base=combo_value(hwnd,CBBASE); if(b->base<0) return;
    f=&b->files[b->base];
    for(i=0;i<CAT_COUNT;i++) b->selected[i]=-1;
    for(j=0;j<f->nincludes;j++) {
        index=find_file(b,f->includes[j]); k=effective_mask(b,index,0);
        for(i=0;i<CAT_COUNT;i++) if(k & (1<<i)) b->selected[i]=index;
    }
    for(i=0;i<CAT_COUNT;i++) combo_select(hwnd,CBCATEGORY+i,b->selected[i]);
    _snprintf(title,sizeof(title),"Copy of %.145s",*f->title ? f->title : f->path); title[159]=0;
    SetDlgItemTextA(hwnd,CBTITLE,title);
    CheckDlgButton(hwnd,CBFURNISH,strstr(f->text,"define ROOM-FURNISH") ? BST_CHECKED : BST_UNCHECKED);
    strcpy(folder,f->path); dot=strrchr(folder,'.'); if(dot) *dot=0;
    i=(int)strlen(folder); while(i && (isdigit((unsigned char)folder[i-1]) || folder[i-1]=='_')) folder[--i]=0;
    desc=NULL;
    if(join(relative,folder,"description.txt") && join(path,b->root,relative)) desc=read_text(path);
    if(desc) {
        char *display=(char*)malloc(strlen(desc)*2+1); size_t n=0,j;
        if(display) {
            for(j=0;desc[j];j++) { if(desc[j]=='\n' && (!j || desc[j-1]!='\r')) display[n++]='\r'; display[n++]=desc[j]; }
            display[n]=0; SetDlgItemTextA(hwnd,CBDESC,display); free(display);
        } else SetDlgItemTextA(hwnd,CBDESC,desc);
    } else SetDlgItemTextA(hwnd,CBDESC,"");
    free(desc);
    SetDlgItemTextA(hwnd,CBSTATUS,"Choose components, a new folder name, and edit the description.");
}
static int populate(HWND hwnd,Builder *b,const char *current)
{
    int i,c,n,mask,initial=-1; char label[480];
    clear_catalog(b); b->error[0]=0; b->base=-1;
    SendDlgItemMessage(hwnd,CBBASE,CB_RESETCONTENT,0,0);
    for(c=0;c<CAT_COUNT;c++) SendDlgItemMessage(hwnd,CBCATEGORY+c,CB_RESETCONTENT,0,0);
    if(!scan_directory(b,"",0)) { SetDlgItemTextA(hwnd,CBSTATUS,b->error); return 0; }
    for(i=0;i<b->count;i++) {
        CBFile *f=&b->files[i];
        if(!strchr(f->path,'\\')) {
            sprintf(label,"%s  [%s]",*f->title ? f->title : "Campaign",f->path);
            n=(int)SendDlgItemMessageA(hwnd,CBBASE,CB_ADDSTRING,0,(LPARAM)label);
            SendDlgItemMessage(hwnd,CBBASE,CB_SETITEMDATA,n,i);
            if(initial<0 || (current && !_stricmp(current,f->path))) initial=i;
        } else {
            const char *friendly=*f->title ? f->title : NULL;
            int owner;
            if(!friendly) for(owner=0;owner<b->count;owner++) {
                CBFile *entry=&b->files[owner];
                if(!strchr(entry->path,'\\') && *entry->title && has_name(entry->includes,entry->nincludes,f->path)) {
                    friendly=entry->title; break;
                }
            }
            if(!_strnicmp(f->path,"standard\\",9)) friendly="Standard";
            mask=effective_mask(b,i,0);
            for(c=0;c<CAT_COUNT;c++) if(mask & (1<<c)) {
                sprintf(label,"%s  [%s]%s",friendly ? friendly : categories[c],f->path,(mask & (mask-1)) ? " (shared)" : "");
                n=(int)SendDlgItemMessageA(hwnd,CBCATEGORY+c,CB_ADDSTRING,0,(LPARAM)label);
                SendDlgItemMessage(hwnd,CBCATEGORY+c,CB_SETITEMDATA,n,i);
            }
        }
    }
    combo_select(hwnd,CBBASE,initial); base_changed(hwnd,b);
    if(initial<0) SetDlgItemTextA(hwnd,CBSTATUS,"No campaign entry files were found in the tables folder.");
    return 1;
}
static void read_form(HWND hwnd,Builder *b)
{
    int i; GetDlgItemTextA(hwnd,CBTITLE,b->title,sizeof(b->title)); GetDlgItemTextA(hwnd,CBSLUG,b->slug,sizeof(b->slug));
    GetDlgItemTextA(hwnd,CBDESC,b->description,sizeof(b->description)); b->furnish=IsDlgButtonChecked(hwnd,CBFURNISH)==BST_CHECKED;
    for(i=0;i<CAT_COUNT;i++) b->selected[i]=combo_value(hwnd,CBCATEGORY+i);
}
static void review_campaign(HWND hwnd,Builder *b)
{
    char *report=(char*)calloc(1,TEXT_LIMIT); int i;
    if(!report) return;
    strcpy(report,"VALIDATION PASSED\r\n\r\nThe selected tables work together. Creating the campaign will copy these files\r\ninto its own folder, including supporting tables:\r\n\r\n");
    for(i=0;i<b->nordered;i++) {
        const char *path=b->files[b->ordered[i]].path;
        if(strlen(report)+strlen(path)+1024<TEXT_LIMIT) { strcat(report,"  "); strcat(report,path); strcat(report,"\r\n"); }
    }
    strcat(report,"\r\nINHERITED ROOM TEXT TO REVIEW\r\n\r\nChanging monsters does not change quest objectives or named rewards.\r\nThese room texts will be copied as written. You can edit the new room tables\r\nafter creation; the originals will be unchanged.\r\n\r\n");
    for(i=0;i<b->nordered;i++) {
        CBFile *f=&b->files[b->ordered[i]]; const char *p=f->text; int any=0;
        if(!(f->mask & 1)) continue;
        while(*p) {
            char word[NAME_LIMIT];
            while(*p && isspace((unsigned char)*p)) ++p;
            if(*p=='#' || *p==';' || *p=='*') { while(*p && *p!='\n') ++p; continue; }
            if(*p=='"' || *p=='\'' || *p=='`') {
                char quote=*p++; const char *start=p; size_t n;
                while(*p && *p!=quote && *p!='\n') ++p;
                n=(size_t)(p-start);
                if(strlen(report)+n+strlen(f->path)+12<TEXT_LIMIT) {
                    if(!any) { strcat(report,f->path); strcat(report,"\r\n"); any=1; }
                    strcat(report,"  "); strncat(report,start,n); strcat(report,"\r\n");
                }
                if(*p==quote) ++p;
            } else p=token(p,word);
        }
        if(any && strlen(report)+3<TEXT_LIMIT) strcat(report,"\r\n");
    }
    preview(hwnd,report); free(report);
}
static INT_PTR CALLBACK builder_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
    Builder *b=(Builder*)GetWindowLongPtr(hwnd,DWLP_USER); int id=LOWORD(wp),i;
    if(msg==WM_INITDIALOG) {
        b=(Builder*)lp; SetWindowLongPtr(hwnd,DWLP_USER,(LONG_PTR)b);
        SendDlgItemMessage(hwnd,CBTITLE,EM_LIMITTEXT,sizeof(b->title)-1,0);
        SendDlgItemMessage(hwnd,CBSLUG,EM_LIMITTEXT,40,0);
        SendDlgItemMessage(hwnd,CBDESC,EM_LIMITTEXT,sizeof(b->description)-1,0);
        for(i=0;i<CAT_COUNT;i++) SendDlgItemMessage(hwnd,CBCATEGORY+i,CB_SETDROPPEDWIDTH,650,0);
        populate(hwnd,b,b->title); return TRUE;
    }
    if(!b) return FALSE;
    if(msg==WM_CLOSE || (msg==WM_COMMAND && id==IDCANCEL)) { EndDialog(hwnd,0); return TRUE; }
    if(msg!=WM_COMMAND) return FALSE;
    if(id==CBBASE && HIWORD(wp)==CBN_SELCHANGE) { base_changed(hwnd,b); return TRUE; }
    if(id==CBREFRESH) {
        char current[MAX_PATH],title[160]; char selected[CAT_COUNT][MAX_PATH];
        char *desc=(char*)malloc(sizeof(b->description)); UINT furnished=IsDlgButtonChecked(hwnd,CBFURNISH);
        if(!desc) return TRUE;
        current[0]=0; if(b->base>=0 && b->base<b->count) strcpy(current,b->files[b->base].path);
        GetDlgItemTextA(hwnd,CBTITLE,title,sizeof(title)); GetDlgItemTextA(hwnd,CBDESC,desc,sizeof(b->description));
        for(i=0;i<CAT_COUNT;i++) { int n=combo_value(hwnd,CBCATEGORY+i); selected[i][0]=0; if(n>=0) strcpy(selected[i],b->files[n].path); }
        if(populate(hwnd,b,current)) for(i=0;i<CAT_COUNT;i++) combo_select(hwnd,CBCATEGORY+i,find_file(b,selected[i]));
        SetDlgItemTextA(hwnd,CBTITLE,title); SetDlgItemTextA(hwnd,CBDESC,desc);
        CheckDlgButton(hwnd,CBFURNISH,furnished); free(desc); return TRUE;
    }
    if(id>=CBVIEW && id<CBVIEW+CAT_COUNT) {
        i=combo_value(hwnd,CBCATEGORY+id-CBVIEW); if(i>=0) preview(hwnd,b->files[i].text); return TRUE;
    }
    if(id>=CBCATEGORY && id<CBCATEGORY+CAT_COUNT && HIWORD(wp)==CBN_SELCHANGE) {
        int n=combo_value(hwnd,id),mask=effective_mask(b,n,0);
        for(i=0;i<CAT_COUNT;i++) if(mask & (1<<i)) combo_select(hwnd,CBCATEGORY+i,n);
        SetDlgItemTextA(hwnd,CBSTATUS,"Selection changed. Check Campaign to validate the combination."); return TRUE;
    }
    if(id==CBCHECK || id==CBCREATE) {
        int ok; read_form(hwnd,b); SetCursor(LoadCursor(NULL,IDC_WAIT));
        ok=run_campaign(b,id==CBCREATE); SetCursor(LoadCursor(NULL,IDC_ARROW));
        if(!ok) { preview(hwnd,b->error); SetDlgItemTextA(hwnd,CBSTATUS,"Check failed. Review the reported issue and adjust the campaign."); }
        else if(id==CBCREATE) {
            MessageBoxA(hwnd,"Campaign created. Choose it under Options > Load Tables when you are ready to generate a map.","Campaign Builder",MB_OK|MB_ICONINFORMATION); EndDialog(hwnd,1);
        } else {
            SetDlgItemTextA(hwnd,CBSTATUS,"Tables passed validation. Review inherited quest text before creating.");
            review_campaign(hwnd,b);
        }
        return TRUE;
    }
    return FALSE;
}
void campaign_builder(const char *tables_directory,const char *current_campaign)
{
    Builder *b=(Builder*)calloc(1,sizeof(Builder)); DWORD n;
    if(!b) return;
    n=GetFullPathNameA(tables_directory,sizeof(b->root),b->root,NULL);
    if(!n || n>=sizeof(b->root)) { free(b); return; }
    n=(DWORD)strlen(b->root); while(n>3 && (b->root[n-1]=='\\' || b->root[n-1]=='/')) b->root[--n]=0;
    b->base=-1; strncpy(b->title,current_campaign,sizeof(b->title)-1);
    DialogBoxParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FCB),GetActiveWindow(),builder_proc,(LPARAM)b);
    clear_catalog(b); free(b);
}
