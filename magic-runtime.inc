/* Included after map sight helpers, before movement and combat. */
static MAGIC_MODEL *magic_model(int side,int index)
{
    return side?&monsters.tokens[index].magic:&heroes[index].magic;
}
static int magic_toughness(int base,const MAGIC_MODEL *m) { return base<12?base+m->armour:base; }
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
    int i; magic_next_turn(); magic.exploration=1;
    for(i=0;i<hero_count;i++) heroes[i].magic.armour=heroes[i].magic.courage=heroes[i].magic.hand=0;
    for(i=0;i<monsters.count;i++) monsters.tokens[i].magic.armour=monsters.tokens[i].magic.courage=monsters.tokens[i].magic.hand=0;
    memset(magic.still_until,0,sizeof(magic.still_until));
}
