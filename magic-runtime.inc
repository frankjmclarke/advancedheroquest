/* Included after map sight helpers, before movement and combat. */
static MAGIC_MODEL *magic_model(int side,int index)
{
    return side?&monsters.tokens[index].magic:&heroes[index].magic;
}
static int magic_toughness(int base,const MAGIC_MODEL *m) { int t=base+m->armour+m->cloak; return base>=12?base:t>12?12:t; }
static int magic_traits(int side,int index)
{
    const PACK_PROFILE *p=pack_profile(side?monsters.tokens[index].profile_id:heroes[index].profile_id);
    return p?p->traits:0;
}
static int magic_attack_toughness(int base,const MAGIC_MODEL *m,int traits)
{
    int t=magic_toughness(base,m);
    if(m->cloak && traits && t<12) t++;
    return t;
}
static int magic_ws(int base,const MAGIC_MODEL *m) { return base<12?base+m->strength:base; }
static int magic_hit_modifier(const MAGIC_MODEL *attacker,const MAGIC_MODEL *defender) { return (defender->sleep?4:0)-attacker->blind; }
static void magic_begin_combat(void)
{
    if(!magic.combat_active) {
        if(magic.combat<1000000) magic.combat++;
        if(magic.trap_event<1000000) magic.trap_event++; /* closes an earlier trap resurrection window */
        magic.combat_active=1; magic.exploration=0;
    }
}
static void magic_end_combat(void)
{
    int i;
    if(!magic.combat_active) return;
    magic.combat_active=0; magic.last_combat=magic.combat;
    for(i=0;i<hero_count;i++) { heroes[i].magic.restore_blocked=0; if(heroes[i].magic.regen<=magic.combat) heroes[i].magic.regen=0; }
    for(i=0;i<monsters.count;i++) if(monsters.tokens[i].magic.regen<=magic.combat) monsters.tokens[i].magic.regen=0;
}
static int magic_gm_phase(void)
{
    int i,restored=0;
    for(i=0;i<hero_count;i++) {
        HERO *h=&heroes[i];
        if(h->magic.regen && h->x>=0 && !HERO_DEAD(h) && h->wounds<h->stats[7]) {
            h->wounds++; restored++; if(HERO_KO(h)) h->condition=0;
        }
    }
    return restored;
}
static int magic_bravery(int base,const MAGIC_MODEL *m) { return m->courage?12:base; }
static int magic_hand(const MAGIC_MODEL *m)
{
    return turn_phase && m->hand && magic.turn>m->hand_turn;
}
static int magic_stopped(int index)
{
    MONSTER *m=&monsters.tokens[index]; int p=floor_piece(m->x,m->y);
    return p>=0 && p<ENCOUNTER_LIMIT && magic.still_until[p]>magic.turn;
}
static void magic_hero_death(HERO *h)
{
    if(HERO_DEAD(h) && h->x>=0) {
        h->magic.death_turn=magic.turn+1; /* zero means death timing is unknown */
        if(magic.combat_active) h->magic.death_combat=magic.combat;
        else {
            int i; if(magic.trap_event<1000000) magic.trap_event++;
            h->magic.death_combat=-magic.trap_event;
            for(i=0;i<hero_count;i++) heroes[i].magic.restore_blocked=0;
        }
        h->magic.corpse_x=h->x; h->magic.corpse_y=h->y;
        h->x=h->y=-1;
    }
}
static void magic_monster_death(MONSTER *m)
{
    if(!m->wounds && m->x>=0) {
        m->magic.death_turn=magic.turn+1;
        m->magic.corpse_x=m->x; m->magic.corpse_y=m->y;
        m->x=m->y=-1;
    }
}
static void magic_next_turn(void)
{
    int i; if(magic.turn<1000000) magic.turn++;
    for(i=0;i<hero_count;i++) heroes[i].magic.swift=0;
    for(i=0;i<monsters.count;i++) monsters.tokens[i].magic.swift=0;
}
static void magic_exploration(void)
{
    int i; magic_end_combat(); magic_next_turn(); magic.exploration=1;
    for(i=0;i<hero_count;i++) heroes[i].magic.armour=heroes[i].magic.courage=heroes[i].magic.hand=0;
    for(i=0;i<monsters.count;i++) monsters.tokens[i].magic.armour=monsters.tokens[i].magic.courage=monsters.tokens[i].magic.hand=0;
    for(i=0;i<hero_count;i++) heroes[i].magic.strength=heroes[i].magic.cloak=heroes[i].magic.blind=heroes[i].magic.fear=0;
    for(i=0;i<monsters.count;i++) monsters.tokens[i].magic.strength=monsters.tokens[i].magic.cloak=monsters.tokens[i].magic.blind=monsters.tokens[i].magic.fear=0;
    memset(magic.still_until,0,sizeof(magic.still_until));
}
