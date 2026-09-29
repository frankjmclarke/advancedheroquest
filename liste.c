#include <liste.h>
#include <pice.h>
#include <defs.h>
#include <boxf.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "pack.h"
#define AHQ_REFERENCE_COUNT (pack_current()?pack_current()->monster_count:0)
#define ahq_references (pack_current()->monsters)

/******************************************************************************/
/*** ---------------------------------------------------------------------- ***/
/******************************************************************************/

GLOBAL size_t draw_monster_liste(_UBYTE **mem)
{
	_WORD i;
	size_t size = 0;
	_UBYTE **ptr;
	_UBYTE **first = mem;
	PICE *pice;
	FEATURE *titel;
	
	for (titel = Titel; titel != NULL; titel = titel->next)
	{
		if (mem != NULL)
		{
			*mem = game_gold_text(NULL,titel->ptr.text?titel->ptr.text:"");
			if (*mem == NULL) goto failed;
			mem++;
		}
		size++;
	}
	
	pice = Pice;
	for (i = 0; i < MAX_PICE; i++)
	{
		if ((ptr = pice->text) != NULL)
		{
			switch (pice->type)
			{
			case SMALL_ROOM:
			case NORMAL_ROOM:
			case HAZARD:
			case LARGE_ROOM:
			case LAIR:
			case QUEST:
			case BIG:
			case MERSCHA:
			case PASSAGE:
			case DEAD_END:
			case LEFT_TURN:
			case RIGHT_TURN:
			case T_JUNCTION:
			case CORNER:
			case STAIRS_OUT:
			case STAIRS_DOWN:
				do
				{	
					if (mem != NULL)
					{
						*mem = game_gold_text(pice,*ptr?*ptr:"");
						if (*mem == NULL) goto failed;
						mem++;
					}
					size++;
				} while (*ptr++ != NULL);
				break;
			case DOOR:
			case SECRET:
			case TEST:
			case INVALID:
			case EMPTY:
				break;
			default:
				abbruch("Invalid type\n(draw_monster_liste)");
				return 0;
			}
		}
		pice++;
	}
	return size;
failed:
	while (mem > first) free(*--mem);
	return 0;
}

/* Room reference expansion. Exact aliases are matched within encounter clauses,
 * never as substrings of longer names (Orc Champion must not become Orc).
 * Keep this independent of the UI so the same text works in either map view. */
typedef struct {
    char *text;
    size_t used, capacity;
    int failed;
} ROOM_BUFFER;

static void room_append_n(ROOM_BUFFER *buf, const char *text, size_t count)
{
    size_t required, capacity;
    char *next;
    if (buf->failed) return;
    if (count > (size_t)-1 - buf->used - 1) { buf->failed = 1; return; }
    required = buf->used + count + 1;
    if (required > buf->capacity) {
        capacity = buf->capacity ? buf->capacity : 1024;
        while (capacity < required) {
            if (capacity > (size_t)-1 / 2) { capacity = required; break; }
            capacity *= 2;
        }
        next = realloc(buf->text, capacity);
        if (next == NULL) { buf->failed = 1; return; }
        buf->text = next;
        buf->capacity = capacity;
    }
    memcpy(buf->text + buf->used, text, count);
    buf->used += count;
    buf->text[buf->used] = '\0';
}

static void room_append(ROOM_BUFFER *buf, const char *text)
{
    room_append_n(buf, text, strlen(text));
}

/* Normalize case, whitespace and hyphens; preserve all original encounter text. */
static char *room_name(const char *start, const char *end)
{
    char *name, *out;
    int space = 0;
    name = malloc((size_t)(end - start) + 1);
    if (name == NULL) return NULL;
    out = name;
    while (start < end) {
        unsigned char c = (unsigned char)*start++;
        if (isspace(c) || c == '-') { space = out != name; continue; }
        if (space) *out++ = ' ';
        *out++ = (char)tolower(c);
        space = 0;
    }
    *out = '\0';
    return name;
}

static int room_alias(const char *name, const char *aliases)
{
    const char *end;
    const char *n, *a;
    /* Generated aliases are lower case with normalized single spaces. */
    while (*aliases) {
        end = strchr(aliases, '|');
        if (end == NULL) end = aliases + strlen(aliases);
        n = name;
        a = aliases;
        while (a < end && *n && *n == (*a == '-' ? ' ' : *a)) { n++; a++; }
        if (a == end && *n == '\0') return 1;
        aliases = *end ? end + 1 : end;
    }
    return 0;
}

static int room_nonmonster(const char *name)
{
    static const char *items[] = {
        "gold crown", "gold crowns", "gold", "points", "point",
        "dungeon counter", "dungeon counters", "fate point", "fate points",
        "treasure", "hidden treasure", "potion", "potions", "attack", "attacks"
    };
    size_t i, len;
    if(pack_nonmonster(name)) return 1;
    for (i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
        len = strlen(items[i]);
        if (strncmp(name, items[i], len) == 0 && (!name[len] || name[len] == ' ')) return 1;
    }
    return 0;
}

static int room_contains(const char *start, const char *end, const char *needle)
{
    size_t len = strlen(needle);
    for (; (size_t)(end - start) >= len; start++)
        if (strncmp(start, needle, len) == 0) return 1;
    return 0;
}

static int room_join(const char *text, const char *word)
{
    while (*word) {
        if (tolower((unsigned char)*text) != *word) return 0;
        text++; word++;
    }
    return 1;
}

static void room_clause(ROOM_BUFFER *buf, const char *start, const char *end,
                        unsigned char *seen)
{
    const char *name_end, *p;
    char *name;
    size_t i, matches = 0;
    int counted = 0, annotated = 0;
    while (start < end && (isspace((unsigned char)*start) || *start == '"')) start++;
    while (start < end && isdigit((unsigned char)*start)) { counted = 1; start++; }
    while (start < end && isspace((unsigned char)*start)) start++;
    name_end = end;
    for (p = start; p < end; p++) {
        if (*p == '(') {
            name_end = p;
            /* Ordinary room prose also has parentheses; only encounter-like
             * annotations justify reporting an otherwise unknown creature. */
            annotated = room_contains(p, end, "Gold Crowns") || room_contains(p, end, "Points") ||
                        room_contains(p, end, "WS ") || room_contains(p, end, "WS:");
            break;
        }
    }
    while (name_end > start && (isspace((unsigned char)name_end[-1]) ||
           name_end[-1] == '"' || name_end[-1] == '.')) name_end--;
    if (name_end == start) return;
    name = room_name(start, name_end);
    if (name == NULL) { buf->failed = 1; return; }
    for (i = 0; i < AHQ_REFERENCE_COUNT; i++)
        if (room_alias(name, ahq_references[i].aliases)) matches++;
    if (matches) {
        int heading = 0;
        for (i = 0; i < AHQ_REFERENCE_COUNT; i++) {
            if (!seen[i] && room_alias(name, ahq_references[i].aliases)) {
                if (!heading) {
                    room_append(buf, "\r\n");
                    if (matches > 1) {
                        room_append(buf, "Multiple source profiles for ");
                        room_append_n(buf, start, (size_t)(name_end - start));
                        room_append(buf, ": all variants shown below.\r\n\r\n");
                    }
                    heading = 1;
                }
                seen[i] = 1;
                room_append(buf, ahq_references[i].text);
            }
        }
    } else if ((counted || annotated) && !room_nonmonster(name)) {
        room_append(buf, "\r\nNo matching monster reference found: ");
        room_append_n(buf, start, (size_t)(name_end - start));
        room_append(buf, ".\r\n\r\n");
    }
    free(name);
}

GLOBAL _UBYTE *room_contents_text(_UBYTE **lines)
{
    ROOM_BUFFER buf = { NULL, 0, 0, 0 };
    const char *line, *start, *p;
    int depth, separator;
    unsigned char seen[PACK_LIMIT];
    if (lines == NULL || *lines == NULL)
        room_append(&buf, "No contents recorded for this room.");
    else for (; *lines != NULL && !buf.failed; lines++) {
        line = (const char *)*lines;
        room_append(&buf, line);
        room_append(&buf, "\r\n");
        memset(seen, 0, sizeof(seen));
        start = p = line;
        depth = 0;
        for (;;) {
            separator = 0;
            if (!*p) {
                room_clause(&buf, start, p, seen);
                break;
            }
            if (*p == '(') depth++;
            if (*p == ')' && depth > 0) depth--;
            if (!depth) {
                if (*p == ',' || *p == ';' || *p == '&') separator = 1;
                else if (room_join(p, " and ")) separator = 5;
                else if (room_join(p, " or ")) separator = 4;
            }
            if (separator) {
                room_clause(&buf, start, p, seen);
                p += separator;
                start = p;
            } else p++;
        }
    }
    if (buf.failed) { free(buf.text); return NULL; }
    return (_UBYTE *)buf.text;
}

/* Reuse the contents viewer's exact alias matching for encounter rosters. */
int room_monsters(_UBYTE **lines,void (*add)(const char *,const int *,int,void *),void *context)
{
    const char *start,*p,*end,*s; char *name;
    int depth,sep,count,counted,stats[9],review=0; size_t i,match,matches;
    if(!lines) return 0;
    for(;*lines;lines++) {
        start=p=(const char*)*lines; depth=0;
        /* Alternatives require adjudication; never spawn both choices. */
        for(s=p;*s;s++) if(room_join(s," or ")) break;
        if(*s) { review=1; continue; }
        for(;;) {
            sep=0;
            if(*p=='(') depth++;
            if(*p==')' && depth) depth--;
            if(!depth) {
                if(*p==',' || *p==';' || *p=='&') sep=1;
                else if(room_join(p," and ")) sep=5;
            }
            if(!*p || sep) {
                s=start; while(s<p && (isspace((unsigned char)*s)||*s=='"')) s++;
                count=1; counted=0;
                if(s<p && isdigit((unsigned char)*s)) {
                    counted=1; count=0; while(s<p && isdigit((unsigned char)*s)) { if(count<10000) count=count*10+(*s-'0'); s++; }
                }
                while(s<p && isspace((unsigned char)*s)) s++;
                end=s; while(end<p && *end!='(') end++;
                while(end>s && (isspace((unsigned char)end[-1])||end[-1]=='.'||end[-1]=='"')) end--;
                name=room_name(s,end); if(!name) return 1;
                matches=0; match=0;
                for(i=0;i<AHQ_REFERENCE_COUNT;i++) if(room_alias(name,ahq_references[i].aliases)) { match=i; matches++; }
                if(matches==1 && count>0 && count<=512) {
                    if(pack_monster_stats((int)match,stats)) {
                        /* Canonical singular alias makes plural encounters share identity. */
                        const char *alias=ahq_references[match].aliases; char canonical[64];
                        size_t len=strcspn(alias,"|"); if(len>63) len=63;
                        memcpy(canonical,alias,len); canonical[len]=0;
                        add(canonical,stats,count,context);
                    } else review=1;
                } else if(matches || ((counted || room_contains(s,p,"Gold Crowns") || room_contains(s,p,"Points") || room_contains(s,p,"WS ")) && !room_nonmonster(name))) review=1;
                free(name);
                if(!*p) break;
                p+=sep; start=p;
            } else p++;
        }
    }
    return review;
}

int room_melee(const char *name,int *dice,int hits[12])
{
    char *clean=room_name(name,name+strlen(name)),*end;
    const char *text=NULL,*p; size_t i; int matches=0,j,n,row[12]; long v;
    if(!pack_current()) { free(clean); return 0; }
    if(!pack_current()->legacy_references) {
        const PACK_PROFILE *entry=pack_monster(name); free(clean);
        if(!entry || !entry->melee.dice) return 0;
        *dice=entry->melee.dice; memcpy(hits,entry->melee.hit,sizeof(entry->melee.hit)); return 1;
    }
    if(!clean) return 0;
    for(i=0;i<AHQ_REFERENCE_COUNT;i++) if(room_alias(clean,ahq_references[i].aliases)) {
        text=ahq_references[i].text; matches++;
    }
    free(clean); if(matches!=1) return 0;
    p=strstr(text,"Hit roll:"); if(!p) return 0; p+=9;
    for(j=0;j<12;j++) {
        while(*p==' ' || *p=='\t') p++;
        if(!isdigit((unsigned char)*p)) return 0;
        v=strtol(p,&end,10); if(v<1 || v>12) return 0; row[j]=(int)v; p=end;
    }
    while(*p==' ' || *p=='\t') p++;
    if(*p!='\r' && *p!='\n') return 0;
    p=strstr(p,"Damage dice:"); if(!p) return 0; p+=12;
    while(*p==' ' || *p=='\t') p++;
    if(!isdigit((unsigned char)*p)) return 0;
    v=strtol(p,&end,10); if(v<1 || v>99) return 0; n=(int)v;
    while(*end==' ' || *end=='\t') end++;
    if(*end!='\r' && *end!='\n') return 0;
    *dice=n; memcpy(hits,row,sizeof(row)); return 1;
}

/* Strict unambiguous ranged lookup. A star outside weapon range has no effect;
 * special marked bands inside range require manual entry instead. */
int room_ranged(const char *name,int *range,int *dice,int hits[5],int *kind)
{
    char *clean=room_name(name,name+strlen(name)),*end; const char *text=NULL,*p,*max;
    size_t i; int matches=0,j,row[5],r,d,starts[5]={1,4,13,25,37}; long v;
    if(!pack_current()) { free(clean); return 0; }
    if(!pack_current()->legacy_references) {
        const PACK_PROFILE *entry=pack_monster(name); free(clean);
        if(!entry || !entry->ranged.kind) return 0;
        *range=entry->ranged.range; *dice=entry->ranged.dice; *kind=entry->ranged.kind;
        memcpy(hits,entry->ranged.hit,sizeof(entry->ranged.hit)); return 1;
    }
    if(!clean) return 0;
    for(i=0;i<AHQ_REFERENCE_COUNT;i++) if(room_alias(clean,ahq_references[i].aliases)) { text=ahq_references[i].text; matches++; }
    free(clean); if(matches!=1) return 0;
    p=strstr(text,"RANGED COMBAT"); if(!p) return 0;
    max=strstr(p,"Max range:");
    if(!max || sscanf(max,"Max range: %d    Damage dice: %d",&r,&d)!=2 || r<1 || r>480 || d<1 || d>99) return 0;
    p=strstr(p,"Hit roll:"); if(!p) return 0; p+=9;
    for(j=0;j<5;j++) {
        while(*p==' ' || *p=='\t') p++;
        if(!isdigit((unsigned char)*p)) return 0;
        v=strtol(p,&end,10); if(v<1 || v>12) return 0; row[j]=(int)v; p=end;
        if(*p=='*') { if(starts[j]<=r) return 0; p++; }
    }
    while(*p==' ' || *p=='\t') p++;
    if(*p!='\r' && *p!='\n') return 0;
    *kind=strstr(text,"crossbow")?2:strstr(text,"bow")?1:4;
    *range=r; *dice=d; memcpy(hits,row,sizeof(row)); return 1;
}
