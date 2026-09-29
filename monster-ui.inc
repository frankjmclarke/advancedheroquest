static void monster_list(HWND hwnd)
{
    int i; char label[180]; MONSTER *m;
    SendDlgItemMessage(hwnd,GMLIST,LB_RESETCONTENT,0,0);
    for(i=0;i<monsters.count;i++) {
        m=&monsters.tokens[i];
        sprintf(label,"%d. %s%s [%s, %d/%d W]",i+1,m->name,m->unique?" *":"",!m->wounds?"dead":m->x<0?"unplaced":"map",m->wounds,m->stats[7]);
        SendDlgItemMessageA(hwnd,GMLIST,LB_ADDSTRING,0,(LPARAM)label);
    }
    SendDlgItemMessage(hwnd,GMLIST,LB_SETCURSEL,selected_monster,0);
}
static void monster_fields(HWND hwnd)
{
    MONSTER *m=selected_monster>=0 && selected_monster<monsters.count?&monsters.tokens[selected_monster]:NULL;
    SetDlgItemTextA(hwnd,GMNAME,m?m->name:"");
    SetDlgItemInt(hwnd,GMMAX,m?m->stats[7]:1,FALSE); SetDlgItemInt(hwnd,GMWOUNDS,m?m->wounds:1,FALSE);
    CheckDlgButton(hwnd,GMUNIQUE,m && m->unique?BST_CHECKED:BST_UNCHECKED);
    EnableWindow(GetDlgItem(hwnd,GMCOMBAT),m!=NULL);
    EnableWindow(GetDlgItem(hwnd,GMRANGED),m!=NULL);
    EnableWindow(GetDlgItem(hwnd,GMAPPLY),m!=NULL); EnableWindow(GetDlgItem(hwnd,GMDAMAGE),m && m->wounds);
    EnableWindow(GetDlgItem(hwnd,GMPLACE),m && m->wounds && m->x<0); EnableWindow(GetDlgItem(hwnd,GMREMOVE),m && m->wounds);
    if(m) {
        int i,n=(int)SendDlgItemMessage(hwnd,GMROOM,CB_GETCOUNT,0,0);
        for(i=0;i<n;i++) if(SendDlgItemMessage(hwnd,GMROOM,CB_GETITEMDATA,i,0)==m->room) SendDlgItemMessage(hwnd,GMROOM,CB_SETCURSEL,i,0);
    }
}
static int monster_editor_apply(HWND hwnd,int add)
{
    MONSTER next; BOOL ok1,ok2; int room,n,i;
    if(!add && selected_monster<0) return 0;
    if(add && monsters.count==MONSTER_LIMIT) { message("This dungeon has reached its 512-monster limit."); return 0; }
    n=(int)SendDlgItemMessage(hwnd,GMROOM,CB_GETCURSEL,0,0);
    if(n<0) { message("Reveal a room before adding monsters."); return 0; }
    room=(int)SendDlgItemMessage(hwnd,GMROOM,CB_GETITEMDATA,n,0);
    if(add) { memset(&next,0,sizeof(next)); next.x=next.y=-1; next.room=room; }
    else next=monsters.tokens[selected_monster];
    GetDlgItemTextA(hwnd,GMNAME,next.name,sizeof(next.name));
    /* Canonical identity: trim, fold case and normalize hyphens/whitespace. */
    { char clean[64]; int j=0,space=0; const unsigned char *s=(unsigned char*)next.name;
      for(;*s;s++) { if(isspace(*s)||*s=='-') { space=j>0; continue; } if(space) clean[j++]=' '; clean[j++]=(char)tolower(*s); space=0; }
      clean[j]=0; strcpy(next.name,clean); }
    next.stats[7]=(int)GetDlgItemInt(hwnd,GMMAX,&ok1,FALSE);
    next.wounds=(int)GetDlgItemInt(hwnd,GMWOUNDS,&ok2,FALSE);
    next.unique=IsDlgButtonChecked(hwnd,GMUNIQUE)==BST_CHECKED;
    if(!next.unique) next.character_id[0]=0;
    if(!next.name[0] || !ok1 || !ok2 || next.stats[7]<1 || next.stats[7]>99 || next.wounds<0 || next.wounds>next.stats[7]) {
        message("Enter a name, Max Wounds from 1 to 99, and remaining Wounds from 0 to Max."); return 0;
    }
    if(next.wounds && character_dead(monster_identity(&next))) { message("That character is dead in this adventure. Use Undo to reverse an accidental killing."); return 0; }
    if(next.wounds) for(i=0;i<monsters.count;i++) {
        if((add || i!=selected_monster) && monsters.tokens[i].wounds && (next.unique || monsters.tokens[i].unique) && !_stricmp(monster_identity(&next),monster_identity(&monsters.tokens[i]))) {
            message("A unique character can have only one living token. Give it its individual encounter name."); return 0;
        }
    }
    if(!next.wounds && next.unique && !character_dead(monster_identity(&next)) && monsters.dead_count==CHARACTER_LIMIT) { message("Character death record is full."); return 0; }
    if(!add && !memcmp(&next,&monsters.tokens[selected_monster],sizeof(next))) return 1;
    remember();
    if(add) selected_monster=monsters.count++;
    monsters.tokens[selected_monster]=next;
    if(!next.wounds) monster_kill(selected_monster);
    else if(add) monster_place(selected_monster);
    active=1; game_changed(); return 1;
}
static INT_PTR CALLBACK monsters_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
    int i,n,id=LOWORD(wp),review=0,unplaced=0; char text[200];
    if(msg==WM_INITDIALOG) {
        SendDlgItemMessage(hwnd,GMNAME,EM_LIMITTEXT,63,0);
        for(i=0;i<MAX_PICE && i<ENCOUNTER_LIMIT;i++) if(monsters.seen[i]) {
            sprintf(text,"Piece %d%s%.100s",i+1,Pice[i].text && Pice[i].text[0]?" - ":"",Pice[i].text && Pice[i].text[0]?(char*)Pice[i].text[0]:"");
            text[100]=0; if(monsters.seen[i]==2) strcat(text," [review]");
            n=(int)SendDlgItemMessageA(hwnd,GMROOM,CB_ADDSTRING,0,(LPARAM)text);
            SendDlgItemMessage(hwnd,GMROOM,CB_SETITEMDATA,n,i);
        }
        SendDlgItemMessage(hwnd,GMROOM,CB_SETCURSEL,0,0);
        monster_list(hwnd); monster_fields(hwnd);
    } else if(msg==WM_CLOSE) { EndDialog(hwnd,0); return TRUE; }
    else if(msg!=WM_COMMAND) return FALSE;
    else if(id==IDOK || id==IDCANCEL) { EndDialog(hwnd,0); return TRUE; }
    else if(id==GMLIST && HIWORD(wp)==LBN_SELCHANGE) {
        selected_monster=(int)SendDlgItemMessage(hwnd,GMLIST,LB_GETCURSEL,0,0); monster_fields(hwnd); game_redraw();
    } else if(id==GMADD || id==GMAPPLY) {
        if(monster_editor_apply(hwnd,id==GMADD)) { monster_list(hwnd); monster_fields(hwnd); }
    } else if(id==GMPLACE && selected_monster>=0 && monsters.tokens[selected_monster].wounds) {
        remember(); if(!monster_place(selected_monster)) message("No free square in this room. Drag another token away, then try Place in room again.");
        active=1; game_changed(); monster_list(hwnd); monster_fields(hwnd);
    } else if(id==GMDAMAGE && selected_monster>=0) {
        monster_damage(selected_monster); monster_list(hwnd); monster_fields(hwnd);
    } else if(id==GMREMOVE && selected_monster>=0) {
        SetDlgItemInt(hwnd,GMWOUNDS,0,FALSE);
        if(monster_editor_apply(hwnd,0)) { monster_list(hwnd); monster_fields(hwnd); }
    } else if(id==GMRANGED && selected_monster>=0) {
        MONSTER next; if(!monster_editor_apply(hwnd,0)) return TRUE; next=monsters.tokens[selected_monster];
        if(edit_ranged(hwnd,&next.ranged,&next.moved,-1,next.name) && memcmp(&next,&monsters.tokens[selected_monster],sizeof(next))) {
            remember(); monsters.tokens[selected_monster]=next; active=1; game_changed();
        }
        monster_fields(hwnd);
    } else if(id==GMCOMBAT && selected_monster>=0) {
        MONSTER next;
        if(!monster_editor_apply(hwnd,0)) return TRUE;
        next=monsters.tokens[selected_monster];
        if(edit_profile(hwnd,&next.melee,next.stats,next.name,-1) && memcmp(&next,&monsters.tokens[selected_monster],sizeof(next))) {
            remember(); monsters.tokens[selected_monster]=next; active=1; game_changed();
        }
        monster_list(hwnd); monster_fields(hwnd);
    } else if(id==GMCONTENTS) {
        _UBYTE *contents;
        n=(int)SendDlgItemMessage(hwnd,GMROOM,CB_GETCURSEL,0,0);
        if(n>=0) {
            i=(int)SendDlgItemMessage(hwnd,GMROOM,CB_GETITEMDATA,n,0);
            contents=game_room_contents(&Pice[i]);
            if(contents) { MessageBoxA(hwnd,(char*)contents,"Encounter contents and reference profiles",MB_OK); free(contents); }
        }
    }
    for(i=0;i<MAX_PICE && i<ENCOUNTER_LIMIT;i++) if(monsters.seen[i]==2) review++;
    for(i=0;i<monsters.count;i++) if(monsters.tokens[i].wounds && monsters.tokens[i].x<0) unplaced++;
    sprintf(text,"%d unplaced; %d encounters need roster review.\r\nDrag to move; double-click to wound. Apply edits before closing.",unplaced,review);
    SetDlgItemTextA(hwnd,GMINFO,text); return TRUE;
}
void game_monsters(void)
{
    attack_monster=-1;
    if(!map_ready) { message("Generate or load a dungeon first."); return; }
    monsters_reveal();
    DialogBoxParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGMONSTERS),GlMainHwnd,monsters_proc,0);
}
