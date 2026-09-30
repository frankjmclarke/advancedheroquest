/* College of Light effects share pack references and ordinary saved spell state. */
static void light_collect(SPELL_CAST *c)
{
    const PACK_SPELL *sp=&pack_current()->spells[c->spell];
    int self=c->side?HERO_LIMIT+c->index:c->index,i,id,x=spell_target_x(self),y=spell_target_y(self),section=floor_piece(x,y);
    int reach=c->side?monsters.tokens[c->index].melee.reach:heroes[c->index].melee.reach;
    if(sp->effect!=SPELL_BLIND && sp->effect!=SPELL_FEAR && sp->effect!=SPELL_CLOAK && sp->effect!=SPELL_ESCAPE) return;
    c->count=0;
    for(i=0;i<hero_count+monsters.count;i++) {
        id=i<hero_count?i:HERO_LIMIT+i-hero_count;
        if(spell_target_x(id)<0) continue;
        if((sp->effect==SPELL_CLOAK && id==self) ||
            (sp->effect==SPELL_BLIND && (id>=HERO_LIMIT)!=c->side && floor_piece(spell_target_x(id),spell_target_y(id))==section) ||
            (sp->effect==SPELL_FEAR && (id>=HERO_LIMIT)==c->side && (id==self || battle_reach(x,y,spell_target_x(id),spell_target_y(id),reach))) ||
            (sp->effect==SPELL_ESCAPE && id<HERO_LIMIT && !HERO_DEAD(&heroes[id]))) c->targets[c->count++]=id;
    }
}
static int light_door(const SPELL_CAST *c)
{
    int i,self=c->side?HERO_LIMIT+c->index:c->index;
    int x=spell_target_x(self),y=spell_target_y(self),section=floor_piece(x,y);
    for(i=0;i<MAX_PICE;i++) {
        PICE *d=&Pice[i]; int dx,dy,other;
        if(d->type!=DOOR || game_fog()[i]) continue;
        dx=d->pos==East?1:d->pos==West?-1:0; dy=d->pos==North?1:d->pos==South?-1:0;
        if(!((c->x==d->x && c->y==d->y) || (c->x==d->x+dx && c->y==d->y+dy))) continue;
        if(!ranged_los(x,y,d->x,d->y) && !ranged_los(x,y,d->x+dx,d->y+dy)) continue;
        if(ranged_los(x,y,d->x,d->y)) other=floor_piece(d->x+dx,d->y+dy);
        else if(ranged_los(x,y,d->x+dx,d->y+dy)) other=floor_piece(d->x,d->y);
        else continue;
        if(other>=0 && d->pos==c->direction) return other;
    }
    return -1;
}
static void light_contents_monster(const char *name,const int *stats,int count,void *context)
{
    SPELL_CAST *c=(SPELL_CAST*)context; size_t len=strlen(c->result); (void)stats;
    snprintf(c->result+len,sizeof(c->result)-len,"%d %s\r\n",count,name);
}
static int light_preview(SPELL_CAST *c,int use_fate)
{
    const PACK_SPELL *sp=&pack_current()->spells[c->spell];
    int self=c->side?HERO_LIMIT+c->index:c->index,i,id,section=floor_piece(spell_target_x(self),spell_target_y(self));
    size_t used=strlen(c->result); (void)use_fate;
    memset(c->damage,0,sizeof(c->damage));
    snprintf(c->result+used,sizeof(c->result)-used,"%s\r\n",sp->text);
    if(sp->effect==SPELL_LEARNING) {
        section=light_door(c);
        used=strlen(c->result); snprintf(c->result+used,sizeof(c->result)-used,"\r\nVisible contents beyond the door:\r\n");
        if(monsters.seen[section]) {
            for(i=0;i<monsters.count;i++) if(monsters.tokens[i].room==section && monsters.tokens[i].wounds)
                light_contents_monster(monsters.tokens[i].name,monsters.tokens[i].stats,1,c);
        } else room_monsters(Pice[section].text,light_contents_monster,c);
        /* The raw encounter narrative contains hidden traps and treasure totals.
           Only explicit visible feature labels are safe to disclose. */
        {
            static const char *features[]={"","","","","Maiden","Witch","Man at Arms","Rogue","Chasm","Statue","Rats","Bats","Mould","Mushrooms","Grate","Pool","Magic circle","","Throne","Wight","Rockfall","Slime","Cess pit","Bridge","Well","Moths","Apparition","Chest (contents concealed)","Stairs and chest (contents concealed)","Tomb","Weapons rack","Cupboard","Table","Fireplace","Bookcase","Rack"};
            int f=Pice[section].feature;
            if(f>=0 && f<sizeof(features)/sizeof(features[0]) && features[f][0]) {
                used=strlen(c->result); snprintf(c->result+used,sizeof(c->result)-used,"%s\r\n",features[f]);
            }
        }
        used=strlen(c->result); snprintf(c->result+used,sizeof(c->result)-used,"Hidden traps, secrets and treasure amounts are not disclosed. Other visible narrative details require GM inspection.");
    }
    if(sp->effect==SPELL_BANISH) {
        int roll,traits,t,dice,required,distance,success=1; char rolls[2048];
        const RANGED_PROFILE *r=c->side?&monsters.tokens[c->index].ranged:&heroes[c->index].ranged;
        id=c->targets[0]; traits=magic_traits(id>=HERO_LIMIT,id>=HERO_LIMIT?id-HERO_LIMIT:id);
        distance=abs(spell_target_x(self)-spell_target_x(id))+abs(spell_target_y(self)-spell_target_y(id));
        required=r->hit[ranged_band(distance)];
        if(!required) { strcpy(c->result,"Configure the caster's ranged hit profile for this distance first."); return 0; }
        if(!c->effect_roll && !magic_die(&c->effect_roll)) return 0;
        roll=c->effect_roll; used=strlen(c->result);
        snprintf(c->result+used,sizeof(c->result)-used,"\r\nRanged D12 %d versus %d (caster's ranged hit profile): %s.\r\n",roll,required,roll-magic_model(c->side,c->index)->blind>=required?"hit":"miss");
        if(roll-magic_model(c->side,c->index)->blind<required) { c->resolved=1; return 1; }
        if(traits==TRAIT_LESSER_DAEMON) {
            if(!c->test_ready) { if(!magic_die(&c->test_roll)) return 0; c->test_ready=1; }
            success=c->test_roll<=(c->side?monsters.tokens[c->index].stats[6]:heroes[c->index].stats[6]);
            c->fate_used=!success && use_fate && !c->side && heroes[c->index].fate>0;
            success=success || c->fate_used;
            used=strlen(c->result); snprintf(c->result+used,sizeof(c->result)-used,"Intelligence D12 %d: %s.\r\n",c->test_roll,success?"success":"failure");
            if(!success && !use_fate && !c->fate_offer && !c->side && heroes[c->index].fate>0) { c->fate_offer=1; return 0; }
        }
        if(traits==TRAIT_UNDEAD || (traits==TRAIT_LESSER_DAEMON && success)) c->damage[0]=id<HERO_LIMIT?heroes[id].wounds:monsters.tokens[id-HERO_LIMIT].wounds;
        else {
            t=id<HERO_LIMIT?heroes[id].stats[3]:monsters.tokens[id-HERO_LIMIT].stats[3]; dice=t;
            if(!spell_damage_roll(rolls,sizeof(rolls),dice,t,&c->damage[0])) return 0;
            used=strlen(c->result); snprintf(c->result+used,sizeof(c->result)-used,"Damage D12: %s\r\n",rolls);
        }
        used=strlen(c->result); snprintf(c->result+used,sizeof(c->result)-used,"%s: %d Wound(s).",spell_target_name(id),c->damage[0]);
    }
    c->fate_offer=0; c->resolved=1; return 1;
}
static void light_apply(SPELL_CAST *c)
{
    const PACK_SPELL *sp=&pack_current()->spells[c->spell];
    int self=c->side?HERO_LIMIT+c->index:c->index,x=spell_target_x(self),y=spell_target_y(self),section=floor_piece(x,y),i,id;
    MAGIC_MODEL *caster=magic_model(c->side,c->index);
    if(sp->effect==SPELL_CLOAK) caster->cloak=1;
    if(sp->effect==SPELL_VENOM) magic.venom[section]=1;
    if(sp->effect==SPELL_BLIND || sp->effect==SPELL_FEAR) for(i=0;i<hero_count+monsters.count;i++) {
        int reach=c->side?monsters.tokens[c->index].melee.reach:heroes[c->index].melee.reach;
        id=i<hero_count?i:HERO_LIMIT+i-hero_count;
        if(spell_target_x(id)<0) continue;
        if(sp->effect==SPELL_BLIND && (id>=HERO_LIMIT)!=c->side && floor_piece(spell_target_x(id),spell_target_y(id))==section) magic_model(id>=HERO_LIMIT,id>=HERO_LIMIT?id-HERO_LIMIT:id)->blind=1;
        if(sp->effect==SPELL_FEAR && (id>=HERO_LIMIT)==c->side && (id==self || battle_reach(x,y,spell_target_x(id),spell_target_y(id),reach))) magic_model(id>=HERO_LIMIT,id>=HERO_LIMIT?id-HERO_LIMIT:id)->fear=1;
    }
    for(i=0;i<c->count;i++) {
        id=c->targets[i];
        if(sp->effect==SPELL_STRENGTH) magic_model(id>=HERO_LIMIT,id>=HERO_LIMIT?id-HERO_LIMIT:id)->strength=1;
        if(sp->effect==SPELL_REGEN) magic_model(id>=HERO_LIMIT,id>=HERO_LIMIT?id-HERO_LIMIT:id)->regen=magic.combat+1;
        if(sp->effect==SPELL_SLEEP) magic_model(id>=HERO_LIMIT,id>=HERO_LIMIT?id-HERO_LIMIT:id)->sleep=1;
    }
    if(sp->effect==SPELL_ESCAPE) {
        magic.escaped[section]=1;
        for(i=0;i<hero_count;i++) if(!HERO_DEAD(&heroes[i]) && heroes[i].x>=0) {
            heroes[i].x=heroes[i].y=-1; heroes[i].focus=0;
            heroes[i].move_spent=heroes[i].run_bonus=heroes[i].moved=heroes[i].attacked=0;
        }
        for(i=0;i<monsters.count;i++) monsters.tokens[i].focus=0;
        magic_end_combat(); turn_phase=0; turn_menu_update();
    }
    caster->restore_blocked=1;
}
