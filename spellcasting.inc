/* Pack-defined spellcasting: preview is inert; Apply commits effects and resources. */
#define SPELL_TARGET_LIMIT (HERO_LIMIT+MONSTER_LIMIT)
static CASTER_STATE *caster_state(int side,int index)
{
    if(side==0 && index>=0 && index<hero_count) return &heroes[index].caster;
    if(side==1 && index>=0 && index<monsters.count) return &monsters.tokens[index].caster;
    return NULL;
}
static int caster_has_book(const CASTER_STATE *s)
{
    int i; if(!s) return 0; for(i=0;i<SPELL_BOOK_LIMIT;i++) if(s->books[i]) return 1; return 0;
}
static const char *cast_state_reason(int side,int index,int spell,const CASTER_STATE *s)
{
    const CHARACTER_PACK *p=pack_current(); int i; static char missing[180];
    if(!p || !s || spell<0 || spell>=p->spell_count || !s->known[spell]) return "Choose a learned spell.";
    if(side?monsters.tokens[index].wounds<=0 || monsters.tokens[index].x<0:!HERO_ACTIVE(&heroes[index]) || heroes[index].x<0)
        return side?"This monster cannot act or is not placed.":HERO_DEAD(&heroes[index])?"This Wizard is dead and cannot act.":HERO_KO(&heroes[index])?"This Wizard is knocked out and cannot act.":"Place this Wizard on the board before casting.";
    if(magic_model(side,index)->sleep && !gm_override) return "This caster is asleep; an adjacent comrade must wake it.";
    if(p->spells[spell].effect==SPELL_RESTORE && (magic.combat_active || magic_model(side,index)->restore_blocked)) return "Restore Life requires combat to have ended, before this Wizard takes another action. Start an exploration turn to end combat.";
    if(s->setup_pending) return "Finish component setup in Spellbook & components first.";
    if(turn_phase && turn_phase!=(side?2:1) && !gm_override) return side?"It is the Hero phase. This monster casts in the GM phase.":"It is the GM phase. This hero casts in the Hero phase.";
    if(s->cast_used && !gm_override) return "This caster has already cast a spell this turn. Advance the turn to cast again.";
    if(p->spells[spell].stationary && (side?monsters.tokens[index].moved:heroes[index].moved) && !gm_override)
        return "This spell requires a stationary caster; the caster has already moved.";
    for(i=0;i<p->component_count;i++) if(s->components[i]<p->spells[spell].components[i]) { snprintf(missing,sizeof(missing),"Requires %u %s; you carry %u.",p->spells[spell].components[i],p->components[i].name,s->components[i]); return missing; }
    return NULL;
}
static const char *cast_reason(int side,int index,int spell)
{
    return cast_state_reason(side,index,spell,caster_state(side,index));
}
typedef struct { int side,index,component,tab,spell; CASTER_STATE draft; } BOOK_EDIT;
static int book_available(const CHARACTER_PACK *p,const CASTER_STATE *s,int spell)
{
    int i; for(i=0;i<p->book_count;i++) if(s->books[i] && p->books[i].spells[spell]) return 1; return 0;
}
static int book_component_available(const CHARACTER_PACK *p,const CASTER_STATE *s,int component)
{
    int i; if(s->setup_pending!=1) return 1;
    for(i=0;i<p->spell_count;i++) if(s->known[i] && p->spells[i].components[component]) return 1;
    return 0;
}
static int spell_stock_casts(const CHARACTER_PACK *p,const CASTER_STATE *s,int spell)
{
    int i,n=-1; for(i=0;i<p->component_count;i++) if(p->spells[spell].components[i]) {
        int available=s->components[i]/p->spells[spell].components[i]; if(n<0 || available<n) n=available;
    }
    return n;
}
static void spell_description(char *text,size_t cap,const CHARACTER_PACK *p,const CASTER_STATE *s,int spell)
{
    const PACK_SPELL *sp=&p->spells[spell]; int i; size_t len;
    snprintf(text,cap,"%s\r\n%s\r\nComponents:",sp->name,sp->text);
    for(i=0;i<p->component_count;i++) if(sp->components[i]) {
        len=strlen(text); snprintf(text+len,cap-len," %s %u (carried %u);",p->components[i].name,sp->components[i],s->components[i]);
    }
    len=strlen(text); snprintf(text+len,cap-len,"\r\n%s; %s. Learning price: %d GC.",sp->stationary?"No movement this turn":"Movement permitted",sp->test?"Intelligence test required":"No Intelligence test",sp->cost);
}
static void report_column(HWND list,int column,const char *title,int width)
{
    LVCOLUMNA c; memset(&c,0,sizeof(c)); c.mask=LVCF_TEXT|LVCF_WIDTH; c.pszText=(char*)title; c.cx=width;
    SendMessageA(list,LVM_INSERTCOLUMNA,column,(LPARAM)&c);
}
static int report_row(HWND list,int row,int id,const char *text)
{
    LVITEMA item; memset(&item,0,sizeof(item)); item.mask=LVIF_TEXT|LVIF_PARAM; item.iItem=row; item.pszText=(char*)text; item.lParam=id;
    return (int)SendMessageA(list,LVM_INSERTITEMA,0,(LPARAM)&item);
}
static void report_cell(HWND list,int row,int column,const char *text)
{
    LVITEMA item; memset(&item,0,sizeof(item)); item.iSubItem=column; item.pszText=(char*)text;
    SendMessageA(list,LVM_SETITEMTEXTA,row,(LPARAM)&item);
}
static int report_selected(HWND list)
{
    int n=ListView_GetNextItem(list,-1,LVNI_SELECTED); LVITEMA item;
    if(n<0) return -1; memset(&item,0,sizeof(item)); item.iItem=n; item.mask=LVIF_PARAM;
    if(!SendMessageA(list,LVM_GETITEMA,0,(LPARAM)&item)) return -1; return (int)item.lParam;
}
static void book_info(HWND w,BOOK_EDIT *e)
{
    const CHARACTER_PACK *p=pack_current(); int i,total=0; char text[400],detail[4096];
    for(i=0;i<p->component_count;i++) total+=e->draft.components[i];
    if(e->draft.setup_pending==1) snprintf(text,sizeof(text),"Components tab: choose %d starting components; allocated %d of %d. Save changes keeps your progress. %s",e->draft.starting_allowance,total,e->draft.starting_allowance,total==e->draft.starting_allowance?"Ready to finish setup and cast.":"Casting requires the exact starting total.");
    else if(e->draft.setup_pending==2) strcpy(text,"Existing save: record the components actually remaining. No starting stock is granted.");
    else strcpy(text,"Changes remain a draft until Save changes. Component adjustments do not deduct gold.");
    SetDlgItemTextA(w,SBINFO,text);
    ShowWindow(GetDlgItem(w,SBFINISH),e->draft.setup_pending?SW_SHOW:SW_HIDE);
    SetDlgItemTextA(w,SBFINISH,e->draft.setup_pending==2?"Record remaining stock":"Finish allocation");
    EnableWindow(GetDlgItem(w,SBFINISH),e->draft.setup_pending==2 || (e->draft.setup_pending==1 && total==e->draft.starting_allowance));
    EnableWindow(GetDlgItem(w,IDOK),TRUE);
    EnableWindow(GetDlgItem(w,SBGMADJUST),!e->draft.setup_pending);
    EnableWindow(GetDlgItem(w,SBADJUSTQTY),e->tab==1 && e->component>=0);
    if(e->tab==0 && e->spell>=0) {
        const char *reason=cast_state_reason(e->side,e->index,e->spell,&e->draft); size_t len;
        spell_description(detail,sizeof(detail),p,&e->draft,e->spell); len=strlen(detail);
        snprintf(detail+len,sizeof(detail)-len,"\r\n%s\r\nAvailable casts are for this spell alone; ingredients are shared with other spells.",reason?reason:"Ready to cast.");
    }
    else if(e->tab==1 && e->component>=0) snprintf(detail,sizeof(detail),"%s: %u carried. Price: %d GC each.\r\nAdjust quantity records inventory; no payment is deducted.\r\nShared ingredients are consumed by every spell that uses them.",p->components[e->component].name,e->draft.components[e->component],p->components[e->component].price);
    else strcpy(detail,"Select an entry to read its rules. GM adjustment manages book membership and learned spells.");
    SetDlgItemTextA(w,SBDETAIL,detail);
}
static void book_lists(HWND w,BOOK_EDIT *e)
{
    const CHARACTER_PACK *p=pack_current(); HWND spells=GetDlgItem(w,SBSPELLS),stock=GetDlgItem(w,SBCOMPONENTS);
    int i,n,book=(int)SendDlgItemMessage(w,SBBOOK,CB_GETCURSEL,0,0); char value[180];
    ListView_DeleteAllItems(spells); ListView_DeleteAllItems(stock);
    for(i=0,n=0;i<p->spell_count;i++) if(book>=0 && p->books[book].spells[i]) {
        int casts=spell_stock_casts(p,&e->draft,i),j;
        report_row(spells,n,i,p->spells[i].name); report_cell(spells,n,1,e->draft.known[i]?"Yes":"No");
        if(!e->draft.known[i]) strcpy(value,"--"); else if(casts<0) strcpy(value,"Unlimited"); else sprintf(value,"%d",casts);
        report_cell(spells,n,2,value); strcpy(value,e->draft.known[i]?"Ready":"Not learned");
        if(e->draft.known[i]) for(j=0;j<p->component_count;j++) if(e->draft.components[j]<p->spells[i].components[j]) { snprintf(value,sizeof(value),"Missing %s",p->components[j].name); break; }
        if(e->draft.known[i] && (casts>0 || casts<0)) {
            const char *reason=cast_state_reason(e->side,e->index,i,&e->draft);
            if(reason) snprintf(value,sizeof(value),"%s",reason);
        }
        report_cell(spells,n,3,value);
        if(e->spell<0) e->spell=i;
        if(e->spell==i) ListView_SetItemState(spells,n,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED); n++;
    }
    for(i=0,n=0;i<p->component_count;i++) if(book_component_available(p,&e->draft,i)) {
        report_row(stock,n,i,p->components[i].name); sprintf(value,"%u",e->draft.components[i]); report_cell(stock,n,1,value);
        sprintf(value,"%d GC",p->components[i].price); report_cell(stock,n,2,value);
        if(e->component<0) e->component=i;
        if(e->component==i) ListView_SetItemState(stock,n,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED); n++;
    }
    ShowWindow(spells,e->tab==0?SW_SHOW:SW_HIDE); ShowWindow(stock,e->tab==1?SW_SHOW:SW_HIDE);
    ShowWindow(GetDlgItem(w,SBADJUSTQTY),e->tab==1?SW_SHOW:SW_HIDE); book_info(w,e);
}
static void gm_lists(HWND w,BOOK_EDIT *e)
{
    const CHARACTER_PACK *p=pack_current(); int i,n,book=(int)SendDlgItemMessage(w,SBBOOK,CB_GETCURSEL,0,0);
    SendDlgItemMessage(w,SBSPELLS,LB_RESETCONTENT,0,0);
    if(book<0) return;
    SetDlgItemTextA(w,SBToggleBook,e->draft.books[book]?"Remove selected book":"Add selected book");
    for(i=0;i<p->spell_count;i++) if(p->books[book].spells[i]) {
        n=(int)SendDlgItemMessageA(w,SBSPELLS,LB_ADDSTRING,0,(LPARAM)p->spells[i].name);
        SendDlgItemMessage(w,SBSPELLS,LB_SETITEMDATA,n,i); SendDlgItemMessage(w,SBSPELLS,LB_SETSEL,e->draft.known[i],n);
    }
    EnableWindow(GetDlgItem(w,SBSPELLS),e->draft.books[book]);
}
static INT_PTR CALLBACK gm_book_proc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
    BOOK_EDIT *e=(BOOK_EDIT*)GetWindowLongPtr(w,DWLP_USER); const CHARACTER_PACK *p=pack_current(); int i,n,id=LOWORD(wp);
    if(msg==WM_INITDIALOG) { e=(BOOK_EDIT*)lp; SetWindowLongPtr(w,DWLP_USER,(LONG_PTR)e); for(i=0;i<p->book_count;i++) SendDlgItemMessageA(w,SBBOOK,CB_ADDSTRING,0,(LPARAM)p->books[i].name); SendDlgItemMessage(w,SBBOOK,CB_SETCURSEL,0,0); gm_lists(w,e); return TRUE; }
    if(msg==WM_CLOSE) { EndDialog(w,0); return TRUE; } if(msg!=WM_COMMAND || !e) return FALSE;
    if(id==IDCANCEL) { EndDialog(w,0); return TRUE; }
    if(id==SBBOOK && HIWORD(wp)==CBN_SELCHANGE) { gm_lists(w,e); return TRUE; }
    if(id==SBSPELLS && HIWORD(wp)==LBN_SELCHANGE) {
        n=(int)SendDlgItemMessage(w,SBSPELLS,LB_GETCOUNT,0,0);
        for(i=0;i<n;i++) e->draft.known[SendDlgItemMessage(w,SBSPELLS,LB_GETITEMDATA,i,0)]=(unsigned char)(SendDlgItemMessage(w,SBSPELLS,LB_GETSEL,i,0)>0); return TRUE;
    }
    if(id==SBToggleBook) {
        n=(int)SendDlgItemMessage(w,SBBOOK,CB_GETCURSEL,0,0); if(n<0) return TRUE;
        e->draft.books[n]=!e->draft.books[n];
        for(i=0;i<p->spell_count;i++) { if(e->draft.books[n] && p->books[n].starting[i]) e->draft.known[i]=1; if(!book_available(p,&e->draft,i)) e->draft.known[i]=0; }
        if(!caster_has_book(&e->draft)) { memset(e->draft.components,0,sizeof(e->draft.components)); e->draft.cast_used=e->draft.move_locked=e->draft.starting_allowance=0; }
        gm_lists(w,e); return TRUE;
    }
    if(id==IDOK && pack_caster_valid(&e->draft,p)) { EndDialog(w,1); return TRUE; } return FALSE;
}
typedef struct { int component,value; } QUANTITY_EDIT;
static INT_PTR CALLBACK quantity_proc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
    QUANTITY_EDIT *e=(QUANTITY_EDIT*)GetWindowLongPtr(w,DWLP_USER); char info[256]; BOOL ok; int n;
    if(msg==WM_INITDIALOG) { e=(QUANTITY_EDIT*)lp; SetWindowLongPtr(w,DWLP_USER,(LONG_PTR)e); snprintf(info,sizeof(info),"%s\r\nInventory adjustment only; no gold is deducted. Enter 0 to 9999.",pack_current()->components[e->component].name); SetDlgItemTextA(w,SBINFO,info); SetDlgItemInt(w,SBQUANTITY,e->value,FALSE); SendDlgItemMessage(w,SBQUANTITY,EM_LIMITTEXT,4,0); return TRUE; }
    if(msg==WM_CLOSE) { EndDialog(w,0); return TRUE; } if(msg!=WM_COMMAND || !e) return FALSE;
    if(LOWORD(wp)==IDCANCEL) { EndDialog(w,0); return TRUE; }
    if(LOWORD(wp)==IDOK) { n=(int)GetDlgItemInt(w,SBQUANTITY,&ok,FALSE); if(!ok || n>9999 || n<0) { SetDlgItemTextA(w,SBINFO,"Enter a whole quantity from 0 to 9999."); return TRUE; } e->value=n; EndDialog(w,1); return TRUE; } return FALSE;
}
static INT_PTR CALLBACK spellbook_proc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
    BOOK_EDIT *e=(BOOK_EDIT*)GetWindowLongPtr(w,DWLP_USER); const CHARACTER_PACK *p=pack_current(); int i,id=LOWORD(wp);
    if(msg==WM_INITDIALOG) {
        TCITEMA tab; INITCOMMONCONTROLSEX init={sizeof(init),ICC_LISTVIEW_CLASSES|ICC_TAB_CLASSES}; char title[140];
        InitCommonControlsEx(&init); e=(BOOK_EDIT*)lp; SetWindowLongPtr(w,DWLP_USER,(LONG_PTR)e); e->spell=-1;
        snprintf(title,sizeof(title),"%s - Spellbook & Components",e->side?monsters.tokens[e->index].name:heroes[e->index].name); SetWindowTextA(w,title);
        memset(&tab,0,sizeof(tab)); tab.mask=TCIF_TEXT; tab.pszText="Spells"; SendDlgItemMessageA(w,SBTABS,TCM_INSERTITEMA,0,(LPARAM)&tab); tab.pszText="Components"; SendDlgItemMessageA(w,SBTABS,TCM_INSERTITEMA,1,(LPARAM)&tab);
        for(i=0;i<p->book_count;i++) SendDlgItemMessageA(w,SBBOOK,CB_ADDSTRING,0,(LPARAM)p->books[i].name);
        for(i=0;i<p->book_count && !e->draft.books[i];i++); SendDlgItemMessage(w,SBBOOK,CB_SETCURSEL,i<p->book_count?i:0,0);
        ListView_SetExtendedListViewStyle(GetDlgItem(w,SBSPELLS),LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
        ListView_SetExtendedListViewStyle(GetDlgItem(w,SBCOMPONENTS),LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
        report_column(GetDlgItem(w,SBSPELLS),0,"Spell",250); report_column(GetDlgItem(w,SBSPELLS),1,"Learned",75); report_column(GetDlgItem(w,SBSPELLS),2,"Casts",80); report_column(GetDlgItem(w,SBSPELLS),3,"Status",350);
        report_column(GetDlgItem(w,SBCOMPONENTS),0,"Component",330); report_column(GetDlgItem(w,SBCOMPONENTS),1,"Carried",100); report_column(GetDlgItem(w,SBCOMPONENTS),2,"Price each",200);
        book_lists(w,e); return TRUE;
    }
    if(msg==WM_CLOSE) { EndDialog(w,0); return TRUE; }
    if(msg==WM_NOTIFY && e) {
        NMHDR *h=(NMHDR*)lp;
        if(h->idFrom==SBTABS && h->code==TCN_SELCHANGE) { e->tab=TabCtrl_GetCurSel(GetDlgItem(w,SBTABS)); book_lists(w,e); return TRUE; }
        if((h->idFrom==SBSPELLS || h->idFrom==SBCOMPONENTS) && h->code==LVN_ITEMCHANGED) { if(h->idFrom==SBSPELLS) e->spell=report_selected(GetDlgItem(w,SBSPELLS)); else e->component=report_selected(GetDlgItem(w,SBCOMPONENTS)); book_info(w,e); return TRUE; }
    }
    if(msg!=WM_COMMAND || !e) return FALSE;
    if(id==IDCANCEL) { EndDialog(w,0); return TRUE; }
    if(id==SBBOOK && HIWORD(wp)==CBN_SELCHANGE) { e->spell=-1; book_lists(w,e); return TRUE; }
    if(id==SBGMADJUST && !e->draft.setup_pending) { BOOK_EDIT next=*e; if(DialogBoxParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGSPELLADJUST),w,gm_book_proc,(LPARAM)&next)==1) { e->draft=next.draft; book_lists(w,e); } return TRUE; }
    if(id==SBADJUSTQTY && e->component>=0) { QUANTITY_EDIT q={e->component,e->draft.components[e->component]}; if(DialogBoxParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGSPELLQUANTITY),w,quantity_proc,(LPARAM)&q)==1) { e->draft.components[q.component]=(unsigned short)q.value; book_lists(w,e); } return TRUE; }
    if(id==SBFINISH || id==IDOK) {
        int total=0; for(i=0;i<p->component_count;i++) total+=e->draft.components[i];
        if(id==SBFINISH && e->draft.setup_pending==1 && total!=e->draft.starting_allowance) { book_info(w,e); return TRUE; }
        if(!pack_caster_valid(&e->draft,p)) { SetDlgItemTextA(w,SBINFO,"The spellbook or component quantities are invalid; check the assigned book and stock."); return TRUE; }
        if(e->draft.setup_pending!=1 || total==e->draft.starting_allowance) e->draft.setup_pending=0;
        if(id==SBFINISH) { book_lists(w,e); return TRUE; } EndDialog(w,1); return TRUE;
    }
    return FALSE;
}
static void spellbook_open(HWND parent,int side,int index)
{
    BOOK_EDIT e; CASTER_STATE *s=caster_state(side,index); const CHARACTER_PACK *p=pack_current();
    if(!s || !p || !p->book_count) { message("This campaign pack has no spellbooks."); return; }
    { INITCOMMONCONTROLSEX init={sizeof(init),ICC_LISTVIEW_CLASSES|ICC_TAB_CLASSES}; InitCommonControlsEx(&init); }
    memset(&e,0,sizeof(e)); e.side=side; e.index=index; e.component=-1; e.draft=*s;
    if(DialogBoxParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGSPELLBOOK),parent,spellbook_proc,(LPARAM)&e)==1 && memcmp(s,&e.draft,sizeof(*s))) {
        remember(); *s=e.draft; active=1; panel_note("Spellbook and component stock updated. Undo restores the previous state."); game_changed();
    }
}
typedef struct {
    int side,index,spell,x,y,resolved,test_roll,fate_used,count,test_ready,fate_offer;
    int direction,effect_roll,dest_x,dest_y,move_cost;
    int map_owned,syncing,flight_destination,committed;
    int targets[SPELL_TARGET_LIMIT],damage[SPELL_TARGET_LIMIT];
    char result[8192];
} SPELL_CAST;
static void spell_template_collect(SPELL_CAST *c);
static int spell_template_matches(const SPELL_CAST *c);
static void spell_map_selection(HWND w,SPELL_CAST *c);
static void spell_keyboard_start(void);
static void spell_keyboard_stop(void);
static const char *spell_target_name(int id) { return id<HERO_LIMIT?heroes[id].name:monsters.tokens[id-HERO_LIMIT].name; }
static int spell_target_x(int id) { return id<HERO_LIMIT?heroes[id].x:monsters.tokens[id-HERO_LIMIT].x; }
static int spell_target_y(int id) { return id<HERO_LIMIT?heroes[id].y:monsters.tokens[id-HERO_LIMIT].y; }
static int light_door(const SPELL_CAST *c);
static void light_collect(SPELL_CAST *c);
static const char *cast_targets_reason(const SPELL_CAST *c)
{
    const PACK_SPELL *sp=&pack_current()->spells[c->spell];
    int i,j,id,self=c->side?HERO_LIMIT+c->index:c->index,x=spell_target_x(self),y=spell_target_y(self);
    int reach=c->side?monsters.tokens[c->index].melee.reach:heroes[c->index].melee.reach;
    if(sp->effect==SPELL_MANUAL) return NULL;
    if(sp->target==7) return magic_wall_reason(c->side,c->index,c->x,c->y,c->direction);
    if(sp->target==5 || sp->target==6) {
        int p=floor_piece(c->x,c->y),xx,yy;
        if(p<0 || p>=ENCOUNTER_LIMIT) return "Choose a board square inside the required dungeon section.";
        if(sp->target==6) return game_fog()[p]?"Open Window requires an unexplored section.":NULL;
        for(yy=Pice[p].y;yy<Pice[p].y+Pice[p].h;yy++) for(xx=Pice[p].x;xx<Pice[p].x+Pice[p].w;xx++)
            if(ranged_los(x,y,xx,yy)) return NULL;
        return "The selected section is outside the caster's line of sight.";
    }
    if(sp->effect==SPELL_LEARNING && light_door(c)<0) return "Click a closed door in line of sight. Use its inside square and orientation if entering coordinates.";
    if(sp->effect==SPELL_ESCAPE && (c->side || !magic.combat_active)) return "Escape requires an active encounter and a Hero caster.";
    if(sp->target==8 || sp->target==11) return NULL;
    if(c->count<0 || (sp->target!=2 && c->count<1) || c->count>SPELL_TARGET_LIMIT || (sp->target!=2 && sp->target!=10 && c->count!=1)) return "Select the required target(s). This spell needs exactly one model.";
    if(sp->target==10) {
        if(c->count>6 || (c->effect_roll && c->count>(c->effect_roll+1)/2)) return "Swift Wind: select no more than half the rolled D12, rounded up, including the caster.";
        for(i=0;i<c->count && c->targets[i]!=self;i++); if(i==c->count) return "Swift Wind must include the caster among the selected characters.";
    }
    if(sp->target==2 && (c->x<0 || c->y<0 || c->x>=Xsize || c->y>=Ysize || abs(x-c->x)+abs(y-c->y)>sp->range || !ranged_los(x,y,c->x,c->y)))
        return "The template anchor must be on visible floor within range and line of sight.";
    if(sp->target==2 && (c->x+sp->template_width>Xsize || c->y+sp->template_height>Ysize)) return "The whole template must fit inside the board.";
    if(sp->target==2 && !spell_template_matches(c)) return "Template targets changed. Preview again to include every covered figure.";
    for(i=0;i<c->count;i++) {
        MAGIC_MODEL *m; int dead,tx,ty;
        id=c->targets[i];
        if(id<0 || (id<HERO_LIMIT?id>=hero_count:id-HERO_LIMIT>=monsters.count)) return "Invalid target.";
        m=magic_model(id>=HERO_LIMIT,id>=HERO_LIMIT?id-HERO_LIMIT:id);
        dead=id<HERO_LIMIT?HERO_DEAD(&heroes[id]):monsters.tokens[id-HERO_LIMIT].wounds<=0;
        for(j=0;j<i;j++) if(c->targets[j]==id) return "A model cannot be targeted twice by the same spell.";
        if(sp->target==9) {
            if(!dead || m->soul_lost || !m->death_turn) return "Choose a recorded corpse with its soul intact.";
            if(sp->effect==SPELL_RESTORE) {
                if(id>=HERO_LIMIT || !m->death_combat ||
                    (m->death_combat>0?m->death_combat!=magic.last_combat:m->death_combat!=-magic.trap_event)) return "Restore Life requires a Hero who died in the combat just ended or the most recent fatal trap incident.";
            } else if(magic.turn!=m->death_turn) return "Resurrection requires a death in the immediately previous turn.";
            if(!battle_floor(m->corpse_x,m->corpse_y) || battle_occupied(m->corpse_x,m->corpse_y)) return "The corpse square must be visible and unoccupied before resurrection.";
            continue;
        }
        tx=spell_target_x(id); ty=spell_target_y(id);
        if(tx<0 || dead) return "Targets must be living models placed on the board.";
        if(sp->target==1 && sp->effect!=SPELL_REGEN && !gm_override && (abs(x-tx)+abs(y-ty)>sp->range || !ranged_los(x,y,tx,ty))) return "The target is outside range or line of sight. GM override permits adjudicated partial visibility.";
        if(sp->effect==SPELL_REGEN && id>=HERO_LIMIT) return "Regeneration affects Heroes only.";
        if(sp->effect==SPELL_BANISH && !magic_traits(id>=HERO_LIMIT,id>=HERO_LIMIT?id-HERO_LIMIT:id)) return "Dagger of Banishment requires an Undead, Lesser Daemon or Greater Daemon profile.";
        if(sp->effect==SPELL_FLIGHT && m->fear) return "Banish Fear protects this model from Flight.";
        if(sp->effect==SPELL_FLIGHT && !gm_override && id>=HERO_LIMIT && magic_stopped(id-HERO_LIMIT)) return "Still Air prevents this monster from making a forced run.";
        if(sp->effect==SPELL_FLIGHT && id<HERO_LIMIT && !HERO_ACTIVE(&heroes[id])) return "A knocked-out model cannot run.";
        if(sp->effect==SPELL_FLIGHT && (id<HERO_LIMIT?heroes[id].moved || heroes[id].caster.move_locked:monsters.tokens[id-HERO_LIMIT].moved || monsters.tokens[id-HERO_LIMIT].caster.move_locked)) return "Flight requires a model which has not moved or been locked against movement this turn.";
        if(sp->effect==SPELL_FLIGHT && (!battle_floor(c->x,c->y) || battle_occupied(c->x,c->y))) return "Choose an unoccupied visible destination square for the forced run.";
        if(sp->target==3 || sp->target==4) {
            if(id!=self && !battle_reach(x,y,tx,ty,reach)) return "The target must be the caster or in the caster's death zone.";
            if(sp->target==3) for(j=0;j<hero_count+monsters.count;j++) {
                int other=j<hero_count?j:HERO_LIMIT+j-hero_count;
                if(sp->effect==SPELL_LIFE && (other>=HERO_LIMIT)==c->side) continue;
                if(other==id || other==self || spell_target_x(other)<0 || (other<HERO_LIMIT?HERO_DEAD(&heroes[other]):monsters.tokens[other-HERO_LIMIT].wounds<=0)) continue;
                if(battle_reach(x,y,spell_target_x(other),spell_target_y(other),reach)) return "Another model is in the caster's death zone; healing is blocked.";
            }
        }
    }
    return NULL;
}
static int spell_damage_roll(char *text,size_t capacity,int dice,int toughness,int *wounds)
{
    HCRYPTPROV provider; int left=dice,count=0,roll; char part[8];
    text[0]=0;
    if(!CryptAcquireContext(&provider,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT)) return 0;
    while(left && count++<MELEE_ROLL_LIMIT) {
        if(!melee_d12(provider,&roll)) break;
        sprintf(part,"%s%d",text[0]?" ":"",roll);
        if(strlen(text)+strlen(part)>=capacity) break;
        strcat(text,part); if(roll!=12) left--;
    }
    CryptReleaseContext(provider,0);
    return !left && melee_damage(text,dice,toughness,wounds);
}
#include "light-spells.inc"
static int cast_prepare(SPELL_CAST *c,int use_fate)
{
    const PACK_SPELL *sp; const char *reason=cast_reason(c->side,c->index,c->spell); HCRYPTPROV provider;
    int i,dice,intelligence,fate; char rolls[2048],line[2200]; size_t used;
    c->resolved=0; memset(c->damage,0,sizeof(c->damage)); c->result[0]=0; c->fate_used=0; if(!c->test_ready) c->test_roll=0;
    if(reason) { snprintf(c->result,sizeof(c->result),"%s",reason); return 0; }
    if(pack_current()->spells[c->spell].target==2) spell_template_collect(c);
    if((reason=cast_targets_reason(c))!=NULL) { snprintf(c->result,sizeof(c->result),"%s",reason); return 0; }
    sp=&pack_current()->spells[c->spell]; light_collect(c); dice=sp->dice;
    if((sp->effect==SPELL_SWIFT || sp->effect==SPELL_FLIGHT) && !c->effect_roll && !magic_die(&c->effect_roll)) return 0;
    if(sp->effect==SPELL_SWIFT && c->count>(c->effect_roll+1)/2) {
        snprintf(c->result,sizeof(c->result),"Swift Wind D12: %d. Select at most %d characters including the caster, then preview again. This roll is retained.",c->effect_roll,(c->effect_roll+1)/2); return 0;
    }
    if(sp->effect==SPELL_FLIGHT) {
        int id=c->targets[0],side=id>=HERO_LIMIT,index=side?id-HERO_LIMIT:id;
        c->dest_x=c->x; c->dest_y=c->y;
        forced_move_budget=(side?monsters.tokens[index].stats[4]:heroes[index].stats[4])+(c->effect_roll==1?0:c->effect_roll);
        force_explore_path=1;
        i=battle_destination_cost(side,index,&c->dest_x,&c->dest_y,&c->move_cost);
        forced_move_budget=force_explore_path=0;
        if(!i) { strcpy(c->result,"No legal forced-run route to that destination. The run roll is retained; choose a different square."); return 0; }
    }
    if(sp->test) {
        if(!c->test_ready) {
            if(!CryptAcquireContext(&provider,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT)) return 0;
            i=melee_d12(provider,&c->test_roll); CryptReleaseContext(provider,0); if(!i) return 0;
        }
        intelligence=c->side?monsters.tokens[c->index].stats[6]:heroes[c->index].stats[6];
        fate=c->side?0:heroes[c->index].fate;
        c->fate_used=c->test_roll>intelligence && use_fate && fate>0;
        if(c->test_roll>intelligence && !c->fate_used) dice=sp->failed_dice;
        snprintf(c->result,sizeof(c->result),"Intelligence D12: %d versus %d: %s%s.\r\n",c->test_roll,intelligence,
            c->test_roll<=intelligence || c->fate_used?"success":"failure",c->fate_used?" (1 Fate will be spent)":"");
    }
    if(sp->effect>=SPELL_LIFE && sp->effect!=SPELL_LIFE && sp->effect!=SPELL_RESTORE) return light_preview(c,use_fate);
    if(sp->effect==SPELL_MANUAL) {
        used=strlen(c->result); snprintf(c->result+used,sizeof(c->result)-used,"GM resolution required: %s\r\nApply records casting and consumes components. It does not apply this effect automatically.",sp->text);
    } else if(sp->effect>=SPELL_ARMOUR && sp->effect!=SPELL_LIFE) {
        int success=!sp->test || c->test_roll<=intelligence || c->fate_used;
        used=strlen(c->result);
        snprintf(c->result+used,sizeof(c->result)-used,"%s\r\n",sp->effect==SPELL_ARMOUR?"Target gains +1 Toughness, capped at 12, until the next exploration turn (does not stack).":
            sp->effect==SPELL_COURAGE?"Target's effective Bravery becomes 12 until the next exploration turn.":
            sp->effect==SPELL_HAND?"Flaming Hand: following combat turns use one D12 direct Wounds on a successful melee hit, until exploration.":
            sp->effect==SPELL_SPY?"Privately inspect the selected unexplored section. It stays hidden, no monsters spawn, and entry gains +3 surprise.":
            sp->effect==SPELL_DOOR?"Create a permanent closed door at the chosen wall. Opening toward unexplored space makes the rock test.":
            sp->effect==SPELL_STILL?"Monsters in this section cannot move or attack until the next full turn.":
            (sp->effect==SPELL_RESURRECT || sp->effect==SPELL_RESTORE)?(success?"Return the dead character to its corpse square with full Wounds.":(sp->effect==SPELL_RESTORE?"Failed resurrection: components are consumed; the soul is intact.":"Failed resurrection: the soul is lost permanently. Components are still consumed.")):"Movement spell ready.");
        used=strlen(c->result);
        if(sp->effect==SPELL_SWIFT) snprintf(c->result+used,sizeof(c->result)-used,"D12 %d: up to %d characters. Selected models have %s this turn; movement already spent still counts.",c->effect_roll,(c->effect_roll+1)/2,magic.exploration?"18 squares of movement":"double Speed");
        if(sp->effect==SPELL_FLIGHT) snprintf(c->result+used,sizeof(c->result)-used,"Run D12 %d: %s will move %d squares to %d, %d. Its normal attack and movement are used. Walls, figures and entry into enemy death zones stop the run.",c->effect_roll,spell_target_name(c->targets[0]),c->move_cost,c->dest_x+1,c->dest_y+1);
    } else for(i=0;i<c->count;i++) {
        int id=c->targets[i],t=id<HERO_LIMIT?magic_attack_toughness(heroes[id].stats[3],&heroes[id].magic,magic_traits(c->side,c->index)):magic_attack_toughness(monsters.tokens[id-HERO_LIMIT].stats[3],&monsters.tokens[id-HERO_LIMIT].magic,magic_traits(c->side,c->index)); c->damage[i]=0;
        if((sp->effect==SPELL_DAMAGE || sp->effect==SPELL_BANISH) && dice) {
            rolls[0]=0; if(!spell_damage_roll(rolls,sizeof(rolls),dice,t,&c->damage[i])) { strcpy(c->result,"Dice generation failed. No changes applied."); return 0; }
            snprintf(line,sizeof(line),"%s: %d Wound(s); D12: %s\r\n",spell_target_name(id),c->damage[i],rolls);
        } else snprintf(line,sizeof(line),"%s: %s\r\n",spell_target_name(id),(sp->effect==SPELL_HEAL || sp->effect==SPELL_LIFE)?"restore all lost Wounds":"no damage after failed test");
        used=strlen(c->result); if(used+strlen(line)<sizeof(c->result)) strcat(c->result,line);
    }
    c->fate_offer=0; c->resolved=1; return 1;
}
static void spell_result_append(SPELL_CAST *c,const char *format,...)
{
    size_t len=strlen(c->result); va_list args;
    if(len>=sizeof(c->result)-1) return;
    va_start(args,format); vsnprintf(c->result+len,sizeof(c->result)-len,format,args); va_end(args);
}
static void spell_applied_result(SPELL_CAST *c,const int *before)
{
    const CHARACTER_PACK *p=pack_current(); const PACK_SPELL *sp=&p->spells[c->spell];
    const CASTER_STATE *s=caster_state(c->side,c->index); int i,id,side,index,wounds; char rolls[8192];
    memcpy(rolls,c->result,sizeof(rolls)); c->result[0]=0;
    spell_result_append(c,"%s cast %s.\r\n\r\n",spell_target_name(c->side?HERO_LIMIT+c->index:c->index),sp->name);
    for(i=0;i<c->count;i++) {
        if(strlen(c->result)>sizeof(c->result)-2048) { spell_result_append(c,"...and %d more affected figures.\r\n",c->count-i); break; }
        id=c->targets[i]; side=id>=HERO_LIMIT; index=side?id-HERO_LIMIT:id;
        wounds=side?monsters.tokens[index].wounds:heroes[index].wounds;
        spell_result_append(c,"%s: ",spell_target_name(id));
        if(sp->effect==SPELL_DAMAGE || sp->effect==SPELL_BANISH) spell_result_append(c,"%d Wound(s) inflicted; %d remaining%s.\r\n",c->damage[i],wounds,
            side?(!wounds?"; killed":""):HERO_DEAD(&heroes[index])?"; dead":HERO_KO(&heroes[index])?"; knocked out":"");
        else if(sp->effect==SPELL_HEAL || sp->effect==SPELL_LIFE) spell_result_append(c,"%d Wound(s) restored; %d Wounds now.\r\n",wounds-before[i],wounds);
        else if(sp->effect==SPELL_ARMOUR) spell_result_append(c,"Toughness is now %d. Dragon Armour lasts until the next exploration turn.\r\n",
            magic_toughness(side?monsters.tokens[index].stats[3]:heroes[index].stats[3],magic_model(side,index)));
        else if(sp->effect==SPELL_COURAGE) spell_result_append(c,"Bravery is now 12 until the next exploration turn.\r\n");
        else if(sp->effect==SPELL_SWIFT) spell_result_append(c,"%d squares of movement remain. Swift Wind expires at Turn %d.\r\n",turn_move_remaining(side,index),magic.turn+2);
        else if(sp->effect==SPELL_FLIGHT) spell_result_append(c,"ran %d squares to %d, %d. Movement and normal attack are used this turn.\r\n",c->move_cost,c->dest_x+1,c->dest_y+1);
        else if(sp->effect==SPELL_RESURRECT || sp->effect==SPELL_RESTORE) spell_result_append(c,wounds?"returned to life with %d Wounds. Movement and normal attack are used this turn.\r\n":(sp->effect==SPELL_RESTORE?"resurrection failed; components spent, soul intact.\r\n":"resurrection failed; the soul is permanently lost.\r\n"),wounds);
        else if(sp->effect==SPELL_STRENGTH) spell_result_append(c,"+1 Strength and +1 WS until exploration; effective WS %d.\r\n",magic_ws(side?monsters.tokens[index].stats[0]:heroes[index].stats[0],magic_model(side,index)));
        else if(sp->effect==SPELL_CLOAK) spell_result_append(c,"Cloak active: +1 T, +2 against Undead/Daemons, until exploration.\r\n");
        else if(sp->effect==SPELL_BLIND) spell_result_append(c,"-1 to hit until exploration.\r\n");
        else if(sp->effect==SPELL_FEAR) spell_result_append(c,"Immune to fear and Flight until exploration.\r\n");
        else if(sp->effect==SPELL_REGEN) spell_result_append(c,"Recovers 1 Wound each GM phase; expires at the end of combat %d.\r\n",magic_model(side,index)->regen);
        else if(sp->effect==SPELL_SLEEP) spell_result_append(c,"Asleep until woken; use its token context menu to wake with a comrade.\r\n");
        else if(sp->effect==SPELL_ESCAPE) spell_result_append(c,"Withdrawn to Reserve without pursuit. Place at the retreat destination using Heroes & Reserve.\r\n");
        else spell_result_append(c,"effect applied.\r\n");
    }
    if((sp->effect==SPELL_DAMAGE || sp->effect==SPELL_BANISH) && !c->count) spell_result_append(c,"No figures were covered by the template; no damage was dealt.\r\n");
    if(sp->effect==SPELL_HAND) spell_result_append(c,"Flaming Hand is active from the following combat turn until the next exploration turn. Successful melee hits will inflict D12 Wounds directly.\r\n");
    if(sp->effect==SPELL_STILL) spell_result_append(c,"Still Air applied to the section at %d, %d. Its monsters cannot move or attack until Turn %d.\r\n",c->x+1,c->y+1,magic.turn+2);
    if(sp->effect==SPELL_SPY) spell_result_append(c,"Section at %d, %d inspected privately. It remains hidden; entering it grants +3 surprise.\r\n",c->x+1,c->y+1);
    if(sp->effect==SPELL_DOOR) spell_result_append(c,"A permanent closed door was created at %d, %d. Open it to discover what lies beyond.\r\n",c->x+1,c->y+1);
    if(sp->effect==SPELL_MANUAL) spell_result_append(c,"GM resolution is still required: %s\r\n",sp->text);
    spell_result_append(c,"\r\nResources spent:\r\n");
    for(i=0;i<p->component_count;i++) if(sp->components[i]) spell_result_append(c,"%u %s consumed; %u remaining.\r\n",sp->components[i],p->components[i].name,s->components[i]);
    if(c->fate_used) spell_result_append(c,"1 Fate spent; %d remaining.\r\n",heroes[c->index].fate);
    spell_result_append(c,"Spell allowance used this turn.%s\r\n",sp->stationary?" The caster cannot move this turn.":"");
    if(sp->test || sp->effect==SPELL_DAMAGE || sp->effect>=SPELL_LIFE) spell_result_append(c,"\r\nRoll details:\r\n%s",rolls);
}
static int cast_apply(SPELL_CAST *c)
{
    const CHARACTER_PACK *p=pack_current(); const PACK_SPELL *sp; CASTER_STATE *s; int i,id,old,before[SPELL_TARGET_LIMIT],total_unique=monsters.dead_count;
    if(!c->resolved || cast_reason(c->side,c->index,c->spell) || cast_targets_reason(c)) return 0;
    if(c->fate_used && (c->side || heroes[c->index].fate<1)) return 0;
    sp=&p->spells[c->spell];
    /* Refuse the whole cast before mutation if character-death bookkeeping is full. */
    for(i=0;i<c->count;i++) if((sp->effect==SPELL_DAMAGE || sp->effect==SPELL_BANISH) && c->targets[i]>=HERO_LIMIT) {
        MONSTER *m=&monsters.tokens[c->targets[i]-HERO_LIMIT];
        if(c->damage[i]>=m->wounds && m->unique && !character_dead(monster_identity(m))) total_unique++;
    }
    if(total_unique>CHARACTER_LIMIT) return 0;
    if(sp->effect==SPELL_FLIGHT) {
        int side=c->targets[0]>=HERO_LIMIT,index=side?c->targets[0]-HERO_LIMIT:c->targets[0],x=c->x,y=c->y,cost,ok;
        forced_move_budget=(side?monsters.tokens[index].stats[4]:heroes[index].stats[4])+(c->effect_roll==1?0:c->effect_roll); force_explore_path=1;
        ok=battle_destination_cost(side,index,&x,&y,&cost); forced_move_budget=force_explore_path=0;
        if(!ok || x!=c->dest_x || y!=c->dest_y || cost!=c->move_cost) return 0;
    }
    for(i=0;i<c->count;i++) before[i]=c->targets[i]<HERO_LIMIT?heroes[c->targets[i]].wounds:monsters.tokens[c->targets[i]-HERO_LIMIT].wounds;
    remember(); if(sp->effect==SPELL_DAMAGE || sp->effect==SPELL_BANISH || sp->effect==SPELL_BLIND || sp->effect==SPELL_SLEEP) magic_begin_combat(); s=caster_state(c->side,c->index);
    for(i=0;i<p->component_count;i++) s->components[i]-=sp->components[i];
    s->cast_used=1; s->move_locked=sp->stationary; s->setup_pending=0;
    if(c->fate_used) heroes[c->index].fate--;
    for(i=0;i<c->count;i++) {
        id=c->targets[i];
        if((sp->effect==SPELL_DAMAGE || sp->effect==SPELL_BANISH) && c->damage[i]) magic_model(id>=HERO_LIMIT,id>=HERO_LIMIT?id-HERO_LIMIT:id)->sleep=0;
        if((sp->effect==SPELL_DAMAGE || sp->effect==SPELL_BANISH) && id<HERO_LIMIT) {
            HERO *h=&heroes[id]; old=h->wounds;
            h->wounds=c->damage[i]>=old?0:old-c->damage[i];
            if(c->damage[i] && !h->wounds) { h->condition=(c->damage[i]>old || h->kind==4)?2:1; magic_hero_death(h); }
        } else if(sp->effect==SPELL_DAMAGE || sp->effect==SPELL_BANISH) {
            MONSTER *m=&monsters.tokens[id-HERO_LIMIT]; m->wounds=c->damage[i]>=m->wounds?0:m->wounds-c->damage[i]; if(!m->wounds) monster_kill(id-HERO_LIMIT);
        } else if((sp->effect==SPELL_HEAL || sp->effect==SPELL_LIFE) && id<HERO_LIMIT) { heroes[id].wounds=heroes[id].stats[7]; heroes[id].condition=0; }
        else if(sp->effect==SPELL_HEAL || sp->effect==SPELL_LIFE) monsters.tokens[id-HERO_LIMIT].wounds=monsters.tokens[id-HERO_LIMIT].stats[7];
    }
    if(sp->effect>=SPELL_LIFE) light_apply(c);
    else magic_model(c->side,c->index)->restore_blocked=1;
    if(sp->effect==SPELL_HAND) { MAGIC_MODEL *m=magic_model(c->side,c->index); m->hand=1; m->hand_turn=magic.turn; }
    if(sp->effect==SPELL_ARMOUR || sp->effect==SPELL_COURAGE || sp->effect==SPELL_SWIFT) for(i=0;i<c->count;i++) {
        id=c->targets[i]; { MAGIC_MODEL *m=magic_model(id>=HERO_LIMIT,id>=HERO_LIMIT?id-HERO_LIMIT:id);
            if(sp->effect==SPELL_ARMOUR) m->armour=1; else if(sp->effect==SPELL_COURAGE) m->courage=1; else m->swift=1; }
    }
    if(sp->effect==SPELL_STILL) magic.still_until[floor_piece(c->x,c->y)]=magic.turn+1;
    if(sp->effect==SPELL_SPY) magic.spied[floor_piece(c->x,c->y)]=1;
    if(sp->effect==SPELL_DOOR) {
        int slot=magic_empty_slot(),dx=c->direction==East?1:c->direction==West?-1:0,dy=c->direction==North?1:c->direction==South?-1:0;
        int p=floor_piece(c->x+dx,c->y+dy);
        init_pice(&Pice[slot],DOOR,c->x,c->y,(DIRECTION)c->direction); game_fog()[slot]=0;
        magic.doors[slot]=p<0 || !game_fog()[p]?1:2;
    }
    if(sp->effect==SPELL_FLIGHT) {
        id=c->targets[0]; if(id<HERO_LIMIT) { HERO *h=&heroes[id]; for(i=0;i<hero_route_count;i++) { h->x=hero_route[i]%Xsize; h->y=hero_route[i]/Xsize; reveal_corridors(); } h->x=c->dest_x; h->y=c->dest_y; h->moved=h->attacked=1; h->move_spent=1000000; }
        else { MONSTER *m=&monsters.tokens[id-HERO_LIMIT]; m->x=c->dest_x; m->y=c->dest_y; m->moved=m->attacked=1; m->move_spent=1000000; }
        focus_enter(id>=HERO_LIMIT,id>=HERO_LIMIT?id-HERO_LIMIT:id);
    }
    if(sp->effect==SPELL_RESURRECT || sp->effect==SPELL_RESTORE) {
        MAGIC_MODEL *m; int success=c->test_roll<=(c->side?monsters.tokens[c->index].stats[6]:heroes[c->index].stats[6]) || c->fate_used;
        id=c->targets[0]; m=magic_model(id>=HERO_LIMIT,id>=HERO_LIMIT?id-HERO_LIMIT:id);
        if(!success && sp->effect==SPELL_RESURRECT) m->soul_lost=1;
        else if(success) {
            if(id<HERO_LIMIT) { HERO *h=&heroes[id]; h->wounds=h->stats[7]; h->condition=0; h->x=m->corpse_x; h->y=m->corpse_y; h->moved=h->attacked=1; }
            else { MONSTER *v=&monsters.tokens[id-HERO_LIMIT]; v->wounds=v->stats[7]; v->x=m->corpse_x; v->y=m->corpse_y; v->moved=v->attacked=1;
                if(v->unique) for(i=0;i<monsters.dead_count;i++) if(!_stricmp(monsters.dead[i],monster_identity(v))) { memmove(&monsters.dead[i],&monsters.dead[i+1],(monsters.dead_count-i-1)*sizeof(monsters.dead[0])); monsters.dead_count--; break; }
            }
            /* A restored model cannot retain a prior life's temporary spell effects,
               run bonus or unused movement in free play. */
            memset(m,0,sizeof(*m));
            if(id<HERO_LIMIT) { heroes[id].move_spent=1000000; heroes[id].run_bonus=0; }
            else { monsters.tokens[id-HERO_LIMIT].move_spent=1000000; monsters.tokens[id-HERO_LIMIT].run_bonus=0; }
        }
    }
    {
        char note[512]; size_t len;
        spell_applied_result(c,before);
        len=strcspn(c->result,"\r\n"); if(len>=sizeof(note)) len=sizeof(note)-1;
        memcpy(note,c->result,len); note[len]=0;
        if(c->count) { const char *detail=strstr(c->result,"\r\n\r\n"); if(detail) {
            size_t more; detail+=4; more=strcspn(detail,"\r\n"); if(more>sizeof(note)-len-2) more=sizeof(note)-len-2;
            note[len++]=' '; memcpy(note+len,detail,more); note[len+more]=0;
        } }
        c->resolved=0; active=1; focus_prune(); panel_note(note); game_changed();
        return 1;
    }
}
static void spell_dialog_refresh(HWND w,SPELL_CAST *c)
{
    const CHARACTER_PACK *p=pack_current(); CASTER_STATE *state=caster_state(c->side,c->index); const PACK_SPELL *sp;
    int i,n; char text[4096],part[180],label[180]; size_t len; const char *reason;
    c->resolved=c->test_ready=c->fate_offer=0; c->test_roll=0; SendDlgItemMessage(w,SCTARGETS,LB_RESETCONTENT,0,0);
    if(c->spell<0 || c->spell>=p->spell_count) { SetDlgItemTextA(w,SCINFO,"No learned spells. Open Components to manage your spellbook."); EnableWindow(GetDlgItem(w,SCROLL),FALSE); EnableWindow(GetDlgItem(w,IDOK),FALSE); return; }
    sp=&p->spells[c->spell]; snprintf(text,sizeof(text),"%s\r\n%s\r\nComponents:",sp->name,sp->text);
    for(i=0;i<p->component_count;i++) if(sp->components[i]) {
        snprintf(part,sizeof(part)," %s %u (stock %u);",p->components[i].name,sp->components[i],state->components[i]); len=strlen(text); snprintf(text+len,sizeof(text)-len,"%s",part);
    }
    len=strlen(text); snprintf(text+len,sizeof(text)-len,"\r\n%s | One spell per turn. Weapon attacks have a separate allowance.",sp->stationary?"Cannot move this turn":"Movement permitted");
    reason=cast_reason(c->side,c->index,c->spell); if(reason) { len=strlen(text); snprintf(text+len,sizeof(text)-len,"\r\nUnavailable: %s",reason); }
    SetDlgItemTextA(w,SCINFO,text); SetDlgItemTextA(w,SCRESULT,"Click targets on the map, then preview. Apply commits the cast; Cancel costs nothing.");
    for(i=0;sp->effect!=SPELL_MANUAL && i<hero_count+monsters.count;i++) {
        int id=i<hero_count?i:HERO_LIMIT+i-hero_count;
        if(sp->target==9) {
            MAGIC_MODEL *m=magic_model(id>=HERO_LIMIT,id>=HERO_LIMIT?id-HERO_LIMIT:id);
            if(!(id<HERO_LIMIT?HERO_DEAD(&heroes[id]):monsters.tokens[id-HERO_LIMIT].wounds<=0) || !m->death_turn || m->soul_lost) continue;
        } else if(spell_target_x(id)<0 || (id<HERO_LIMIT?HERO_DEAD(&heroes[id]):monsters.tokens[id-HERO_LIMIT].wounds<=0)) continue;
        sprintf(label,"%s: %s [%d W] at %d, %d",id<HERO_LIMIT?"Hero":"Monster",spell_target_name(id),id<HERO_LIMIT?heroes[id].wounds:monsters.tokens[id-HERO_LIMIT].wounds,spell_target_x(id)+1,spell_target_y(id)+1);
        if(sp->target==9) { MAGIC_MODEL *m=magic_model(id>=HERO_LIMIT,id>=HERO_LIMIT?id-HERO_LIMIT:id); snprintf(label,sizeof(label),"Dead: %s; corpse %d, %d; %s",spell_target_name(id),m->corpse_x+1,m->corpse_y+1,magic.turn==m->death_turn?"eligible this turn":"outside resurrection window"); }
        n=(int)SendDlgItemMessageA(w,SCTARGETS,LB_ADDSTRING,0,(LPARAM)label); SendDlgItemMessage(w,SCTARGETS,LB_SETITEMDATA,n,id);
    }
    SetDlgItemTextA(w,SCTARGETLABEL,sp->effect==SPELL_MANUAL?"Targeting and effects are resolved by the GM, following the description above.":
        sp->target==11?"Click a closed door on the map to inspect visible contents beyond it.":sp->target==2?"Click the template's lower-left square on the map. All covered figures, including friends, are selected automatically.":sp->target==10?"Click Swift Wind characters to add/remove them, including the caster. Preview rolls the limit once.":
        sp->target==9?(sp->effect==SPELL_RESTORE?"Select a Hero who died in the combat just ended. Cast before the Wizard does anything else.":"Select a dead character. Resurrection is allowed only in the turn after death."):
        sp->target==5 || sp->target==6 || sp->target==7 || sp->target==11 || sp->target==8?"Use section / wall coordinates below, or no target for a self spell.":"Select one model to receive this spell.");
    EnableWindow(GetDlgItem(w,SCTARGETS),sp->effect!=SPELL_MANUAL && sp->target!=2 && sp->target!=5 && sp->target!=6 && sp->target!=7 && sp->target!=11 && sp->target!=8);
    EnableWindow(GetDlgItem(w,SCMAPPICK),sp->effect!=SPELL_MANUAL && sp->target!=8);
    ShowWindow(GetDlgItem(w,SCCENTRELABEL),(sp->target==2 || sp->target==5 || sp->target==6 || sp->target==7 || sp->target==11 || sp->effect==SPELL_FLIGHT)?SW_SHOW:SW_HIDE);
    ShowWindow(GetDlgItem(w,SCCENTREX),(sp->target==2 || sp->target==5 || sp->target==6 || sp->target==7 || sp->target==11 || sp->effect==SPELL_FLIGHT)?SW_SHOW:SW_HIDE);
    ShowWindow(GetDlgItem(w,SCCENTREY),(sp->target==2 || sp->target==5 || sp->target==6 || sp->target==7 || sp->target==11 || sp->effect==SPELL_FLIGHT)?SW_SHOW:SW_HIDE);
    EnableWindow(GetDlgItem(w,SCCENTREX),sp->target==2 || sp->target==5 || sp->target==6 || sp->target==7 || sp->target==11 || sp->effect==SPELL_FLIGHT); EnableWindow(GetDlgItem(w,SCCENTREY),IsWindowEnabled(GetDlgItem(w,SCCENTREX)));
    ShowWindow(GetDlgItem(w,SCDIRECTION),(sp->target==7 || sp->effect==SPELL_LEARNING)?SW_SHOW:SW_HIDE);
    SetDlgItemTextA(w,SCCENTRELABEL,sp->target==2?"Template lower-left square (X / Y)":sp->target==11?"Door square (X / Y) and orientation":sp->target==7?"Inside wall square (X / Y), outward direction":sp->effect==SPELL_FLIGHT?"Forced-run destination (X / Y)":"Square in selected section (X / Y)");
    EnableWindow(GetDlgItem(w,SCFATE),FALSE); SetDlgItemTextA(w,SCROLL,"Preview cast / roll");
    ShowWindow(GetDlgItem(w,SCMANUAL),sp->effect==SPELL_MANUAL?SW_SHOW:SW_HIDE); EnableWindow(GetDlgItem(w,SCMANUAL),sp->effect==SPELL_MANUAL); CheckDlgButton(w,SCMANUAL,BST_UNCHECKED);
    EnableWindow(GetDlgItem(w,SCROLL),!reason); EnableWindow(GetDlgItem(w,IDOK),FALSE);
}
static void cast_spell_list(HWND w,SPELL_CAST *c)
{
    const CHARACTER_PACK *p=pack_current(); CASTER_STATE *s=caster_state(c->side,c->index); int i,n;
    SendDlgItemMessage(w,SCSpell,CB_RESETCONTENT,0,0); c->spell=-1;
    for(i=0;i<p->spell_count;i++) if(s->known[i]) {
        char label[300]; const char *reason=cast_reason(c->side,c->index,i);
        snprintf(label,sizeof(label),"%s - %s",p->spells[i].name,reason?"Unavailable":"Ready");
        n=(int)SendDlgItemMessageA(w,SCSpell,CB_ADDSTRING,0,(LPARAM)label); SendDlgItemMessage(w,SCSpell,CB_SETITEMDATA,n,i);
        if(c->spell<0) c->spell=i;
    }
    SendDlgItemMessage(w,SCSpell,CB_SETCURSEL,0,0); spell_dialog_refresh(w,c);
}
static const char *cast_gather(HWND w,SPELL_CAST *c)
{
    const PACK_SPELL *sp; int i,n; BOOL ok; const char *reason=cast_reason(c->side,c->index,c->spell);
    if(reason) return reason; sp=&pack_current()->spells[c->spell]; c->count=0;
    if(sp->effect==SPELL_MANUAL) { if(!IsDlgButtonChecked(w,SCMANUAL)) return "Confirm that the GM will resolve the described effect."; }
    else {
        n=(int)SendDlgItemMessage(w,SCTARGETS,LB_GETCOUNT,0,0);
        for(i=0;i<n && c->count<SPELL_TARGET_LIMIT;i++) if(SendDlgItemMessage(w,SCTARGETS,LB_GETSEL,i,0)>0) c->targets[c->count++]=(int)SendDlgItemMessage(w,SCTARGETS,LB_GETITEMDATA,i,0);
    }
    if(sp->target==2 || sp->target==5 || sp->target==6 || sp->target==7 || sp->target==11 || sp->effect==SPELL_FLIGHT) {
        c->x=(int)GetDlgItemInt(w,SCCENTREX,&ok,FALSE)-1; if(!ok) return "Enter a valid board square X.";
        c->y=(int)GetDlgItemInt(w,SCCENTREY,&ok,FALSE)-1; if(!ok) return "Enter a valid board square Y.";
    }
    c->direction=(int)SendDlgItemMessage(w,SCDIRECTION,CB_GETCURSEL,0,0);
    if(sp->target==2) spell_template_collect(c);
    light_collect(c);
    return cast_targets_reason(c);
}
static void cast_dialog_status(HWND w,SPELL_CAST *c)
{
    const CHARACTER_PACK *p=pack_current(); const PACK_SPELL *sp; const char *reason=cast_gather(w,c);
    char cost[600]; int i,friendly=0; size_t len;
    if(c->spell<0 || c->spell>=p->spell_count) { SetDlgItemTextA(w,SCREASON,"Choose a learned spell."); return; }
    sp=&p->spells[c->spell]; strcpy(cost,"Spends:");
    for(i=0;i<p->component_count;i++) if(sp->components[i]) { len=strlen(cost); snprintf(cost+len,sizeof(cost)-len," %u %s;",sp->components[i],p->components[i].name); }
    if(strlen(cost)==7) strcat(cost," no components;");
    if(sp->target==2) { len=strlen(cost); snprintf(cost+len,sizeof(cost)-len," %dx%d template: %d figure(s).",sp->template_width,sp->template_height,c->count); }
    len=strlen(cost); snprintf(cost+len,sizeof(cost)-len," %s%s",sp->stationary?"No movement this turn.":"Movement permitted.",c->fate_used?" 1 Fate.":"");
    SetDlgItemTextA(w,SCCOST,cost);
    for(i=0;i<c->count;i++) if((c->targets[i]<HERO_LIMIT)==!c->side) friendly=1;
    SetDlgItemTextA(w,SCFRIENDLY,(sp->effect==SPELL_DAMAGE || sp->effect==SPELL_BANISH) && friendly?"Includes friendly figures. They will also suffer damage.":"");
    EnableWindow(GetDlgItem(w,SCROLL),!reason && !c->resolved);
    EnableWindow(GetDlgItem(w,IDOK),!reason && c->resolved);
    EnableWindow(GetDlgItem(w,SCFATE),!reason && c->fate_offer && !c->side && heroes[c->index].fate>0);
    SetDlgItemTextA(w,SCREASON,reason?reason:c->resolved?"Ready to apply. Apply spends the listed resources; Cancel discards the preview.":c->fate_offer?"Intelligence test failed. Spend Fate or continue with failure; Apply is unavailable.":"Apply is unavailable until you preview the cast.");
}
static int cast_ui_preview(HWND w,SPELL_CAST *c,int use_fate)
{
    const PACK_SPELL *sp; const char *reason=cast_gather(w,c); HCRYPTPROV provider; int ok,intelligence;
    if(reason) { SetDlgItemTextA(w,SCRESULT,reason); return 0; } sp=&pack_current()->spells[c->spell];
    if(sp->test && !c->test_ready) {
        if(!CryptAcquireContext(&provider,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT)) { SetDlgItemTextA(w,SCRESULT,"Dice generator unavailable. No resources spent."); return 0; }
        ok=melee_d12(provider,&c->test_roll); CryptReleaseContext(provider,0); if(!ok) return 0;
        c->test_ready=1; intelligence=c->side?monsters.tokens[c->index].stats[6]:heroes[c->index].stats[6];
        if(c->test_roll>intelligence) {
            c->fate_offer=1; c->resolved=0;
            snprintf(c->result,sizeof(c->result),"Intelligence D12: %d versus %d: failure.\r\n%s",c->test_roll,intelligence,!c->side && heroes[c->index].fate>0?"Choose Spend 1 Fate to succeed, or Continue with failure. No Fate or components have been spent.":"No Fate is available. Choose Continue with failure to resolve the spell.");
            SetDlgItemTextA(w,SCRESULT,c->result); SetDlgItemTextA(w,SCROLL,"Continue with failure"); cast_dialog_status(w,c); return 1;
        }
    }
    ok=cast_prepare(c,use_fate); if(c->fate_offer) SetDlgItemTextA(w,SCROLL,"Continue with failure"); SetDlgItemTextA(w,SCRESULT,c->result); cast_dialog_status(w,c); return ok;
}
static void cast_selection_changed(HWND w,SPELL_CAST *c)
{
    /* Choosing a different target must not grant another Intelligence test.
       Changing the spell resets the test through spell_dialog_refresh. */
    c->resolved=0; c->fate_used=0;
    SetDlgItemTextA(w,SCROLL,c->fate_offer?"Continue with failure":"Preview cast / roll");
    SetDlgItemTextA(w,SCRESULT,c->test_ready?
        "Selection changed. Preview again using the original Intelligence roll; Fate may still convert failure.":
        "Selection changed. Preview the cast before applying.");
    cast_dialog_status(w,c);
}
static BOOL CALLBACK spell_result_hide_input(HWND child,LPARAM unused)
{
    int id=GetDlgCtrlID(child);
    if(id!=SCRESULT && id!=IDOK) ShowWindow(child,SW_HIDE);
    return TRUE;
}
static void spell_show_result(HWND w,SPELL_CAST *c)
{
    RECT r={10,10,540,435}; char title[160];
    c->committed=1; spell_map_picking=0;
    snprintf(title,sizeof(title),"Spell cast - %s",pack_current()->spells[c->spell].name); SetWindowTextA(w,title);
    EnumChildWindows(w,spell_result_hide_input,0);
    MapDialogRect(w,&r); MoveWindow(GetDlgItem(w,SCRESULT),r.left,r.top,r.right-r.left,r.bottom-r.top,TRUE);
    SetDlgItemTextA(w,SCRESULT,c->result); SendDlgItemMessage(w,SCRESULT,EM_SETSEL,0,0);
    SendDlgItemMessage(w,SCRESULT,EM_SCROLLCARET,0,0);
    SetDlgItemTextA(w,IDOK,"Close"); EnableWindow(GetDlgItem(w,IDOK),TRUE);
    ShowWindow(w,SW_SHOW); SetFocus(GetDlgItem(w,IDOK)); game_redraw();
}
static INT_PTR CALLBACK spellcast_proc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
    SPELL_CAST *c=(SPELL_CAST*)GetWindowLongPtr(w,DWLP_USER); int id=LOWORD(wp),n,i; BOOL ok;
    if(msg==WM_INITDIALOG) {
        c=(SPELL_CAST*)lp; SetWindowLongPtr(w,DWLP_USER,(LONG_PTR)c);
        { const char *directions[]={"North","South","East","West"}; for(i=0;i<4;i++) SendDlgItemMessageA(w,SCDIRECTION,CB_ADDSTRING,0,(LPARAM)directions[i]); SendDlgItemMessage(w,SCDIRECTION,CB_SETCURSEL,0,0); }
        SetDlgItemInt(w,SCCENTREX,c->x+1,FALSE); SetDlgItemInt(w,SCCENTREY,c->y+1,FALSE);
        cast_spell_list(w,c); cast_dialog_status(w,c); return TRUE;
    }
    if(msg==WM_NCDESTROY && c && c->map_owned) {
        if(spell_map_dialog==w) { spell_map_dialog=NULL; spell_map_picking=0; spell_keyboard_stop(); }
        SetWindowLongPtr(w,DWLP_USER,0); free(c); game_redraw(); return FALSE;
    }
    if(msg==WM_CLOSE) { if(c && c->map_owned) DestroyWindow(w); else EndDialog(w,0); return TRUE; }
    if(msg!=WM_COMMAND || !c) return FALSE;
    if(c->committed) {
        if(id==IDOK || id==IDCANCEL) { if(c->map_owned) DestroyWindow(w); else EndDialog(w,1); }
        return TRUE;
    }
    if(id==IDCANCEL) { if(c->map_owned) DestroyWindow(w); else EndDialog(w,0); return TRUE; }
    if(id==SCMAPPICK) {
        spell_map_picking=1; ShowWindow(w,SW_HIDE);
        panel_note("Spell targeting: click the map. Flight: first click a model, then its destination. Escape returns to the casting window.");
        SetForegroundWindow(GlMainHwnd); game_redraw(); return TRUE;
    }
    if(id==SCSTOCK) { spellbook_open(w,c->side,c->index); cast_spell_list(w,c); cast_dialog_status(w,c); return TRUE; }
    if(id==SCSpell && HIWORD(wp)==CBN_SELCHANGE) {
        c->effect_roll=0; c->flight_destination=0; n=(int)SendDlgItemMessage(w,SCSpell,CB_GETCURSEL,0,0); c->spell=n<0?-1:(int)SendDlgItemMessage(w,SCSpell,CB_GETITEMDATA,n,0); spell_dialog_refresh(w,c); cast_dialog_status(w,c); spell_map_selection(w,c); game_redraw(); return TRUE;
    }
    if((id==SCTARGETS && HIWORD(wp)==LBN_SELCHANGE) || ((id==SCCENTREX || id==SCCENTREY) && HIWORD(wp)==EN_CHANGE) || id==SCMANUAL || (id==SCDIRECTION && HIWORD(wp)==CBN_SELCHANGE)) {
        if(c->syncing) return TRUE;
        cast_selection_changed(w,c); spell_map_selection(w,c); game_redraw(); return TRUE;
    }
    if(id==SCROLL) { cast_ui_preview(w,c,0); game_redraw(); return TRUE; }
    if(id==SCFATE && c->fate_offer && !c->side && heroes[c->index].fate>0) { cast_ui_preview(w,c,1); game_redraw(); return TRUE; }
    if(id==IDOK) {
        char *inspection=NULL; int spy=pack_current()->spells[c->spell].effect==SPELL_SPY;
        if(spy) {
            int section=floor_piece(c->x,c->y);
            if(section<0) return TRUE;
            if(Pice[section].text && Pice[section].text[0]) {
                inspection=game_room_contents(&Pice[section]);
                if(!inspection) { SetDlgItemTextA(w,SCRESULT,"Private inspection could not be prepared. No components have been spent; try again."); return TRUE; }
            }
        }
        if(cast_apply(c)) {
            if(spy) {
                spell_result_append(c,"\r\nPrivate inspection:\r\n%s",inspection?inspection:"This section has no recorded contents. It remains unexplored.");
            }
            spell_show_result(w,c);
        } else SetDlgItemTextA(w,SCRESULT,"The cast cannot be applied. Check targets, components, turn restrictions and available character-death records, then preview again.");
        free(inspection); return TRUE;
    }
    return FALSE;
}
static void spellcast_open(HWND parent,int side,int index)
{
    SPELL_CAST *c; CASTER_STATE *s=caster_state(side,index); HWND w;
    if(spell_map_dialog) { ShowWindow(spell_map_dialog,SW_SHOW); SetForegroundWindow(spell_map_dialog); return; }
    if(!caster_has_book(s)) { message("This model has no spellbook. Use Spellbook & components to configure one."); return; }
    c=(SPELL_CAST*)calloc(1,sizeof(*c)); if(!c) { message("Unable to allocate the casting window."); return; }
    c->side=side; c->index=index; c->x=side?monsters.tokens[index].x:heroes[index].x; c->y=side?monsters.tokens[index].y:heroes[index].y;
    /* Set ownership only after successful creation: initialization can fail. */
    w=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGSPELLCAST),parent,spellcast_proc,(LPARAM)c);
    if(!w) { free(c); message("Unable to open the casting window."); return; }
    c->map_owned=1; spell_map_dialog=w; move_mode=monster_move_mode=0; attack_monster=-1;
    spell_keyboard_start(); spell_map_selection(w,c); ShowWindow(w,SW_SHOW); game_redraw();
}
