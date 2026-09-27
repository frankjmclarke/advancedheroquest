"""Native ranged combat, geometry, focus, movement, codec and mouse checks.
Run after build-msvc.bat in an x86 MSVC developer shell.
"""
from pathlib import Path
import ast
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
tree = ast.parse((ROOT/'tests/game-check.py').read_text())
base = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
            and any(isinstance(t, ast.Name) and t.id=='HARNESS' for t in n.targets))
monster = (ROOT/'tests/monster-check.py').read_text()
start = monster.index('static void screenshot')
screenshot = monster[start:monster.index('int main(int argc',start)]
HARNESS = base[:base.index('int main(int argc')] + screenshot + r''' 
static void room_fixture(void) {
    PICE pieces[3]; int cells[144],i; unsigned char fog[3]={1,0,0};
    memset(pieces,0,sizeof(pieces)); pieces[0].type=NORMAL_ROOM;
    pieces[0].w=pieces[0].h=12; pieces[1].type=pieces[2].type=EMPTY;
    for(i=0;i<144;i++) cells[i]=0;
    assert(map_restore(pieces,3,12,12,cells)); map_ready=active=1; ready=0;
    game_set_fog(fog,3); memset(&monsters,0,sizeof(monsters)); monsters.seen[0]=1;
    hero_count=1; hero_defaults(&heroes[0],2,1); heroes[0].x=heroes[0].y=1;
    monsters.count=1; strcpy(monsters.tokens[0].name,"orc");
    monsters.tokens[0].x=5; monsters.tokens[0].y=1;
    monsters.tokens[0].stats[0]=5; monsters.tokens[0].stats[3]=7;
    monsters.tokens[0].stats[7]=monsters.tokens[0].wounds=8;
    assert(melee_reference("orc",&monsters.tokens[0].melee));
    strcpy(session_title,"Ranged tests"); selected=selected_monster=-1; move_mode=0;
}
static void shot_draft(SHOT *s,int side) {
    memset(s,0,sizeof(*s)); s->hero=heroes[0]; s->monster=monsters.tokens[0]; s->side=side;
}
static HWND shot_dialog(SHOT *s) {
    HWND w=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGRANGED),NULL,shot_proc,(LPARAM)s); assert(w); return w;
}
static int ranged_dialogs,melee_dialogs;
static BOOL CALLBACK cancel_attack(HWND hwnd,LPARAM unused) {
    char title[80]; GetWindowTextA(hwnd,title,sizeof(title));
    if(!strcmp(title,"Ranged combat")) { ranged_dialogs++; SendMessage(hwnd,WM_COMMAND,IDCANCEL,0); }
    if(!strcmp(title,"Hand-to-hand combat")) { melee_dialogs++; SendMessage(hwnd,WM_COMMAND,IDCANCEL,0); }
    return TRUE;
}
static void CALLBACK attack_tick(HWND hwnd,UINT msg,UINT_PTR timer,DWORD time) { EnumThreadWindows(GetCurrentThreadId(),cancel_attack,0); }
static void click_square(int x,int y) {
    SendMessage(test_map_hwnd,WM_LBUTTONDOWN,MK_LBUTTON,mouse_at(x,y));
    SendMessage(test_map_hwnd,WM_LBUTTONUP,0,mouse_at(x,y));
}
static void fix_crc(unsigned char *p,size_t n) {
    unsigned long crc=0xffffffffUL; size_t i; int j;
    for(i=16;i<n;i++) { crc^=p[i]; for(j=0;j<8;j++) crc=(crc>>1)^((crc&1)?0xedb88320UL:0); }
    crc=~crc; for(j=0;j<4;j++) p[12+j]=(unsigned char)(crc>>(8*j));
}
int main(int argc,char **argv) {
    RANGED_PROFILE p; RANGED_EDIT edit; SHOT s; HWND dialog; int i,x,y,kind,range,dice,row[5];
    GAME_DATA data,*decoded; unsigned char *bytes,*again; size_t size,again_size; UINT_PTR timer;
    assert(mem_alloc(40,40,1000,4096)); heap_clear(); room_fixture();
    assert(ranged_band(3)==0 && ranged_band(4)==1 && ranged_band(12)==1 && ranged_band(13)==2 && ranged_band(24)==2 && ranged_band(25)==3 && ranged_band(36)==3 && ranged_band(37)==4);
    assert(heroes[0].ranged.kind==1 && heroes[0].ranged.range==48 && heroes[0].ranged.dice==4);
    assert(room_ranged("goblin archers",&range,&dice,row,&kind) && range==24 && dice==3 && kind==1 && row[0]==7);
    assert(!room_ranged("skaven gutter runner",&range,&dice,row,&kind));
    assert(!room_ranged("orc",&range,&dice,row,&kind));
    assert(!ranged_eligibility(0,0,0,&heroes[0].ranged,0));
    p=heroes[0].ranged; p.kind=0; assert(ranged_eligibility(0,0,0,&p,0));
    p=heroes[0].ranged; p.range=3; assert(ranged_eligibility(0,0,0,&p,0));
    assert(ranged_eligibility(0,0,0,&heroes[0].ranged,1));
    p=heroes[0].ranged; p.kind=3; assert(!ranged_eligibility(0,0,0,&p,1));
    p.kind=2; assert(ranged_eligibility(0,0,0,&p,1));
    monsters.tokens[0].x=2; assert(ranged_eligibility(0,0,0,&heroes[0].ranged,0));
    monsters.tokens[0].y=2; assert(!ranged_eligibility(0,0,0,&heroes[0].ranged,0));
    /* Friendly, hostile and defeated on-board models all block the ray. */
    monsters.tokens[0].x=5; monsters.tokens[0].y=1;
    hero_count=2; hero_defaults(&heroes[1],0,2); heroes[1].x=3; heroes[1].y=1;
    assert(!ranged_los(1,1,5,1)); heroes[1].wounds=0; assert(!ranged_los(1,1,5,1));
    heroes[1].x=heroes[1].y=-1; assert(ranged_los(1,1,5,1));
    monsters.tokens[1]=monsters.tokens[0]; monsters.tokens[1].x=3; monsters.count=2;
    assert(!ranged_los(1,1,5,1)); monsters.count=1;
    heroes[1].x=2; heroes[1].y=1; heroes[1].wounds=4;
    assert(ranged_los(1,1,4,4)); /* grazes the figure's corner only */
    heroes[1].y=2; assert(!ranged_los(1,1,4,4));
    heroes[1].x=heroes[1].y=-1;
    /* Other enemy's death zone blocks, unless already focused on someone else. */
    monsters.count=2; monsters.tokens[1]=monsters.tokens[0]; monsters.tokens[1].x=1; monsters.tokens[1].y=2;
    assert(ranged_eligibility(0,0,0,&heroes[0].ranged,0));
    monsters.tokens[1].focus=2; heroes[1].x=2; heroes[1].y=2;
    assert(!ranged_eligibility(0,0,0,&heroes[0].ranged,0));
    /* No diagonal death-zone restriction, including a diagonal target. */
    monsters.tokens[1].x=2; monsters.tokens[1].y=0; monsters.tokens[1].focus=0;
    assert(!ranged_eligibility(0,0,0,&heroes[0].ranged,0));
    room_fixture();
    /* Weapon-specific ranged critical/fumble thresholds affect resolution. */
    heroes[0].ranged.critical=11; heroes[0].ranged.fumble=2; heroes[0].ranged.dice=1;
    shot_draft(&s,0); dialog=shot_dialog(&s); SetDlgItemInt(dialog,RSHIT,11,FALSE);
    SetDlgItemTextA(dialog,RSDAMAGE,"3"); assert(shot_calculate(dialog,&s) && strstr(s.result,"Critical")); DestroyWindow(dialog);
    shot_draft(&s,0); dialog=shot_dialog(&s); SetDlgItemInt(dialog,RSHIT,2,FALSE);
    assert(shot_calculate(dialog,&s) && s.fumble && s.resolved && strstr(s.result,"misses")); DestroyWindow(dialog);
    /* Long reach permits diagonal hand-to-hand combat and extends the death zone. */
    heroes[0].x=1; heroes[0].y=1; monsters.tokens[0].x=2; monsters.tokens[0].y=2;
    assert(!melee_reachable(0,0,0)); heroes[0].melee.reach=2;
    assert(melee_reachable(0,0,0));
    assert(strstr(ranged_eligibility(0,0,0,&heroes[0].ranged,0),"melee reach"));
    /* Diagonal reach crosses map-piece boundaries in either direction. */
    {
        TYPE layouts[3][2]={{PASSAGE,NORMAL_ROOM},{NORMAL_ROOM,PASSAGE},{NORMAL_ROOM,SMALL_ROOM}};
        int layout,j;
        for(layout=0;layout<3;layout++) {
            PICE pieces[3]; int cells[36];
            memset(pieces,0,sizeof(pieces)); for(j=0;j<36;j++) cells[j]=-1;
            pieces[0].type=layouts[layout][0]; pieces[1].type=layouts[layout][1]; pieces[2].type=EMPTY;
            pieces[0].x=pieces[0].y=1; pieces[1].x=pieces[1].y=2;
            pieces[0].w=pieces[0].h=pieces[1].w=pieces[1].h=1;
            cells[1*6+1]=0; cells[2*6+2]=1;
            assert(map_restore(pieces,3,6,6,cells)); memset(game_fog(),0,MAX_PICE);
            game_fog()[0]=game_fog()[1]=1;
            heroes[0].x=heroes[0].y=1; heroes[0].melee.reach=2;
            monsters.tokens[0].x=monsters.tokens[0].y=2;
            assert(melee_reachable(0,0,0));
        }
    }
    room_fixture();
    heroes[0].melee.reach=1; monsters.tokens[0].x=5; monsters.tokens[0].y=1;
    monsters.count=2; monsters.tokens[1]=monsters.tokens[0];
    monsters.tokens[1].x=2; monsters.tokens[1].y=2; monsters.tokens[1].melee.reach=2;
    assert(strstr(ranged_eligibility(0,0,0,&heroes[0].ranged,0),"death zone"));
    monsters.tokens[1].melee.reach=1;
    assert(!ranged_eligibility(0,0,0,&heroes[0].ranged,0));
    monsters.count=1; monsters.tokens[0].x=2; monsters.tokens[0].y=2;
    heroes[0].melee.reach=2; assert(strstr(ranged_eligibility(0,0,0,&heroes[0].ranged,0),"melee reach"));
    room_fixture();
    /* Movement stops at first zone, focuses its owner, and can be undone. */
    monsters.tokens[0].x=4; x=7; y=1;
    assert(battle_destination(0,0,&x,&y) && x==3 && y==1);
    assert(move_hero(0,7,1,game_fog())); assert(heroes[0].x==3 && heroes[0].moved && monsters.tokens[0].focus==1);
    game_command(MGAMEUNDO); assert(heroes[0].x==1 && !heroes[0].moved && !monsters.tokens[0].focus);
    heroes[0].x=3; focus_enter(0,0); assert(monsters.tokens[0].focus==1);
    heroes[0].x=4; heroes[0].y=2; focus_prune(); assert(monsters.tokens[0].focus==1);
    heroes[0].x=5; focus_prune(); assert(!monsters.tokens[0].focus);
    heroes[0].x=3; heroes[0].y=1; focus_enter(0,0); assert(monsters.tokens[0].focus==1);
    heroes[0].x=heroes[0].y=-1; focus_prune(); assert(!monsters.tokens[0].focus);
    room_fixture(); heroes[0].moved=1; monsters.tokens[0].moved=1;
    game_command(MGAMETURN); assert(!heroes[0].moved && !monsters.tokens[0].moved);
    game_command(MGAMEUNDO); assert(heroes[0].moved && monsters.tokens[0].moved);
    room_fixture();
    /* Critical uses half Toughness, including odd Toughness, with no free attack. */
    shot_draft(&s,0); dialog=shot_dialog(&s);
    SetDlgItemInt(dialog,RSHIT,12,FALSE); SetDlgItemTextA(dialog,RSDAMAGE,"4 3 12 1 5");
    assert(shot_calculate(dialog,&s) && s.new_wounds==4 && monsters.tokens[0].wounds==8);
    screenshot(dialog,argv[1]); assert(shot_apply(&s)); assert(monsters.tokens[0].wounds==4);
    game_command(MGAMEUNDO); assert(monsters.tokens[0].wounds==8); DestroyWindow(dialog);
    /* Elf: enter only four physical damage dice; bonus twelves are automatic. */
    shot_draft(&s,0); dialog=shot_dialog(&s); SetDlgItemInt(dialog,RSHIT,8,FALSE);
    SetDlgItemTextA(dialog,RSDAMAGE,"12 1 1 1"); assert(shot_calculate(dialog,&s));
    {
        char pool[2048]; int wounds;
        GetDlgItemTextA(dialog,RSDAMAGE,pool,sizeof(pool));
        assert(!strncmp(pool,"12 1 1 1 ",9) && melee_damage(pool,4,7,&wounds));
        assert(s.new_wounds==(wounds>8?0:8-wounds) && monsters.tokens[0].wounds==8);
    }
    DestroyWindow(dialog);
    /* Miss and Cancel never damage the intended target. */
    shot_draft(&s,0); dialog=shot_dialog(&s); SetDlgItemInt(dialog,RSHIT,2,FALSE);
    assert(shot_calculate(dialog,&s) && s.new_wounds==8); assert(shot_apply(&s)); DestroyWindow(dialog);
    game_command(MGAMETURN);
    shot_draft(&s,0); dialog=shot_dialog(&s); SetDlgItemInt(dialog,RSHIT,12,FALSE);
    SetDlgItemTextA(dialog,RSDAMAGE,"7 7 7 7"); assert(shot_calculate(dialog,&s));
    SendMessage(dialog,WM_COMMAND,IDCANCEL,0); assert(monsters.tokens[0].wounds==8); DestroyWindow(dialog);
    /* A fumble without an eligible ally is a clean miss. */
    room_fixture(); shot_draft(&s,0); dialog=shot_dialog(&s);
    assert(!GetDlgItem(dialog,RSROLLDAMAGE) && !GetDlgItem(dialog,RSCALC));
    SetDlgItemInt(dialog,RSHIT,1,FALSE);
    assert(shot_calculate(dialog,&s) && s.resolved && s.fumble && s.friendly_index==-1);
    assert(strstr(s.result,"misses") && shot_apply(&s) && monsters.tokens[0].wounds==8); DestroyWindow(dialog);
    /* The target's controller can choose a nearby monster for a hero's fumble. */
    room_fixture(); monsters.count=2; monsters.tokens[1]=monsters.tokens[0];
    strcpy(monsters.tokens[1].name,"orc ally"); monsters.tokens[1].x=5; monsters.tokens[1].y=3;
    monsters.tokens[1].stats[3]=1; monsters.tokens[1].stats[7]=monsters.tokens[1].wounds=2;
    heroes[0].ranged.dice=1; shot_draft(&s,0); dialog=shot_dialog(&s); SetDlgItemInt(dialog,RSHIT,1,FALSE);
    assert(!shot_calculate(dialog,&s) && !s.resolved && (GetWindowLong(GetDlgItem(dialog,RSALLY),GWL_STYLE)&WS_VISIBLE));
    assert(SendDlgItemMessage(dialog,RSALLY,CB_GETCOUNT,0,0)==1);
    SendDlgItemMessage(dialog,RSALLY,CB_SETCURSEL,0,0);
    assert(shot_roll_damage(dialog,&s) && s.resolved && s.friendly_index==1);
    assert(shot_apply(&s) && monsters.tokens[0].wounds==8 && monsters.tokens[1].wounds<2); DestroyWindow(dialog);
    /* Monster fumbles use the target controller's nearby hero choices too. */
    room_fixture(); hero_count=2; hero_defaults(&heroes[1],4,2); heroes[1].x=1; heroes[1].y=2;
    heroes[1].stats[3]=1; heroes[1].wounds=2;
    assert(monster_ranged_defaults(&monsters.tokens[0].ranged,"goblin archer"));
    shot_draft(&s,1); dialog=shot_dialog(&s); SetDlgItemInt(dialog,RSHIT,1,FALSE);
    assert(!shot_calculate(dialog,&s) && !s.resolved);
    SendDlgItemMessage(dialog,RSALLY,CB_SETCURSEL,0,0);
    assert(shot_roll_damage(dialog,&s) && s.resolved && s.friendly_index==1);
    assert(shot_apply(&s) && heroes[0].wounds==4 && heroes[1].wounds<2); DestroyWindow(dialog);
    /* The movement warning must always have a visible, usable Next Turn button. */
    for(i=0;i<2;i++) {
        char status[256]; room_fixture();
        assert(monster_ranged_defaults(&monsters.tokens[0].ranged,"goblin archer"));
        heroes[0].moved=monsters.tokens[0].moved=1;
        shot_draft(&s,i); dialog=shot_dialog(&s);
        GetDlgItemTextA(dialog,RSSTATUS,status,sizeof(status));
        assert(strstr(status,"Use Next Turn") && (GetWindowLong(GetDlgItem(dialog,RSNEXTTURN),GWL_STYLE)&WS_VISIBLE));
        assert(IsWindowEnabled(GetDlgItem(dialog,RSNEXTTURN)));
        SendMessage(dialog,WM_COMMAND,RSNEXTTURN,0);
        assert(!shot_blocked(&s) && IsWindowEnabled(GetDlgItem(dialog,RSROLL)));
        assert(GetWindowLong(GetDlgItem(dialog,RSNEXTTURN),GWL_STYLE)&WS_VISIBLE);
        DestroyWindow(dialog);
    }
    /* Applied shots (including misses) consume the hero's normal ranged attack.
       The in-dialog turn reset is immediate, undoable, and clears all movement. */
    room_fixture(); shot_draft(&s,0); dialog=shot_dialog(&s);
    SetDlgItemInt(dialog,RSHIT,2,FALSE); assert(shot_calculate(dialog,&s));
    assert(!heroes[0].fired && shot_apply(&s) && heroes[0].fired); DestroyWindow(dialog);
    shot_draft(&s,0); dialog=shot_dialog(&s);
    assert(shot_blocked(&s) && !IsWindowEnabled(GetDlgItem(dialog,RSROLL)));
    assert(!(GetWindowLong(GetDlgItem(dialog,RSNEXTTURN),GWL_STYLE)&WS_DISABLED));
    SendMessage(dialog,WM_COMMAND,RSROLL,0); assert(!s.resolved);
    heroes[0].moved=monsters.tokens[0].moved=1;
    SendMessage(dialog,WM_COMMAND,RSNEXTTURN,0);
    assert(!heroes[0].fired && !heroes[0].moved && !monsters.tokens[0].moved);
    assert(!shot_blocked(&s) && IsWindowEnabled(GetDlgItem(dialog,RSROLL)));
    DestroyWindow(dialog); game_command(MGAMEUNDO); assert(heroes[0].fired && heroes[0].moved);
    game_command(MGAMETURN); assert(!heroes[0].fired);
    /* Actual combined automatic rolls preview valid attacks, without committing. */
    room_fixture();
    for(i=0;i<8;i++) {
        shot_draft(&s,0); dialog=shot_dialog(&s);
        SendMessage(dialog,WM_COMMAND,RSROLL,0); assert(s.resolved && monsters.tokens[0].wounds==8); DestroyWindow(dialog);
    }
    /* Version 5 retains equipped/unequipped choice, movement, and focus. */
    heroes[0].x=4; monsters.tokens[0].focus=1; heroes[0].moved=1; heroes[0].fired=1;
    assert(capture(&data)); assert(game_encode(&data,&bytes,&size));
    decoded=game_decode(bytes,size); assert(decoded && decoded->heroes[0].fired && decoded->heroes[0].moved && decoded->monsters.tokens[0].focus==1);
    assert(game_encode(decoded,&again,&again_size) && size==again_size && !memcmp(bytes,again,size));
    free(again); game_data_free(decoded);
    size-=pack_extension(&data);
    size-=4*data.hero_count;
    bytes[8]=5; fix_crc(bytes,size); decoded=game_decode(bytes,size);
    assert(decoded && !decoded->heroes[0].fired); game_data_free(decoded);
    for(i=0;i<data.hero_count;i++) size-=44+strlen(data.heroes[i].ranged.weapon);
    for(i=0;i<data.monsters.count;i++) size-=44+strlen(data.monsters.tokens[i].ranged.weapon);
    bytes[8]=4; fix_crc(bytes,size); decoded=game_decode(bytes,size);
    assert(decoded && decoded->heroes[0].ranged.range==48 && !decoded->heroes[0].moved); game_data_free(decoded); free(bytes);
    data.heroes[0].ranged.kind=0; data.heroes[0].ranged.critical=11; data.heroes[0].ranged.fumble=2;
    assert(game_encode(&data,&bytes,&size)); decoded=game_decode(bytes,size);
    assert(decoded && !decoded->heroes[0].ranged.kind && decoded->heroes[0].ranged.critical==11 && decoded->heroes[0].ranged.fumble==2); game_data_free(decoded); free(bytes);
    data.heroes[0].ranged.hit[2]=13; assert(!game_encode(&data,&bytes,&size)); free(data.cells);
    /* Profile dialog round trip, no default replacement without request. */
    memset(&edit,0,sizeof(edit)); edit.hero_kind=2;
    dialog=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGRANGEDPROFILE),NULL,ranged_profile_proc,(LPARAM)&edit);
    assert(dialog); SendMessage(dialog,WM_COMMAND,RPDEFAULT,0);
    assert(GetDlgItemInt(dialog,RPRANGE,NULL,FALSE)==48 && !edit.profile.range);
    assert(GetDlgItemInt(dialog,RPCRIT,NULL,FALSE)==12 && GetDlgItemInt(dialog,RPFUMBLE,NULL,FALSE)==1);
    SetDlgItemInt(dialog,RPCRIT,11,FALSE); SetDlgItemInt(dialog,RPFUMBLE,2,FALSE);
    CheckDlgButton(dialog,RPMOVED,BST_CHECKED); screenshot(dialog,argv[2]);
    SendMessage(dialog,WM_COMMAND,IDOK,0); assert(edit.profile.range==48 && edit.profile.critical==11 && edit.profile.fumble==2 && edit.moved); DestroyWindow(dialog);
    /* Click-select and drag routing, both directions, in both GM/player views. */
    room_fixture(); test_map_hwnd=CreateWindowExA(0,"STATIC","Ranged test map",WS_POPUP,0,0,800,800,NULL,NULL,GetModuleHandle(NULL),NULL);
    assert(test_map_hwnd); game_attach((WINDOW_DEF*)1,0); display_zoom=4; test_document.xx=test_document.yy=0;
    timer=SetTimer(NULL,0,50,attack_tick); assert(timer);
    for(i=0;i<2;i++) {
        MAP_HOOK *hook=(MAP_HOOK*)GetPropA(test_map_hwnd,"HQHeroMap"); hook->player=i;
        click_square(1,1); click_square(5,1); assert(ranged_dialogs==i*3+1);
        click_square(5,1); click_square(1,1); assert(ranged_dialogs==i*3+2);
        SendMessage(test_map_hwnd,WM_LBUTTONDOWN,MK_LBUTTON,mouse_at(1,1));
        SendMessage(test_map_hwnd,WM_LBUTTONUP,0,mouse_at(5,1)); assert(ranged_dialogs==i*3+3);
    }
    click_square(5,1);
    SendMessage(test_map_hwnd,WM_RBUTTONDOWN,0,mouse_at(0,0));
    click_square(1,1); assert(ranged_dialogs==6 && selected==0);
    SendMessage(test_map_hwnd,WM_RBUTTONDOWN,0,mouse_at(0,0));
    monsters.tokens[0].x=2; monsters.tokens[0].y=2; click_square(1,1); click_square(2,2); assert(ranged_dialogs==7);
    monsters.tokens[0].y=1; click_square(1,1); click_square(2,1); assert(melee_dialogs==1);
    /* A checked diagonal-reach profile attacks across adjacent cells in a room. */
    monsters.tokens[0].x=2; monsters.tokens[0].y=2; heroes[0].melee.reach=2;
    click_square(1,1); click_square(2,2); assert(melee_dialogs==2);
    heroes[0].melee.reach=1;
    assert(heroes[0].x==1 && heroes[0].y==1 && !heroes[0].moved);
    KillTimer(NULL,timer); DestroyWindow(test_map_hwnd); test_map_hwnd=NULL;
    /* Walls block, a revealed connecting door opens the ray; no unseen shots. */
    {
        PICE pieces[3]; int cells[24]; unsigned char fog[3]={1,1,0};
        memset(pieces,0,sizeof(pieces)); pieces[0].type=pieces[1].type=NORMAL_ROOM; pieces[2].type=EMPTY;
        pieces[0].w=pieces[1].w=3; pieces[0].h=pieces[1].h=4; pieces[1].x=3;
        for(i=0;i<24;i++) cells[i]=(i%6<3)?0:1;
        assert(map_restore(pieces,3,6,4,cells)); game_set_fog(fog,3);
        hero_count=monsters.count=0;
        assert(!ranged_los(1,1,4,1));
        Pice[2].type=DOOR; Pice[2].x=2; Pice[2].y=1; Pice[2].w=Pice[2].h=1; Pice[2].pos=East;
        assert(!ranged_los(1,1,4,1)); game_fog()[2]=1;
        assert(ranged_los(1,1,4,1)); game_fog()[1]=0; assert(!ranged_los(1,1,4,1));
    }
    game_shutdown(); mem_freeall();
    puts("PASS: ranged profiles/bands, both-direction click/drag targeting, diagonal shots, walls/doors/figures,");
    puts("movement/death-zone focus/turn reset, criticals/fumbles/Undo, native dialogs, dice, v5 persistence and v4 migration.");
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='hq-ranged-test-') as temp:
    work=Path(temp)
    (work/'check.c').write_text(HARNESS)
    exclude={'main.obj','maindm.obj','winddm.obj','game.obj'}
    objects=[str(p) for p in (ROOT/'obj/msvc').glob('*.obj') if p.name not in exclude]
    subprocess.run(['cl','/nologo','/D_CRT_SECURE_NO_WARNINGS','/I'+str(ROOT),'/I'+str(ROOT/'my_lib'),
                    '/I'+str(ROOT/'my_lib/windows'),'/I'+str(ROOT/'rsh'),'check.c','/Fe:check.exe',*objects,
                    str(ROOT/'obj/msvc/menu.res'),str(ROOT/'obj/msvc/grafic.res'),'user32.lib','gdi32.lib',
                    'shell32.lib','comdlg32.lib','version.lib','winspool.lib','ole32.lib','advapi32.lib'],cwd=work,check=True)
    subprocess.run([str(work/'check.exe'),str(ROOT/'obj/ranged-preview.bmp'),str(ROOT/'obj/ranged-profile-preview.bmp')],cwd=work,check=True,timeout=60)
