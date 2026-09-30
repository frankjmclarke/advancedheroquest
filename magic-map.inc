/* Map effects use the existing campaign tables and piece slots. */
static int magic_die(int *roll)
{
    HCRYPTPROV provider; int ok;
    if(!CryptAcquireContext(&provider,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT)) return 0;
    ok=melee_d12(provider,roll); CryptReleaseContext(provider,0); return ok;
}
static int magic_empty_slot(void)
{
    int i; for(i=0;i<MAX_PICE && i<ENCOUNTER_LIMIT;i++) if(Pice[i].type==EMPTY) return i; return -1;
}
static int magic_door_edge(int x,int y,int direction)
{
    int i,dx=direction==East?1:direction==West?-1:0,dy=direction==North?1:direction==South?-1:0;
    for(i=0;i<MAX_PICE;i++) if(Pice[i].type==DOOR || Pice[i].type==SECRET) {
        int ex=Pice[i].pos==East?1:Pice[i].pos==West?-1:0,ey=Pice[i].pos==North?1:Pice[i].pos==South?-1:0;
        if((Pice[i].x==x && Pice[i].y==y && ex==dx && ey==dy) ||
           (Pice[i].x==x+dx && Pice[i].y==y+dy && ex==-dx && ey==-dy)) return i;
    }
    return -1;
}
static const char *magic_wall_reason(int side,int index,int x,int y,int direction)
{
    int section=floor_piece(side?monsters.tokens[index].x:heroes[index].x,side?monsters.tokens[index].y:heroes[index].y);
    int dx=direction==East?1:direction==West?-1:0,dy=direction==North?1:direction==South?-1:0;
    if(direction<0 || direction>3 || section<0 || floor_piece(x,y)!=section) return "Choose an inside wall square in the caster's section and its outward direction.";
    if(floor_piece(x+dx,y+dy)==section || magic_door_edge(x,y,direction)>=0 || sight_edge(x,y,x+dx,y+dy,game_fog())) return "That edge has no wall, or already has a door.";
    if(x+dx<0 || y+dy<0 || x+dx>=Xsize || y+dy>=Ysize) return "The door would leave the map. Choose another wall.";
    if(magic_empty_slot()<0) return "No spare map-piece slot is available for a magic door.";
    return NULL;
}
static int magic_generate_behind(const PICE *door)
{
    PICE room; int slot=magic_empty_slot(),x,y; ROOMS kind; TYPE type; ROOMORPASSAGE choice;
    if(slot<0 || !room_or_passage_table || !room_type_table) return 0;
    choice=room_or_passage(); kind=choice==Passage?Invalid:room_type();
    type=kind==Small?SMALL_ROOM:kind==Hazard?HAZARD:kind==Large?LARGE_ROOM:kind==Lair?LAIR:kind==Quest?QUEST:kind==Big?BIG:kind==Merscha?MERSCHA:kind==Normal?NORMAL_ROOM:PASSAGE;
    memset(&room,0,sizeof(room)); if(!init_pice(&room,type,0,0,door->pos)) return 0;
    room.x=door->pos==East?door->x+1:door->pos==West?door->x-room.w:door->x-room.w/2;
    room.y=door->pos==North?door->y+1:door->pos==South?door->y-room.h:door->y-room.h/2;
    if(!test_pice(&room)) return 0;
    switch(type) {
    case SMALL_ROOM: if(!small_features(&room)) return 0; break;
    case NORMAL_ROOM: if(!normal_features(&room)) return 0; break;
    case HAZARD: if(!hazard_features(&room)) return 0; break;
    case LARGE_ROOM: if(!large_features(&room)) return 0; break;
    case LAIR: if(!lair_features(&room)) return 0; break;
    case QUEST: if(!quest_features(&room)) return 0; break;
    case BIG: if(!big_features(&room)) return 0; break;
    case MERSCHA: if(!merscha_features(&room)) return 0; break;
    default: if(!passage_features(&room)) return 0; break;
    }
    Pice[slot]=room;
    for(y=room.y;y<room.y+room.h;y++) for(x=room.x;x<room.x+room.w;x++) set_square(x,y,&Pice[slot]);
    if(!makemap_extend(&Pice[slot])) {
        int n; memcpy(Pice,undo_pieces,sizeof(PICE)*MAX_PICE);
        for(n=0;n<Xsize*Ysize;n++) set_square(n%Xsize,n/Xsize,undo_cells[n]<0?NULL:&Pice[undo_cells[n]]);
        return 0;
    }
    /* The ordinary continuation may also place an entrance door on this edge.
       Keep the original Bright Key door as the single saved opening. */
    {
        int i,owner=(int)(door-Pice),dx=door->pos==East?1:door->pos==West?-1:0,dy=door->pos==North?1:door->pos==South?-1:0;
        for(i=0;i<MAX_PICE;i++) if(i!=owner && (Pice[i].type==DOOR || Pice[i].type==SECRET)) {
            PICE *d=&Pice[i],*cell; int ex=d->pos==East?1:d->pos==West?-1:0,ey=d->pos==North?1:d->pos==South?-1:0;
            if((d->x==door->x && d->y==door->y && ex==dx && ey==dy) ||
               (d->x==door->x+dx && d->y==door->y+dy && ex==-dx && ey==-dy)) {
                int x=d->x,y=d->y,p; d->type=EMPTY;
                if(get_square(x,y,&cell) && cell==d) { p=floor_piece(x,y); set_square(x,y,p<0?NULL:&Pice[p]); }
            }
        }
    }
    game_fog()[slot]=1; return 1;
}
/* A supplied roll separates rule resolution from UI and makes it testable. */
static int magic_open_index(int i,int roll)
{
    PICE *door=&Pice[i]; unsigned char *visible=game_fog(); int dx,dy,other;
    if(magic.doors[i]==3) { panel_note("Bright Key: solid rock behind this door. It cannot be opened."); return 1; }
    if(visible[i]) return 0;
    if(magic.doors[i]==1 && (roll<1 || roll>12)) return 0;
    remember();
    if(magic.doors[i]==1) {
        magic.doors[i]=roll<=4?3:2;
        if(roll<=4) { panel_note("Bright Key opening roll: 1-4. Solid rock; the door remains blocked."); return 1; }
    }
    dx=door->pos==East?1:door->pos==West?-1:0; dy=door->pos==North?1:door->pos==South?-1:0;
    other=floor_piece(door->x+dx,door->y+dy);
    if(other<0 && !magic_generate_behind(door)) {
        panel_note("Bright Key passed its rock test, but the generated section cannot be placed. The door stays closed; clear map space before retrying. Its rock test will not be rerolled."); return 1;
    }
    visible[i]=1; if(other>=0) visible[other]=1;
    panel_note("Bright Key door opened. The section beyond is now revealed."); return 1;
}
int game_open_door(unsigned char *visible,int pixel_x,int pixel_y)
{
    int i,x,y,roll;
    for(i=MAX_PICE-1;i>=0;i--) if((Pice[i].type==DOOR || Pice[i].type==SECRET) && fog_visible(visible,i)) {
        PICE *door=&Pice[i]; x=(door->x+1)*ICON_SCALE; y=(Ysize-door->y)*ICON_SCALE;
        if(door->pos==North) y-=ICON_SCALE/2; else if(door->pos==South) y+=ICON_SCALE/2;
        else if(door->pos==East) x+=ICON_SCALE/2; else x-=ICON_SCALE/2;
        if(pixel_x<x || pixel_x>=x+ICON_SCALE || pixel_y<y || pixel_y>=y+ICON_SCALE) continue;
        if(i>=ENCOUNTER_LIMIT || !magic.doors[i]) { if(!visible[i]) remember(); return fog_open_door(visible,pixel_x,pixel_y); }
        roll=0;
        if(magic.doors[i]==1 && !magic_die(&roll)) { message("Dice generator unavailable. The door remains closed."); return 1; }
        return magic_open_index(i,roll);
    }
    return fog_open_door(visible,pixel_x,pixel_y);
}
