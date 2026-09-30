/* Inert spell targeting and map annotations. Geometry comes from the pack. */
static int spell_square_visible(int x,int y,const unsigned char *visible)
{
    int p=floor_piece(x,y);
    return p>=0 && (!visible || (fog_visible(visible,p) && game_corridor_visible(x,y)));
}
static int spell_template_contains(const SPELL_CAST *c,int id)
{
    const PACK_SPELL *sp=&pack_current()->spells[c->spell];
    int x=spell_target_x(id),y=spell_target_y(id);
    if(id<HERO_LIMIT?HERO_DEAD(&heroes[id]):monsters.tokens[id-HERO_LIMIT].wounds<=0) return 0;
    return x>=c->x && x<c->x+sp->template_width && y>=c->y && y<c->y+sp->template_height &&
        spell_square_visible(x,y,game_fog());
}
static void spell_template_collect(SPELL_CAST *c)
{
    int i,id; c->count=0;
    for(i=0;i<hero_count+monsters.count;i++) {
        id=i<hero_count?i:HERO_LIMIT+i-hero_count;
        if(spell_template_contains(c,id)) c->targets[c->count++]=id;
    }
}
static int spell_template_matches(const SPELL_CAST *c)
{
    int i,j,id,count=0;
    for(i=0;i<hero_count+monsters.count;i++) {
        id=i<hero_count?i:HERO_LIMIT+i-hero_count;
        if(!spell_template_contains(c,id)) continue;
        count++;
        for(j=0;j<c->count && c->targets[j]!=id;j++);
        if(j==c->count) return 0;
    }
    return count==c->count;
}
static SPELL_CAST *spell_map_cast(void)
{
    return spell_map_dialog?(SPELL_CAST*)GetWindowLongPtr(spell_map_dialog,DWLP_USER):NULL;
}
static void spell_map_selection(HWND w,SPELL_CAST *c)
{
    int i,n,id; const PACK_SPELL *sp;
    if(c->spell<0) return;
    sp=&pack_current()->spells[c->spell];
    if(sp->target==2) spell_template_collect(c);
    else return; /* model/group selections are gathered from the list */
    c->syncing=1;
    n=(int)SendDlgItemMessage(w,SCTARGETS,LB_GETCOUNT,0,0);
    SendDlgItemMessage(w,SCTARGETS,LB_SETSEL,FALSE,-1);
    for(i=0;i<n;i++) {
        id=(int)SendDlgItemMessage(w,SCTARGETS,LB_GETITEMDATA,i,0);
        if(spell_template_contains(c,id)) SendDlgItemMessage(w,SCTARGETS,LB_SETSEL,TRUE,i);
    }
    c->syncing=0;
    cast_dialog_status(w,c);
}
static void spell_map_resume(void)
{
    if(!spell_map_dialog) return;
    spell_map_picking=0; ShowWindow(spell_map_dialog,SW_SHOW); SetForegroundWindow(spell_map_dialog);
}
static _BOOL spell_message_filter(_VOID *message)
{
    MSG *msg=(MSG*)message;
    if(spell_map_dialog) {
        if(msg->hwnd==spell_map_dialog || IsChild(spell_map_dialog,msg->hwnd)) {
            return IsDialogMessageA(spell_map_dialog,msg);
        } else if(msg->message==WM_KEYDOWN && msg->wParam==VK_ESCAPE && spell_map_picking) {
            spell_map_resume(); return TRUE;
        }
    }
    return FALSE;
}
static void spell_keyboard_start(void)
{
    Wind_SetMessageFilter(spell_message_filter);
}
static void spell_keyboard_stop(void)
{
    Wind_SetMessageFilter(NULL);
}
static int spell_corpse_at(int x,int y,const unsigned char *visible)
{
    int i,id,fallback=-1; MAGIC_MODEL *m;
    if(!spell_square_visible(x,y,visible)) return -1;
    for(i=0;i<hero_count+monsters.count;i++) {
        id=i<hero_count?i:HERO_LIMIT+i-hero_count;
        if(!(id<HERO_LIMIT?HERO_DEAD(&heroes[id]):monsters.tokens[id-HERO_LIMIT].wounds<=0)) continue;
        m=magic_model(id>=HERO_LIMIT,id>=HERO_LIMIT?id-HERO_LIMIT:id);
        if(m->death_turn && m->corpse_x==x && m->corpse_y==y) {
            if(!m->soul_lost && m->death_turn==magic.turn) return id;
            if(fallback<0) fallback=id;
        }
    }
    return fallback;
}
static int spell_map_click(int x,int y,const unsigned char *visible)
{
    SPELL_CAST *c=spell_map_cast(); const PACK_SPELL *sp; int id=-1,i,n,select,found=0;
    HWND w=spell_map_dialog;
    if(!c || c->spell<0) return 0;
    if(c->committed) { MessageBeep(MB_ICONWARNING); spell_map_resume(); return 1; }
    sp=&pack_current()->spells[c->spell];
    if(x<0 || y<0 || x>=Xsize || y>=Ysize ||
       (sp->target!=6 && !spell_square_visible(x,y,game_fog()))) {
        panel_note("Choose a revealed board square for this spell."); MessageBeep(MB_ICONWARNING); return 1;
    }
    if(sp->target==6 && visible && !spell_square_visible(x,y,visible)) {
        panel_note("Use the GM view to select an unexplored section for Open Window."); return 1;
    }
    c->syncing=1;
    if(sp->target==2 || sp->target==5 || sp->target==6 || sp->target==7 || sp->target==11 || (sp->effect==SPELL_FLIGHT && c->flight_destination)) {
        SetDlgItemInt(w,SCCENTREX,x+1,FALSE); SetDlgItemInt(w,SCCENTREY,y+1,FALSE); c->x=x; c->y=y;
        if(sp->target==11) for(i=0;i<MAX_PICE;i++) {
            PICE *d=&Pice[i]; int dx=d->pos==East?1:d->pos==West?-1:0,dy=d->pos==North?1:d->pos==South?-1:0;
            if(d->type==DOOR && !game_fog()[i] && ((x==d->x && y==d->y) || (x==d->x+dx && y==d->y+dy))) { SendDlgItemMessage(w,SCDIRECTION,CB_SETCURSEL,d->pos,0); break; }
        }
        if(sp->target==7) {
            int p=floor_piece(x,y),direction=-1;
            if(p>=0) {
                if(y==Pice[p].y+Pice[p].h-1) direction=North;
                else if(y==Pice[p].y) direction=South;
                else if(x==Pice[p].x+Pice[p].w-1) direction=East;
                else if(x==Pice[p].x) direction=West;
            }
            if(direction>=0) SendDlgItemMessage(w,SCDIRECTION,CB_SETCURSEL,direction,0);
        }
        if(sp->effect==SPELL_FLIGHT) c->flight_destination=0;
    } else if(sp->target!=8 && sp->effect!=SPELL_MANUAL) {
        if(sp->target==9) id=spell_corpse_at(x,y,visible);
        else { id=hero_at(x,y,visible); if(id<0) { id=monster_at(x,y); if(id>=0) id+=HERO_LIMIT; } }
        n=(int)SendDlgItemMessage(w,SCTARGETS,LB_GETCOUNT,0,0);
        for(i=0;i<n;i++) if((int)SendDlgItemMessage(w,SCTARGETS,LB_GETITEMDATA,i,0)==id) {
            select=sp->target==10?!SendDlgItemMessage(w,SCTARGETS,LB_GETSEL,i,0):1;
            if(sp->target!=10) SendDlgItemMessage(w,SCTARGETS,LB_SETSEL,FALSE,-1);
            SendDlgItemMessage(w,SCTARGETS,LB_SETSEL,select,i); found=1; break;
        }
        if(!found) { c->syncing=0; panel_note("Click an eligible model or corpse. See the casting window for restrictions."); MessageBeep(MB_ICONWARNING); return 1; }
        if(sp->effect==SPELL_FLIGHT) {
            c->flight_destination=1; c->syncing=0; cast_selection_changed(w,c);
            panel_note("Flight target selected. Now click an empty destination square."); game_redraw(); return 1;
        }
    }
    c->syncing=0; cast_selection_changed(w,c); spell_map_selection(w,c);
    spell_map_resume(); game_redraw(); return 1;
}
static void spell_map_hover(int x,int y)
{
    SPELL_CAST *c=spell_map_cast();
    if(!c || c->committed || !spell_map_picking || c->spell<0 || pack_current()->spells[c->spell].target!=2 ||
       x<0 || y<0 || x>=Xsize || y>=Ysize || (c->x==x && c->y==y)) return;
    c->syncing=1; c->x=x; c->y=y;
    SetDlgItemInt(spell_map_dialog,SCCENTREX,x+1,FALSE); SetDlgItemInt(spell_map_dialog,SCCENTREY,y+1,FALSE);
    c->syncing=0; cast_selection_changed(spell_map_dialog,c); spell_map_selection(spell_map_dialog,c); game_redraw();
}
static void magic_effect_text(int side,int index,char *text,size_t cap)
{
    MAGIC_MODEL *m=magic_model(side,index); size_t len;
    text[0]=0;
    if(m->armour) snprintf(text,cap,"Armour +1 (max 12): until exploration");
    if(m->strength) { len=strlen(text); snprintf(text+len,cap-len,"%sStrength of Life: +1 S / WS until exploration",len?"\n":""); }
    if(m->cloak) { len=strlen(text); snprintf(text+len,cap-len,"%sCloak: +1 T (+2 vs Undead/Daemons) until exploration",len?"\n":""); }
    if(m->blind) { len=strlen(text); snprintf(text+len,cap-len,"%sBlinded: -1 to hit until exploration",len?"\n":""); }
    if(m->regen) { len=strlen(text); snprintf(text+len,cap-len,"%sRegeneration: +1 W at GM phase; ends combat %d",len?"\n":"",m->regen); }
    if(m->fear) { len=strlen(text); snprintf(text+len,cap-len,"%sFear immunity until exploration",len?"\n":""); }
    if(m->sleep) { len=strlen(text); snprintf(text+len,cap-len,"%sAsleep: adjacent comrade must spend full turn",len?"\n":""); }
    if(m->courage) { len=strlen(text); snprintf(text+len,cap-len,"%sCourage Br12: until exploration",len?"\n":""); }
    if(m->hand) { len=strlen(text); snprintf(text+len,cap-len,"%sFlaming Hand%s: until exploration",len?"\n":"",magic_hand(m)?"":" (next combat turn)"); }
    if(m->swift) { len=strlen(text); snprintf(text+len,cap-len,"%sSwift Wind: until Turn %d",len?"\n":"",magic.turn+2); }
    if(side && magic_stopped(index)) { int p=floor_piece(monsters.tokens[index].x,monsters.tokens[index].y);
        len=strlen(text); snprintf(text+len,cap-len,"%sStill Air: until Turn %d",len?"\n":"",magic.still_until[p]+1); }
}
static const char *magic_corpse_status(const MAGIC_MODEL *m)
{
    int i,j; const CHARACTER_PACK *p=pack_current();
    if(m->soul_lost) return "Soul lost";
    if(!magic.combat_active && m->death_combat && (m->death_combat==magic.last_combat || (m->death_combat<0 && m->death_combat==-magic.trap_event))) {
        for(i=0;i<hero_count;i++) for(j=0;j<p->spell_count;j++)
            if(p->spells[j].effect==SPELL_RESTORE && !cast_reason(0,i,j)) return "Restore Life window: check caster's actions";
    }
    if(magic.turn<m->death_turn) return "Resurrect next turn";
    if(magic.turn>m->death_turn) return "Resurrection expired";
    if(!battle_floor(m->corpse_x,m->corpse_y) || battle_occupied(m->corpse_x,m->corpse_y)) return "Resurrect now: square blocked";
    return "Resurrect now";
}
static void spell_outline(HDC dc,int x,int y,int size,COLORREF color,int corpse)
{
    HPEN pen=CreatePen(PS_SOLID,2,color); HGDIOBJ oldpen,oldbrush;
    if(!pen) return;
    oldpen=SelectObject(dc,pen); oldbrush=SelectObject(dc,GetStockObject(HOLLOW_BRUSH));
    Rectangle(dc,x+2,y+2,x+size-2,y+size-2);
    if(corpse) { MoveToEx(dc,x+size/4,y+size/4,NULL); LineTo(dc,x+3*size/4,y+3*size/4);
        MoveToEx(dc,x+3*size/4,y+size/4,NULL); LineTo(dc,x+size/4,y+3*size/4); }
    SelectObject(dc,oldbrush); SelectObject(dc,oldpen); DeleteObject(pen);
}
static void spell_map_draw(HDC dc,const unsigned char *visible,int size,int sx,int sy)
{
    SPELL_CAST *c=spell_map_cast(); int i,id,x,y,j,p,label_count=0,lines,attempt,moved;
    RECT r,overlap,labels[SPELL_TARGET_LIMIT]; char text[512]; MAGIC_MODEL *m; const char *line;
    SetBkMode(dc,OPAQUE); SetBkColor(dc,RGB(24,30,35));
    if(c && c->spell>=0 && pack_current()->spells[c->spell].target==2) {
        const PACK_SPELL *sp=&pack_current()->spells[c->spell];
        COLORREF color=!c->committed && cast_targets_reason(c)?RGB(230,85,65):RGB(255,180,40);
        for(y=c->y;y<c->y+sp->template_height;y++) for(x=c->x;x<c->x+sp->template_width;x++)
            if(spell_square_visible(x,y,visible)) spell_outline(dc,(x+1)*size-sx,(Ysize-y)*size-sy,size,color,0);
    }
    if(c && c->spell>=0) {
        const PACK_SPELL *sp=&pack_current()->spells[c->spell];
        if(sp->target==5 || sp->target==6) {
            p=floor_piece(c->x,c->y);
            if(p>=0 && (!visible || fog_visible(visible,p)))
                for(y=Pice[p].y;y<Pice[p].y+Pice[p].h;y++) for(x=Pice[p].x;x<Pice[p].x+Pice[p].w;x++)
                    spell_outline(dc,(x+1)*size-sx,(Ysize-y)*size-sy,size,RGB(255,180,40),0);
        } else if(sp->target==7 || sp->effect==SPELL_FLIGHT) {
            x=c->resolved && sp->effect==SPELL_FLIGHT?c->dest_x:c->x;
            y=c->resolved && sp->effect==SPELL_FLIGHT?c->dest_y:c->y;
            if(spell_square_visible(x,y,visible)) spell_outline(dc,(x+1)*size-sx,(Ysize-y)*size-sy,size,RGB(255,180,40),0);
        }
    }
    for(i=0;i<hero_count+monsters.count;i++) {
        id=i<hero_count?i:HERO_LIMIT+i-hero_count; m=magic_model(id>=HERO_LIMIT,id>=HERO_LIMIT?id-HERO_LIMIT:id);
        x=spell_target_x(id); y=spell_target_y(id);
        if(id<HERO_LIMIT?HERO_DEAD(&heroes[id]):monsters.tokens[id-HERO_LIMIT].wounds<=0) {
            if(!m->death_turn || !spell_square_visible(m->corpse_x,m->corpse_y,visible)) continue;
            x=(m->corpse_x+1)*size-sx; y=(Ysize-m->corpse_y)*size-sy;
            SetTextColor(dc,m->soul_lost?RGB(245,100,100):magic.turn==m->death_turn?RGB(100,240,150):magic.turn<m->death_turn?RGB(255,200,90):RGB(165,175,185));
            spell_outline(dc,x,y,size,GetTextColor(dc),1);
            snprintf(text,sizeof(text),"%s\n%s",spell_target_name(id),magic_corpse_status(m));
        } else {
            if(!spell_square_visible(x,y,visible)) continue;
            magic_effect_text(id>=HERO_LIMIT,id>=HERO_LIMIT?id-HERO_LIMIT:id,text,sizeof(text));
            x=(x+1)*size-sx; y=(Ysize-y)*size-sy;
            if(text[0]) { SetTextColor(dc,RGB(120,210,255)); spell_outline(dc,x,y,size,GetTextColor(dc),0); }
            if(c) for(j=0;j<c->count;j++) if(c->targets[j]==id) spell_outline(dc,x,y,size,RGB(255,180,40),0);
        }
        if(size>=24 && text[0]) {
            lines=1; for(line=text;*line;line++) if(*line=='\n') lines++;
            r.left=x+size+3; r.top=y; r.right=r.left+240; r.bottom=y+16*lines;
            /* Keep nearby spell/corpse callouts readable rather than painting
               multiple names and expiry lines over one another. */
            for(attempt=0;attempt<label_count;attempt++) {
                moved=0;
                for(j=0;j<label_count;j++) if(IntersectRect(&overlap,&r,&labels[j])) {
                    r.top=labels[j].bottom+3; r.bottom=r.top+16*lines; moved=1; break;
                }
                if(!moved) break;
            }
            if(r.top!=y) {
                HPEN pen=CreatePen(PS_DOT,1,GetTextColor(dc)); HGDIOBJ previous;
                if(pen) { previous=SelectObject(dc,pen); MoveToEx(dc,x+size,y+size/2,NULL); LineTo(dc,r.left,r.top+6);
                    SelectObject(dc,previous); DeleteObject(pen); }
            }
            labels[label_count++]=r;
            DrawTextA(dc,text,-1,&r,DT_LEFT|DT_NOPREFIX);
        }
    }
    /* Still Air is attached to a section, including when it contains no models. */
    for(p=0;p<MAX_PICE && p<ENCOUNTER_LIMIT;p++) if(magic.still_until[p]>magic.turn && (!visible || fog_visible(visible,p))) {
        r.left=(Pice[p].x+1)*size-sx; r.top=(Ysize-Pice[p].y-Pice[p].h+1)*size-sy;
        r.right=r.left+Pice[p].w*size; r.bottom=r.top+18;
        snprintf(text,sizeof(text),"Still Air: until Turn %d",magic.still_until[p]+1); SetTextColor(dc,RGB(120,210,255));
        DrawTextA(dc,text,-1,&r,DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
    }
    r.left=8; r.top=8; r.right=630; r.bottom=26; SetTextColor(dc,RGB(220,235,245));
    DrawTextA(dc,c?"SPELL TARGETING: click map | gold = targets | red = invalid | Esc returns to casting":
        "Magic: blue = active effect | corpses: green = resurrect now, amber = next turn, grey = expired, red = soul lost",
        -1,&r,DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
}
