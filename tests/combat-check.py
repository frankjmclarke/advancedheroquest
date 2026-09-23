"""Melee model, native dialogs, mouse routing and save compatibility checks.
Run after build-msvc.bat in an x86 MSVC developer shell.
"""
from pathlib import Path
import ast
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
def literal(path):
    tree = ast.parse(path.read_text())
    return next(ast.literal_eval(n.value) for n in tree.body
                if isinstance(n, ast.Assign) and isinstance(n.value, ast.Constant)
                and any(isinstance(t, ast.Name) and t.id == 'HARNESS' for t in n.targets))
base = literal(ROOT / 'tests/game-check.py')
monster = (ROOT / 'tests/monster-check.py').read_text()
screenshot = monster[monster.index('static void screenshot'):monster.index('int main(int argc', monster.index('static void screenshot'))]
HARNESS = base[:base.index('int main(int argc')] + screenshot + r'''
static void draft(COMBAT *c,int side) {
    memset(c,0,sizeof(*c)); c->hero=heroes[0]; c->monster=monsters.tokens[0]; c->side=side; c->pending=-1;
}
static int dialog_seen,dialog_apply;
static BOOL CALLBACK close_combat(HWND hwnd,LPARAM unused) {
    char title[80]; COMBAT *c;
    GetWindowTextA(hwnd,title,sizeof(title));
    if(strcmp(title,"Hand-to-hand combat")) return TRUE;
    c=(COMBAT*)GetWindowLongPtr(hwnd,DWLP_USER); assert(c);
    dialog_seen++; assert(heroes[0].x==c->hero.x && monsters.tokens[0].x==c->monster.x);
    if(dialog_apply) {
        SetDlgItemInt(hwnd,MCHIT,6,FALSE); SetDlgItemTextA(hwnd,MCDAMAGE,"6 2");
        assert(combat_calculate(hwnd,c)); SendMessage(hwnd,WM_COMMAND,IDOK,0);
    } else SendMessage(hwnd,WM_COMMAND,IDCANCEL,0);
    return TRUE;
}
static void CALLBACK close_tick(HWND hwnd,UINT msg,UINT_PTR timer,DWORD time) { EnumThreadWindows(GetCurrentThreadId(),close_combat,0); }
int main(int argc,char **argv) {
    int i,room=-1,x,y,wounds,dice,hit[12],oldx,oldy,mx,my; PICE *p,*q;
    MELEE_PROFILE reference; COMBAT c; PROFILE_EDIT profile; HWND dialog;
    GAME_DATA data,*decoded; unsigned char *bytes; size_t size; UINT_PTR timer;
    assert(mem_alloc(100,100,1000,4096)); heap_clear();
    assert(SetCurrentDirectoryA(argv[1])); assert(read_table("sonne2.tab",NULL)); generate();
    hero_count=1; memset(&monsters,0,sizeof(monsters));
    for(i=0;i<MAX_PICE;i++) if(Pice[i].type==NORMAL_ROOM) { room=i; break; } assert(room>=0);
    memset(game_fog(),0,MAX_PICE); game_fog()[room]=1; monsters.seen[room]=1;
    hero_defaults(&heroes[0],0,1); heroes[0].stats[0]=6; heroes[0].stats[3]=6;
    strcpy(heroes[0].melee.weapon,"Test sword"); heroes[0].melee.dice=2;
    for(i=0;i<12;i++) heroes[0].melee.hit[i]=6;
    monsters.count=1; strcpy(monsters.tokens[0].name,"skaven warrior");
    monsters.tokens[0].room=room; monsters.tokens[0].stats[0]=6; monsters.tokens[0].stats[3]=6;
    monsters.tokens[0].stats[7]=monsters.tokens[0].wounds=3;
    assert(melee_reference("skaven warrior",&monsters.tokens[0].melee));
    for(y=Pice[room].y;y<Pice[room].y+Pice[room].h;y++) for(x=Pice[room].x;x<Pice[room].x+Pice[room].w-1;x++) {
        if(get_square(x,y,&p) && get_square(x+1,y,&q) && p==&Pice[room] && q==p) goto found;
    }
    assert(0);
found:
    heroes[0].x=x; heroes[0].y=y; monsters.tokens[0].x=x+1; monsters.tokens[0].y=y;
    oldx=x; oldy=y; mx=x+1; my=y;
    assert(melee_adjacent(0,0)); monsters.tokens[0].y++; assert(!melee_adjacent(0,0)); monsters.tokens[0].y--;
    monsters.tokens[0].x++; assert(!melee_adjacent(0,0)); monsters.tokens[0].x--;
    game_fog()[room]=0; assert(!melee_adjacent(0,0)); game_fog()[room]=1;
    heroes[0].wounds=0; assert(!melee_adjacent(0,0)); heroes[0].wounds=4;
    assert(room_melee("SKAVEN WARRIORS",&dice,hit) && dice==3 && hit[0]==2 && hit[11]==10);
    memset(&reference,0,sizeof(reference)); assert(!melee_reference("unlisted beast",&reference) && !reference.dice);
    assert(melee_damage("6 5",2,6,&wounds) && wounds==1);
    assert(melee_damage("12,12,1,6",2,6,&wounds) && wounds==3);
    assert(melee_damage("12 1",1,99,&wounds) && wounds==1);
    assert(!melee_damage("12",1,6,&wounds)); assert(!melee_damage("6 7",1,6,&wounds));
    assert(!melee_damage("-1",1,6,&wounds)); assert(!melee_damage("13",1,6,&wounds));
    assert(!melee_damage("6x",1,6,&wounds)); assert(!melee_damage("",1,6,&wounds));
    assert(!melee_damage("999999999999999999999",1,6,&wounds));
    draft(&c,0); c.monster.stats[0]=13; assert(!combat_required(&c));
    draft(&c,0); c.hero.melee.hit[5]=0; assert(!combat_required(&c));
    draft(&c,0); dialog=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGCOMBAT),NULL,combat_proc,(LPARAM)&c); assert(dialog);
    SetDlgItemInt(dialog,MCHIT,5,FALSE); assert(combat_calculate(dialog,&c));
    assert(c.monster.wounds==3 && c.pending==-1 && monsters.tokens[0].wounds==3);
    assert(!combat_calculate(dialog,&c)); DestroyWindow(dialog);
    /* Manual initial dice automatically gain all outstanding bonus rolls. */
    {
        char pool[2048]="12 12"; int again;
        assert(damage_auto(pool,sizeof(pool),2,6,&wounds));
        assert(!strncmp(pool,"12 12 ",6) && melee_damage(pool,2,6,&again) && wounds==again && wounds>=2);
        strcpy(pool,"12"); assert(!damage_auto(pool,sizeof(pool),2,6,&wounds));
        strcpy(pool,"12 2 1"); assert(damage_auto(pool,sizeof(pool),2,6,&wounds) && !strcmp(pool,"12 2 1"));
    }
    /* One button rolls and calculates, without changing the live encounter. */
    for(i=0;i<6;i++) {
        char generated[2048]; BOOL valid;
        draft(&c,i%2); dialog=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGCOMBAT),NULL,combat_proc,(LPARAM)&c);
        assert(!GetDlgItem(dialog,MCROLLDAMAGE) && !GetDlgItem(dialog,MCCALC));
        SendMessage(dialog,WM_COMMAND,MCROLLHIT,0);
        dice=(int)GetDlgItemInt(dialog,MCHIT,&valid,FALSE); assert(valid && dice>=1 && dice<=12);
        assert(c.resolved && c.steps==1);
        GetDlgItemTextA(dialog,MCDAMAGE,generated,sizeof(generated));
        if(dice!=1 && dice>=combat_required(&c)) {
            assert(melee_damage(generated,c.side?c.monster.melee.dice:c.hero.melee.dice,c.side?c.hero.stats[3]:c.monster.stats[3],&wounds));
        } else assert(!generated[0]);
        SendMessage(dialog,WM_COMMAND,MCROLLHIT,0); assert(c.steps==1);
        DestroyWindow(dialog);
    }
    /* Critical, optional free attack and fumble reversal in one draft. */
    draft(&c,0); dialog=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGCOMBAT),NULL,combat_proc,(LPARAM)&c);
    SetDlgItemInt(dialog,MCHIT,12,FALSE); SetDlgItemTextA(dialog,MCDAMAGE,"12");
    assert(!combat_calculate(dialog,&c) && c.monster.wounds==3);
    SetDlgItemTextA(dialog,MCDAMAGE,"12 2 1"); assert(combat_calculate(dialog,&c));
    assert(c.monster.wounds==2 && c.pending==0 && monsters.tokens[0].wounds==3);
    SendMessage(dialog,WM_COMMAND,MCNEXT,0); assert(c.side==0 && !c.resolved);
    SetDlgItemInt(dialog,MCHIT,1,FALSE); assert(combat_calculate(dialog,&c) && c.pending==1);
    SendMessage(dialog,WM_COMMAND,MCNEXT,0); assert(c.side==1);
    SetDlgItemInt(dialog,MCHIT,8,FALSE); SetDlgItemTextA(dialog,MCDAMAGE,"6 1 2");
    assert(combat_calculate(dialog,&c)); assert(c.hero.wounds==3 && heroes[0].wounds==4);
    screenshot(dialog,argv[2]);
    assert(combat_apply(&c)); assert(heroes[0].wounds==3 && monsters.tokens[0].wounds==2);
    game_command(MGAMEUNDO); assert(heroes[0].wounds==4 && monsters.tokens[0].wounds==3);
    DestroyWindow(dialog);
    /* Death, identity ledger, board removal and entire combat Undo. */
    monsters.tokens[0].unique=1;
    draft(&c,0); dialog=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGCOMBAT),NULL,combat_proc,(LPARAM)&c);
    SetDlgItemInt(dialog,MCHIT,6,FALSE); SetDlgItemTextA(dialog,MCDAMAGE,"12 12 6 6");
    assert(combat_calculate(dialog,&c)); assert(c.monster.wounds==0 && c.pending==-1);
    assert(combat_apply(&c)); assert(monsters.tokens[0].x==-1 && character_dead("skaven warrior"));
    game_command(MGAMEUNDO); assert(monsters.tokens[0].wounds==3 && monsters.tokens[0].x==mx && !monsters.dead_count);
    DestroyWindow(dialog);
    heroes[0].kind=4; draft(&c,1); c.hero.wounds=0; c.steps=1; c.resolved=1;
    assert(combat_apply(&c) && heroes[0].x==-1); game_command(MGAMEUNDO); heroes[0].kind=0;
    /* Round trip equipment and printed tables; invalid profiles fail encoding. */
    assert(capture(&data)); assert(game_encode(&data,&bytes,&size)); free(data.cells);
    decoded=game_decode(bytes,size); assert(decoded);
    assert(!strcmp(decoded->heroes[0].melee.weapon,heroes[0].melee.weapon));
    assert(decoded->heroes[0].melee.dice==heroes[0].melee.dice);
    assert(!memcmp(decoded->heroes[0].melee.hit,heroes[0].melee.hit,sizeof(heroes[0].melee.hit)));
    assert(!memcmp(&decoded->monsters.tokens[0].melee,&monsters.tokens[0].melee,sizeof(MELEE_PROFILE)));
    free(bytes); decoded->heroes[0].melee.hit[2]=13; assert(!game_encode(decoded,&bytes,&size)); game_data_free(decoded);
    /* Missing equipment upgrades on load; partial and custom data survive. */
    assert(capture(&data)); memset(&data.heroes[0].melee,0,sizeof(MELEE_PROFILE));
    assert(game_encode(&data,&bytes,&size));
    decoded=game_decode(bytes,size); assert(decoded && decoded->heroes[0].melee.dice==4);
    assert(!memcmp(decoded->heroes[0].stats,data.heroes[0].stats,sizeof(data.heroes[0].stats)));
    game_data_free(decoded); free(bytes);
    data.heroes[0].melee.hit[5]=8; /* even a single entered cell must be kept */
    assert(game_encode(&data,&bytes,&size)); decoded=game_decode(bytes,size);
    assert(decoded && !decoded->heroes[0].melee.dice && decoded->heroes[0].melee.hit[5]==8);
    game_data_free(decoded); free(bytes); free(data.cells);
    {
        const int expected_dice[9]={4,4,3,1,2,3,2,4,2}; HERO preset;
        for(i=0;i<HERO_CLASS_COUNT;i++) {
            hero_defaults(&preset,i,1); assert(preset.melee.dice==expected_dice[i]);
            for(dice=0;dice<12;dice++) assert(preset.melee.hit[dice]>=1 && preset.melee.hit[dice]<=12);
            if(i>=4) assert(strstr(preset.melee.weapon,"suggested"));
        }
        hero_defaults(&preset,0,1); assert(preset.melee.hit[3]==3 && preset.melee.hit[11]==10);
        hero_defaults(&preset,2,1); assert(preset.melee.hit[3]==2 && preset.melee.hit[9]==8);
        hero_defaults(&preset,3,1); assert(preset.melee.hit[0]==3 && preset.melee.hit[7]==10);
    }
    /* Real profile controls, bounded entry and native layout. */
    profile.profile=heroes[0].melee; profile.ws=6; profile.t=6; profile.reference=NULL; profile.hero_kind=0;
    dialog=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGMELEE),NULL,profile_proc,(LPARAM)&profile); assert(dialog);
    SetDlgItemTextA(dialog,MPWEAPON,"Sword and shield"); SetDlgItemInt(dialog,MPHIT+11,13,FALSE);
    SendMessage(dialog,WM_COMMAND,IDOK,0); assert(profile.profile.hit[11]==6);
    SetDlgItemInt(dialog,MPHIT+11,10,FALSE); screenshot(dialog,argv[3]);
    SendMessage(dialog,WM_COMMAND,IDOK,0); assert(!strcmp(profile.profile.weapon,"Sword and shield") && profile.profile.hit[11]==10);
    DestroyWindow(dialog);
    /* Missing hero entries are previewed from defaults without overwriting
       existing values or changing the draft on Cancel. Reset leaves WS/T alone. */
    memset(&profile,0,sizeof(profile)); profile.hero_kind=2; profile.ws=9; profile.t=7;
    strcpy(profile.profile.weapon,"Custom blade"); profile.profile.hit[4]=11;
    dialog=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGMELEE),NULL,profile_proc,(LPARAM)&profile);
    assert(GetDlgItemInt(dialog,MPDICE,NULL,FALSE)==3);
    assert(GetDlgItemInt(dialog,MPHIT+4,NULL,FALSE)==11);
    assert(GetDlgItemInt(dialog,MPHIT+3,NULL,FALSE)==2);
    assert(!profile.profile.dice); /* draft changes only on Save */
    SetDlgItemInt(dialog,MPWS,8,FALSE); SendMessage(dialog,WM_COMMAND,MPREF,0);
    assert(GetDlgItemInt(dialog,MPHIT+4,NULL,FALSE)==3);
    assert(GetDlgItemInt(dialog,MPWS,NULL,FALSE)==8);
    screenshot(dialog,argv[3]);
    SendMessage(dialog,WM_COMMAND,IDCANCEL,0); assert(!profile.profile.dice && profile.profile.hit[4]==11);
    DestroyWindow(dialog);
    /* Actual drag messages route attacks in both views, leaving positions fixed.
       The timer closes only test dialogs on this thread, never another process. */
    test_map_hwnd=CreateWindowExA(0,"STATIC","Melee test map",WS_POPUP,0,0,800,800,NULL,NULL,GetModuleHandle(NULL),NULL); assert(test_map_hwnd);
    game_attach((WINDOW_DEF*)1,0); display_zoom=4;
    test_document.xx=(oldx+1)*32-150; test_document.yy=(Ysize-oldy)*32-150;
    timer=SetTimer(NULL,0,50,close_tick); assert(timer);
    for(i=0;i<2;i++) {
        MAP_HOOK *hook=(MAP_HOOK*)GetPropA(test_map_hwnd,"HQHeroMap"); hook->player=i;
        SendMessage(test_map_hwnd,WM_LBUTTONDOWN,MK_LBUTTON,mouse_at(oldx,oldy));
        SendMessage(test_map_hwnd,WM_MOUSEMOVE,MK_LBUTTON,mouse_at(mx,my));
        SendMessage(test_map_hwnd,WM_LBUTTONUP,0,mouse_at(mx,my));
        assert(dialog_seen==i*2+1 && heroes[0].wounds==4 && monsters.tokens[0].wounds==3);
        SendMessage(test_map_hwnd,WM_LBUTTONDOWN,MK_LBUTTON,mouse_at(mx,my));
        SendMessage(test_map_hwnd,WM_LBUTTONUP,0,mouse_at(oldx,oldy));
        assert(dialog_seen==i*2+2);
    }
    dialog_apply=1;
    SendMessage(test_map_hwnd,WM_LBUTTONDOWN,MK_LBUTTON,mouse_at(oldx,oldy));
    SendMessage(test_map_hwnd,WM_LBUTTONUP,0,mouse_at(mx,my));
    assert(dialog_seen==5 && monsters.tokens[0].wounds==2);
    assert(heroes[0].x==oldx && heroes[0].y==oldy && monsters.tokens[0].x==mx && monsters.tokens[0].y==my);
    game_command(MGAMEUNDO); assert(monsters.tokens[0].wounds==3);
    KillTimer(NULL,timer); DestroyWindow(test_map_hwnd); test_map_hwnd=NULL;
    /* Closed room walls and revealed doors in all four orientations. */
    for(i=0;i<4;i++) {
        PICE pieces[3]; int cells[36],j,dx=i==East?1:i==West?-1:0,dy=i==North?1:i==South?-1:0;
        memset(pieces,0,sizeof(pieces));
        for(j=0;j<36;j++) cells[j]=-1;
        pieces[0].type=pieces[1].type=NORMAL_ROOM; pieces[2].type=EMPTY;
        pieces[0].x=pieces[0].y=2; pieces[1].x=2+dx; pieces[1].y=2+dy;
        pieces[0].w=pieces[0].h=pieces[1].w=pieces[1].h=1;
        cells[2*6+2]=0; cells[(2+dy)*6+2+dx]=1;
        assert(map_restore(pieces,3,6,6,cells)); memset(game_fog(),0,MAX_PICE); game_fog()[0]=game_fog()[1]=1;
        heroes[0].x=heroes[0].y=2; monsters.tokens[0].x=2+dx; monsters.tokens[0].y=2+dy;
        assert(!melee_adjacent(0,0));
        Pice[2].type=DOOR; Pice[2].x=Pice[2].y=2; Pice[2].w=Pice[2].h=1; Pice[2].pos=(DIRECTION)i;
        assert(!melee_adjacent(0,0)); game_fog()[2]=1;
        assert(melee_adjacent(0,0)); Pice[2].type=SECRET; assert(melee_adjacent(0,0));
        game_fog()[1]=0; assert(!melee_adjacent(0,0));
    }
    game_shutdown(); mem_freeall();
    puts("PASS: melee lookup, adjacency/fog, hit/miss, exploding dice, critical/fumble free attacks,");
    puts("draft isolation, Apply/Undo/death ledger, profiles/codec, native dialogs and both-direction drag routing in both views.");
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='hq-melee-test-') as temp:
    work = Path(temp)
    (work / 'check.c').write_text(HARNESS)
    exclude = {'main.obj', 'maindm.obj', 'winddm.obj', 'game.obj'}
    objects = [str(p) for p in (ROOT / 'obj/msvc').glob('*.obj') if p.name not in exclude]
    subprocess.run(['cl', '/nologo', '/D_CRT_SECURE_NO_WARNINGS', '/I'+str(ROOT),
                    '/I'+str(ROOT/'my_lib'), '/I'+str(ROOT/'my_lib/windows'), '/I'+str(ROOT/'rsh'),
                    'check.c', '/Fe:check.exe', *objects, str(ROOT/'obj/msvc/menu.res'), str(ROOT/'obj/msvc/grafic.res'),
                    'user32.lib', 'gdi32.lib', 'shell32.lib', 'comdlg32.lib', 'version.lib', 'winspool.lib', 'ole32.lib', 'advapi32.lib'], cwd=work, check=True)
    subprocess.run([str(work/'check.exe'), str(ROOT/'tables'), str(ROOT/'obj/combat-preview.bmp'),
                    str(ROOT/'obj/melee-profile-preview.bmp')], cwd=work, check=True, timeout=60)

