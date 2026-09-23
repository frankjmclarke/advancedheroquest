/* Monster encounters: manual adjudication, persistent adventure deaths. */
static int character_dead(const char *name)
{
    int i; for(i=0;i<monsters.dead_count;i++) if(!_stricmp(name,monsters.dead[i])) return 1;
    return 0;
}
static int monster_at(int x,int y)
{
    int i; PICE *p;
    if(!get_square(x,y,&p) || !p || !fog_visible(game_fog(),(_WORD)(p-Pice))) return -1;
    for(i=0;i<monsters.count;i++) if(monsters.tokens[i].wounds>0 && monsters.tokens[i].x==x && monsters.tokens[i].y==y) return i;
    return -1;
}
static int monster_can_move(int index,int x,int y)
{
    int i; PICE *p;
    if(!get_square(x,y,&p) || !p || p->type==EMPTY || p->type==TEST || !fog_visible(game_fog(),(_WORD)(p-Pice))) return 0;
    for(i=0;i<hero_count;i++) if(heroes[i].x==x && heroes[i].y==y) return 0;
    for(i=0;i<monsters.count;i++) if(i!=index && monsters.tokens[i].x==x && monsters.tokens[i].y==y) return 0;
    return index>=0 && index<monsters.count && monsters.tokens[index].wounds>0;
}
static int monster_place(int index)
{
    MONSTER *m=&monsters.tokens[index]; PICE *room=&Pice[m->room],*p,*neighbor;
    int x,y,pass,dx,dy,door;
    for(pass=0;pass<4;pass++) for(y=room->y;y<room->y+room->h;y++) for(x=room->x;x<room->x+room->w;x++) {
        if(!get_square(x,y,&p) || p!=room || !monster_can_move(index,x,y)) continue;
        door=0;
        for(dy=-1;dy<=1;dy++) for(dx=-1;dx<=1;dx++)
            if(get_square(x+dx,y+dy,&neighbor) && neighbor && (neighbor->type==DOOR || neighbor->type==SECRET)) door=1;
        if(pass<2 && door) continue;
        if(((x-room->x+y-room->y)&1)!=(pass&1)) continue;
        m->x=x; m->y=y; return 1;
    }
    return 0;
}
static void monster_add_reference(const char *name,const int *stats,int count,void *context)
{
    int room=*(int*)context,i; MONSTER *m;
    if(character_dead(name)) return;
    for(i=0;i<monsters.count;i++) if(monsters.tokens[i].unique && monsters.tokens[i].wounds && !_stricmp(name,monsters.tokens[i].name)) return;
    for(i=0;i<count;i++) {
        if(monsters.count==MONSTER_LIMIT) { monsters.seen[room]=2; return; }
        m=&monsters.tokens[monsters.count]; memset(m,0,sizeof(*m));
        strcpy(m->name,name); memcpy(m->stats,stats,sizeof(m->stats)); m->wounds=stats[7];
        if(room_melee(name,&m->melee.dice,m->melee.hit)) strcpy(m->melee.weapon,"Printed melee profile");
        monster_ranged_defaults(&m->ranged,name);
        m->room=room; m->x=m->y=-1; monsters.count++; monster_place(monsters.count-1);
    }
}
static void monsters_reveal(void)
{
    int i,changed=0; unsigned char *visible;
    if(!map_ready || !Pice) return;
    visible=game_fog(); if(!visible) return;
    /* Initial corridor visibility is not a room reveal. A new map stays empty. */
    for(i=0;i<MAX_PICE && i<ENCOUNTER_LIMIT;i++) {
        if(!visible[i] || monsters.seen[i] || !strchr("SNHXAQBM",Pice[i].type)) continue;
        monsters.seen[i]=1; changed=1;
        if(room_monsters(Pice[i].text,monster_add_reference,&i)) monsters.seen[i]=2;
    }
    if(changed) { active=1; dirty=1; undo_valid=0; }
}
static void monster_kill(int index)
{
    MONSTER *m=&monsters.tokens[index]; int i;
    if(!m->wounds) {
        m->x=m->y=-1;
        if(m->unique && !character_dead(m->name)) {
            strcpy(monsters.dead[monsters.dead_count++],m->name);
            /* The same unique character cannot remain alive elsewhere. */
            for(i=0;i<monsters.count;i++) if(!_stricmp(m->name,monsters.tokens[i].name)) {
                monsters.tokens[i].wounds=0; monsters.tokens[i].x=monsters.tokens[i].y=-1;
            }
        }
    }
}
static int monster_damage(int index)
{
    MONSTER *m;
    if(index<0 || index>=monsters.count || !monsters.tokens[index].wounds) return 0;
    m=&monsters.tokens[index];
    if(m->wounds==1 && m->unique && !character_dead(m->name) && monsters.dead_count==CHARACTER_LIMIT) {
        message("Character death record is full. Save this adventure before making further changes."); return 0;
    }
    remember(); m->wounds--;
    if(!m->wounds) monster_kill(index);
    active=1; game_changed(); return 1;
}
/* Stable species colours, independent of token number, dungeon and save order.
 * Unknown/custom monsters get a repeatable colour derived from their name. */
static COLORREF monster_colour(const char *name)
{
    static const struct { const char *word; COLORREF colour; } species[]={
        {"skeleton",RGB(207,192,145)}, {"zombie",RGB(112,120,57)},
        {"orc",RGB(36,120,65)}, {"goblin",RGB(25,133,137)},
        {"skaven",RGB(145,87,48)}, {"mummy",RGB(177,135,59)},
        {"ghoul",RGB(77,113,147)}, {"wight",RGB(82,72,153)},
        {"wraith",RGB(106,113,143)}, {"vampire",RGB(156,44,73)},
        {"chaos",RGB(119,48,145)}, {"ogre",RGB(173,87,44)},
        {"troll",RGB(45,119,110)}, {"minotaur",RGB(137,60,42)},
        {"fimir",RGB(131,139,48)}, {"bloodthirster",RGB(175,37,35)},
        {"rat",RGB(119,100,91)}, {"wolf",RGB(95,113,127)},
        {"bat",RGB(112,69,124)}
    };
    static const COLORREF custom[]={RGB(43,104,157),RGB(155,66,101),
        RGB(153,103,37),RGB(74,119,71),RGB(111,77,151),RGB(34,123,131),
        RGB(162,70,46),RGB(88,98,141),RGB(139,81,57),RGB(126,107,45),
        RGB(60,115,102),RGB(147,57,71)};
    char normalized[64]; size_t i,len; const char *p,*end;
    unsigned long hash=2166136261UL;
    for(i=0;name[i] && i<sizeof(normalized)-1;i++) {
        normalized[i]=(char)tolower((unsigned char)name[i]);
        if(normalized[i]=='-') normalized[i]=' ';
        hash=(hash^(unsigned char)normalized[i])*16777619UL;
    }
    normalized[i]=0;
    for(i=0;i<sizeof(species)/sizeof(species[0]);i++) {
        len=strlen(species[i].word); p=normalized;
        while((p=strstr(p,species[i].word))!=NULL) {
            end=p+len;
            if(*end=='s') end++;
            if((p==normalized || !isalpha((unsigned char)p[-1])) && !isalpha((unsigned char)*end)) return species[i].colour;
            p++;
        }
    }
    return custom[hash%(sizeof(custom)/sizeof(custom[0]))];
}
static void monster_draw(HDC dc,int x,int y,int size,int index)
{
    MONSTER *m=&monsters.tokens[index]; int saved=SaveDC(dc); char text[100]; RECT r;
    COLORREF colour=monster_colour(m->name);
    HBRUSH fill=CreateSolidBrush(colour);
    HPEN edge=CreatePen(PS_SOLID,index==selected_monster?3:2,RGB(239,137,117));
    SelectObject(dc,fill); SelectObject(dc,edge); Ellipse(dc,x+1,y+1,x+size-1,y+size-1);
    SetBkMode(dc,TRANSPARENT);
    SetTextColor(dc,299*GetRValue(colour)+587*GetGValue(colour)+114*GetBValue(colour)>150000?RGB(30,26,22):RGB(255,240,220));
    r.left=x; r.top=y; r.right=x+size; r.bottom=y+size;
    sprintf(text,"%d",index+1); DrawTextA(dc,text,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    if(size>=24) {
        r.left=x-size/2; r.right=x+size+size/2; r.top=y+size; r.bottom=r.top+16;
        sprintf(text,"%s %d [%dW]",m->name,index+1,m->wounds);
        SetTextColor(dc,RGB(255,240,220));
        SetBkMode(dc,OPAQUE); DrawTextA(dc,text,-1,&r,DT_CENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
    }
    RestoreDC(dc,saved); DeleteObject(fill); DeleteObject(edge);
}

