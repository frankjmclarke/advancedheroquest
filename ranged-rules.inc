/* Shared combat geometry and turn state. Focus is an opposing roster index + 1. */
static int battle_floor(int x,int y)
{
    PICE *p; unsigned char *visible=game_fog();
    if(!visible) return 0;
    return get_square(x,y,&p) && p && p->type!=EMPTY && p->type!=TEST && fog_visible(visible,(_WORD)(p-Pice));
}
static int battle_edge(int x,int y,int nx,int ny)
{
    int i,dx,dy; PICE *p,*q,*d;
    if(abs(x-nx)+abs(y-ny)!=1 || !battle_floor(x,y) || !battle_floor(nx,ny)) return 0;
    get_square(x,y,&p); get_square(nx,ny,&q);
    for(i=0;i<MAX_PICE;i++) {
        d=&Pice[i]; if(d->type==EMPTY || d->type==TEST) continue;
        if(d->type==DOOR || d->type==SECRET) {
            if(!game_fog()[i]) continue;
            dx=d->pos==East?1:d->pos==West?-1:0; dy=d->pos==North?1:d->pos==South?-1:0;
            if((x==d->x && y==d->y && nx==x+dx && ny==y+dy) ||
               (nx==d->x && ny==d->y && x==nx+dx && y==ny+dy)) return 1;
        } else if(x>=d->x && x<d->x+d->w && y>=d->y && y<d->y+d->h &&
                  nx>=d->x && nx<d->x+d->w && ny>=d->y && ny<d->y+d->h) return 1;
    }
    return strchr("PELRTCOD",p->type)!=NULL && strchr("PELRTCOD",q->type)!=NULL;
}
static void focus_prune(void)
{
    int i,n;
    if(!map_ready || !Pice) return;
    for(i=0;i<hero_count;i++) {
        HERO *h=&heroes[i]; n=h->focus-1;
        if(n>=0 && (n>=monsters.count || h->x<0 || monsters.tokens[n].x<0 ||
           !battle_edge(h->x,h->y,monsters.tokens[n].x,monsters.tokens[n].y))) h->focus=0;
    }
    for(i=0;i<monsters.count;i++) {
        MONSTER *m=&monsters.tokens[i]; n=m->focus-1;
        if(n>=0 && (n>=hero_count || m->x<0 || heroes[n].x<0 ||
           !battle_edge(m->x,m->y,heroes[n].x,heroes[n].y))) m->focus=0;
    }
}
static int death_zone(int side,int index,int x,int y)
{
    int i;
    if(side) {
        for(i=0;i<hero_count;i++) if(heroes[i].wounds && heroes[i].x>=0 &&
            (!heroes[i].focus || heroes[i].focus==index+1) && battle_edge(x,y,heroes[i].x,heroes[i].y)) return 1;
    } else {
        for(i=0;i<monsters.count;i++) if(monsters.tokens[i].wounds && monsters.tokens[i].x>=0 &&
            (!monsters.tokens[i].focus || monsters.tokens[i].focus==index+1) && battle_edge(x,y,monsters.tokens[i].x,monsters.tokens[i].y)) return 1;
    }
    return 0;
}
static int entered_zone(int side,int index,int px,int py,int x,int y)
{
    int i,ex,ey,focus,alive;
    for(i=0;i<(side?hero_count:monsters.count);i++) {
        ex=side?heroes[i].x:monsters.tokens[i].x; ey=side?heroes[i].y:monsters.tokens[i].y;
        focus=side?heroes[i].focus:monsters.tokens[i].focus; alive=side?heroes[i].wounds:monsters.tokens[i].wounds;
        if(alive && ex>=0 && (!focus || focus==index+1) && battle_edge(x,y,ex,ey) && !battle_edge(px,py,ex,ey)) return 1;
    }
    return 0;
}
static void focus_enter(int side,int index)
{
    int i,x=side?monsters.tokens[index].x:heroes[index].x,y=side?monsters.tokens[index].y:heroes[index].y;
    focus_prune();
    if(side) {
        for(i=0;i<hero_count;i++) if(heroes[i].wounds && !heroes[i].focus && battle_edge(x,y,heroes[i].x,heroes[i].y)) heroes[i].focus=index+1;
    } else {
        for(i=0;i<monsters.count;i++) if(monsters.tokens[i].wounds && !monsters.tokens[i].focus && battle_edge(x,y,monsters.tokens[i].x,monsters.tokens[i].y)) monsters.tokens[i].focus=index+1;
    }
}
static int battle_occupied(int x,int y)
{
    int i;
    for(i=0;i<hero_count;i++) if(heroes[i].x==x && heroes[i].y==y) return 1;
    for(i=0;i<monsters.count;i++) if(monsters.tokens[i].x==x && monsters.tokens[i].y==y) return 1;
    return 0;
}
/* Centre-to-centre grid ray. Exact corner contacts do not count as crossing
 * either side figure; accept either open two-edge route around the corner. */
static int ranged_los(int x,int y,int tx,int ty)
{
    int dx=abs(tx-x),dy=abs(ty-y),sx=tx>x?1:-1,sy=ty>y?1:-1,ix=0,iy=0,nx,ny,a,b;
    if(!battle_floor(x,y) || !battle_floor(tx,ty)) return 0;
    while(ix<dx || iy<dy) {
        a=(1+2*ix)*dy; b=(1+2*iy)*dx; nx=x; ny=y;
        if(a==b) {
            nx+=sx; ny+=sy;
            if(!((battle_edge(x,y,nx,y) && battle_edge(nx,y,nx,ny)) ||
                 (battle_edge(x,y,x,ny) && battle_edge(x,ny,nx,ny)))) return 0;
            ix++; iy++;
        } else if(a<b) { nx+=sx; ix++; if(!battle_edge(x,y,nx,ny)) return 0; }
        else { ny+=sy; iy++; if(!battle_edge(x,y,nx,ny)) return 0; }
        x=nx; y=ny;
        if((x!=tx || y!=ty) && battle_occupied(x,y)) return 0;
    }
    return 1;
}
/* With opponents on the board, use a shortest open orthogonal path. A move
 * stops at its first new enemy zone. Reserve placement stays a creative action. */
static int battle_destination(int side,int index,int *tx,int *ty)
{
    int x=side?monsters.tokens[index].x:heroes[index].x,y=side?monsters.tokens[index].y:heroes[index].y;
    int *parent,*queue,total=Xsize*Ysize,head=0,tail=0,i,j,n,nx,ny,start,goal,at,previous,opponents=0;
    const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
    if(x<0) return 1;
    if(side) { for(i=0;i<hero_count;i++) if(heroes[i].x>=0 && heroes[i].wounds) opponents++; }
    else { for(i=0;i<monsters.count;i++) if(monsters.tokens[i].x>=0 && monsters.tokens[i].wounds) opponents++; }
    if(!opponents) return 1;
    if(*tx<0 || *ty<0 || *tx>=Xsize || *ty>=Ysize || total<1) return 0;
    parent=(int*)malloc(total*sizeof(int)); queue=(int*)malloc(total*sizeof(int));
    if(!parent || !queue) { free(parent); free(queue); return 0; }
    for(i=0;i<total;i++) parent[i]=-1;
    start=y*Xsize+x; goal=*ty*Xsize+*tx; parent[start]=start; queue[tail++]=start;
    while(head<tail && parent[goal]<0) {
        at=queue[head++];
        for(j=0;j<4;j++) {
            nx=at%Xsize+dx[j]; ny=at/Xsize+dy[j];
            if(nx<0 || ny<0 || nx>=Xsize || ny>=Ysize) continue;
            n=ny*Xsize+nx;
            if(parent[n]>=0 || battle_occupied(nx,ny) || !battle_edge(at%Xsize,at/Xsize,nx,ny)) continue;
            parent[n]=at; queue[tail++]=n;
        }
    }
    if(parent[goal]<0) { free(parent); free(queue); return 0; }
    n=0; for(at=goal;at!=start;at=parent[at]) queue[n++]=at;
    previous=start;
    for(i=n-1;i>=0;i--) {
        at=queue[i];
        if(entered_zone(side,index,previous%Xsize,previous/Xsize,at%Xsize,at/Xsize)) { goal=at; break; }
        previous=at;
    }
    *tx=goal%Xsize; *ty=goal/Xsize; free(parent); free(queue); return 1;
}
static int ranged_band(int range) { return range<=3?0:range<=12?1:range<=24?2:range<=36?3:4; }
static const char *ranged_eligibility(int h,int m,int side,const RANGED_PROFILE *p,int moved)
{
    int x,y,tx,ty,d;
    if(h<0 || h>=hero_count || m<0 || m>=monsters.count || !heroes[h].wounds || !monsters.tokens[m].wounds || heroes[h].x<0 || monsters.tokens[m].x<0) return "Both models must be alive and placed.";
    x=side?monsters.tokens[m].x:heroes[h].x; y=side?monsters.tokens[m].y:heroes[h].y;
    tx=side?heroes[h].x:monsters.tokens[m].x; ty=side?heroes[h].y:monsters.tokens[m].y;
    d=abs(tx-x)+abs(ty-y);
    if(d<=1) return "Orthogonally adjacent targets require hand-to-hand combat.";
    if(!p->kind) return "No ranged weapon equipped. Set the attacker ranged profile.";
    if(p->range && d>p->range) return "Target is out of range.";
    if(!p->range || !p->dice || !p->hit[ranged_band(d)]) return "Configure maximum range, damage and this range band's hit roll.";
    if(moved && (p->kind==1 || p->kind==2)) return "Bow/crossbow: attacker has moved this turn. Use Next Turn when appropriate.";
    if(death_zone(side,side?m:h,x,y)) return "Attacker is in an enemy death zone.";
    if(!ranged_los(x,y,tx,ty)) return "Line of sight is blocked by a figure, wall, closed door or unexplored area.";
    return NULL;
}
