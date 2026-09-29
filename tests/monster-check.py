"""Monster integration regression. Run in the x86 MSVC developer shell after building."""
from pathlib import Path
import ast
import json
import runpy
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
# Reuse the established native map/window test fixture without running its tests.
tree = ast.parse((ROOT / 'tests/game-check.py').read_text())
base = next(ast.literal_eval(n.value) for n in tree.body
            if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'HARNESS' for t in n.targets))
HARNESS = base[:base.index('int main(int argc')] + r'''
static void crc_fix(unsigned char *bytes,size_t size) {
    unsigned long crc=0xffffffffUL; size_t i; int j;
    for(i=16;i<size;i++) { crc^=bytes[i]; for(j=0;j<8;j++) crc=(crc>>1)^((crc&1)?0xedb88320UL:0); }
    crc=~crc; for(j=0;j<4;j++) bytes[12+j]=(unsigned char)(crc>>(8*j));
}
static void screenshot(HWND dialog,const char *path) {
    HDC screen=GetDC(NULL),dc=CreateCompatibleDC(screen); HBITMAP bitmap,old;
    BITMAPINFO info; BITMAPFILEHEADER file; void *pixels; FILE *fp; RECT r; int w,h;
    ShowWindow(dialog,SW_SHOWNOACTIVATE); UpdateWindow(dialog); GetWindowRect(dialog,&r);
    w=r.right-r.left; h=r.bottom-r.top; memset(&info,0,sizeof(info));
    info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth=w; info.bmiHeader.biHeight=-h;
    info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32;
    bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&pixels,NULL,0); assert(bitmap);
    old=(HBITMAP)SelectObject(dc,bitmap); PrintWindow(dialog,dc,0);
    memset(&file,0,sizeof(file)); file.bfType=0x4d42; file.bfOffBits=sizeof(file)+sizeof(info.bmiHeader); file.bfSize=file.bfOffBits+w*h*4;
    fp=fopen(path,"wb"); assert(fp); fwrite(&file,sizeof(file),1,fp); fwrite(&info.bmiHeader,sizeof(info.bmiHeader),1,fp); fwrite(pixels,w*h*4,1,fp); fclose(fp);
    SelectObject(dc,old); DeleteObject(bitmap); DeleteDC(dc); ReleaseDC(NULL,screen);
}
int main(int argc,char **argv) {
    int room=-1,i,j,x,y,n,oldx,oldy,dx=-1,dy=-1,capacity;
    PICE *p; HWND dialog; unsigned char *bytes,*bytes2; size_t size,size2; GAME_DATA d,*decoded;
    char error[256];
    char *encounter[]={"#42","2 Goblins, 1 Orc (20 Gold Crowns)",NULL};
    char *overflow[]={"40 Goblins",NULL}; char *alternative[]={"1 Orc or 2 Goblins",NULL};
    char *unknown[]={"  2 Strange Beasts (20 Gold Crowns)",NULL};
    assert(mem_alloc(100,100,1000,4096)); heap_clear();
    assert(SetCurrentDirectoryA(argv[1])); assert(read_table("sonne2.tab",NULL)); generate();
    assert(monsters.count==0); game_changed(); assert(monsters.count==0);
    hero_count=0; memset(&monsters,0,sizeof(monsters));
    for(i=0;i<MAX_PICE;i++) if(Pice[i].type==NORMAL_ROOM) { room=i; break; }
    assert(room>=0); Pice[room].text=encounter;
    memset(game_fog(),0,MAX_PICE); game_fog()[room]=1;
    monsters_reveal(); assert(monsters.count==3 && monsters.seen[room]==1);
    assert(!strcmp(monsters.tokens[0].name,"goblin") && monsters.tokens[0].wounds==2);
    assert(!strcmp(monsters.tokens[2].name,"orc") && monsters.tokens[2].wounds==4);
    for(i=0;i<3;i++) { assert(monsters.tokens[i].x>=0); assert(monster_at(monsters.tokens[i].x,monsters.tokens[i].y)==i); }
    monsters_reveal(); assert(monsters.count==3);
    assert(room_monsters(alternative,monster_add_reference,&room)); assert(monsters.count==3);
    assert(room_monsters(unknown,monster_add_reference,&room)); assert(monsters.count==3);
    assert(!monster_can_move(0,-1,0));
    assert(!monster_can_move(0,monsters.tokens[1].x,monsters.tokens[1].y));
    hero_defaults(&heroes[0],0,1); hero_count=1;
    assert(!can_move(0,monsters.tokens[0].x,monsters.tokens[0].y,NULL));
    oldx=monsters.tokens[0].x; oldy=monsters.tokens[0].y;
    for(y=Pice[room].y;y<Pice[room].y+Pice[room].h;y++) for(x=Pice[room].x;x<Pice[room].x+Pice[room].w;x++)
        if(monster_can_move(0,x,y) && (x!=oldx || y!=oldy)) { dx=x; dy=y; }
    assert(dx>=0); heroes[0].x=dx; heroes[0].y=dy; assert(!monster_can_move(0,dx,dy)); heroes[0].x=heroes[0].y=-1;
    test_map_hwnd=CreateWindowExA(0,"STATIC","Monster test",WS_POPUP,0,0,1200,1200,NULL,NULL,GetModuleHandle(NULL),NULL); assert(test_map_hwnd);
    game_attach((WINDOW_DEF*)1,0); display_zoom=3;
    test_document.xx=(oldx+1)*24-300; test_document.yy=(Ysize-oldy)*24-300;
    SendMessage(test_map_hwnd,WM_LBUTTONDOWN,MK_LBUTTON,mouse_at(oldx,oldy));
    SendMessage(test_map_hwnd,WM_MOUSEMOVE,MK_LBUTTON,mouse_at(dx,dy)); assert(preview_monster==0);
    SendMessage(test_map_hwnd,WM_LBUTTONUP,0,mouse_at(dx,dy)); assert(monsters.tokens[0].x==dx && monsters.tokens[0].y==dy);
    game_command(MGAMEUNDO); assert(monsters.tokens[0].x==oldx && monsters.tokens[0].y==oldy);
    SendMessage(test_map_hwnd,WM_LBUTTONDOWN,MK_LBUTTON,mouse_at(oldx,oldy));
    SendMessage(test_map_hwnd,WM_LBUTTONUP,0,mouse_at(oldx,oldy));
    SendMessage(test_map_hwnd,WM_LBUTTONDBLCLK,MK_LBUTTON,mouse_at(oldx,oldy));
    SendMessage(test_map_hwnd,WM_LBUTTONUP,0,mouse_at(oldx,oldy));
    assert(monsters.tokens[0].wounds==1 && monsters.tokens[0].x==oldx && !move_mode);
    monsters.tokens[0].unique=1; strcpy(monsters.tokens[0].name,"the goblin king");
    SendMessage(test_map_hwnd,WM_LBUTTONDBLCLK,MK_LBUTTON,mouse_at(oldx,oldy));
    SendMessage(test_map_hwnd,WM_LBUTTONUP,0,mouse_at(oldx,oldy));
    assert(!monsters.tokens[0].wounds && monsters.tokens[0].x==-1 && character_dead("THE GOBLIN KING"));
    game_command(MGAMEUNDO); assert(monsters.tokens[0].wounds==1 && !character_dead("the goblin king") && monsters.tokens[0].x==oldx);
    game_command(MGAMEUNDO); assert(!monsters.tokens[0].wounds && character_dead("the goblin king"));
    DestroyWindow(test_map_hwnd); test_map_hwnd=NULL;
    assert(capture(&d)); assert(game_encode(&d,&bytes,&size)); free(d.cells);
    decoded=game_decode(bytes,size); assert(decoded && !memcmp(&decoded->monsters,&monsters,sizeof(monsters)));
    assert(game_encode(decoded,&bytes2,&size2) && size==size2 && !memcmp(bytes,bytes2,size)); free(bytes2);
    decoded->monsters.tokens[1].x=monsters.tokens[2].x; decoded->monsters.tokens[1].y=monsters.tokens[2].y;
    assert(!game_encode(decoded,&bytes2,&size2)); game_data_free(decoded); free(bytes);
    selected_monster=1;
    dialog=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGMONSTERS),NULL,monsters_proc,0); assert(dialog);
    SetDlgItemInt(dialog,GMWOUNDS,1,FALSE); assert(monster_editor_apply(dialog,0)); assert(monsters.tokens[1].wounds==1);
    SetDlgItemInt(dialog,GMWOUNDS,2,FALSE); assert(monster_editor_apply(dialog,0)); assert(monsters.tokens[1].wounds==2);
    SetDlgItemTextA(dialog,GMNAME,"  Ogre Keeper  "); SetDlgItemInt(dialog,GMMAX,5,FALSE); SetDlgItemInt(dialog,GMWOUNDS,5,FALSE);
    CheckDlgButton(dialog,GMUNIQUE,BST_CHECKED); assert(monster_editor_apply(dialog,1)); assert(!strcmp(monsters.tokens[3].name,"ogre keeper"));
    monster_list(dialog); monster_fields(dialog); screenshot(dialog,argv[2]); DestroyWindow(dialog);
    /* More monsters than room squares are preserved in the unplaced roster. */
    n=monsters.count; assert(!room_monsters(overflow,monster_add_reference,&room)); assert(monsters.count==n+40);
    capacity=0; for(i=0;i<monsters.count;i++) if(monsters.tokens[i].wounds && monsters.tokens[i].x<0) capacity++;
    assert(capacity>0); assert(capture(&d)); assert(game_encode(&d,&bytes,&size)); free(d.cells);
    decoded=game_decode(bytes,size); assert(decoded && decoded->monsters.count==monsters.count); game_data_free(decoded); free(bytes);
    /* Next dungeon drops ordinary encounters, retains the adventure death ledger. */
    game_new_dungeon("Next adventure dungeon"); assert(character_dead("the goblin king"));
    assert(monsters.count==0 && heroes[0].x==-1 && heroes[0].y==-1);
    game_changed(); game_command(MGAMEUNDO); assert(monsters.count==0);
    for(i=0;i<monsters.count;i++) assert(strcmp(monsters.tokens[i].name,"ogre keeper")); n=monsters.count; game_fog()[room]=1; monsters_reveal(); assert(monsters.count==n+3);
    n=monsters.count; monster_add_reference("the goblin king",monsters.tokens[1].stats,1,&room); assert(monsters.count==n);
    /* Version 3 stores the chosen map view. Version 2 had no view word. */
    assert(capture(&d)); d.player_view=1; assert(game_encode(&d,&bytes,&size)); free(d.cells);
    decoded=game_decode(bytes,size); assert(decoded && decoded->player_view==1);
    decoded->player_view=2; assert(!game_encode(decoded,&bytes2,&size2)); game_data_free(decoded);
    /* Strip v5 ranged/turn data to build a genuine v4 save. */
    size-=pack_extension(&d);
    size-=4*d.hero_count;
    for(i=0;i<d.hero_count;i++) size-=44+strlen(d.heroes[i].ranged.weapon);
    for(i=0;i<d.monsters.count;i++) size-=44+strlen(d.monsters.tokens[i].ranged.weapon);
    bytes[8]=4; crc_fix(bytes,size); decoded=game_decode(bytes,size); assert(decoded); game_data_free(decoded);
    /* Strip the v4 profile extension to construct actual older layouts. */
    for(i=0;i<d.hero_count;i++) size-=56+strlen(d.heroes[i].melee.weapon);
    for(i=0;i<d.monsters.count;i++) size-=56+strlen(d.monsters.tokens[i].melee.weapon);
    bytes[8]=3; crc_fix(bytes,size);
    decoded=game_decode(bytes,size); assert(decoded && decoded->player_view==1 && decoded->heroes[0].melee.dice>0);
    game_data_free(decoded);
    bytes[8]=2; size-=4; crc_fix(bytes,size);
    decoded=game_decode(bytes,size); assert(decoded && decoded->player_view==-1 && decoded->monsters.count==monsters.count);
    game_data_free(decoded); free(bytes);
    /* Real version-1 layout, with no monster extension, remains readable. */
    memset(&monsters,0,sizeof(monsters)); assert(capture(&d));
    d.heroes[0].kind=1; /* Valid fantasy class, absent from the one-hero pack. */
    assert(game_encode(&d,&bytes,&size)); free(d.cells);
    size-=pack_extension(&d);
    size-=4*d.hero_count;
    for(i=0;i<d.hero_count;i++) size-=44+strlen(d.heroes[i].ranged.weapon);
    for(i=0;i<d.hero_count;i++) size-=56+strlen(d.heroes[i].melee.weapon);
    size-=12+4*MAX_PICE; bytes[8]=1; crc_fix(bytes,size);
    decoded=game_decode(bytes,size); assert(decoded && decoded->monsters.count==0 && decoded->monsters.dead_count==0);
    game_data_free(decoded);
    assert(pack_select_campaign(argv[3],error));
    assert(pack_current()->hero_count==1);
    assert(!game_decode(bytes,size)); /* Reject before indexing heroes[1]. */
    free(bytes);
    /* A 100% setting omits every ordinary model once and keeps its gold
       award fixed across repeated reveals and a save round trip. */
    game_new_dungeon("Difficulty test");
    for(i=0;i<MAX_PICE;i++) if(Pice[i].type==NORMAL_ROOM) { room=i; break; }
    assert(room>=0); Pice[room].text=encounter;
    monster_omit_pct=100; gold_bonus_pct=25;
    memset(game_fog(),0,MAX_PICE); game_fog()[room]=1;
    monsters_reveal(); assert(monsters.seen[room]==1 && monsters.count==0 && room_gold_bonus_pct[room]==25);
    { char *contents=game_room_contents(&Pice[room]); assert(contents && strstr(contents,"25 Gold Crowns")); free(contents); }
    {
        size_t lines=draw_monster_liste(NULL),k; _UBYTE **list=(_UBYTE**)calloc(lines,sizeof(*list)); int found=0;
        assert(list && draw_monster_liste(list)==lines);
        for(k=0;k<lines;k++) { if(strstr(list[k],"25 Gold Crowns")) found=1; free(list[k]); }
        free(list); assert(found);
    }
    monster_omit_pct=0; gold_bonus_pct=0; monsters_reveal(); assert(monsters.count==0);
    assert(capture(&d)); assert(game_encode(&d,&bytes,&size)); free(d.cells);
    decoded=game_decode(bytes,size); assert(decoded && decoded->room_gold_bonus_pct[room]==25 && decoded->monsters.seen[room]==1);
    game_data_free(decoded); free(bytes);
    game_shutdown(); mem_freeall();
    puts("PASS: roster parsing, automatic placement, ambiguity review, collisions, drag, double-click damage, deaths/Undo, editor, overflow, v3 persistence and v1/v2 compatibility.");
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='hq-monster-test-') as temp:
    work = Path(temp)
    one_hero = json.loads((ROOT / 'data/packs/fantasy.json').read_text(encoding='utf-8'))
    one_hero['heroes'] = one_hero['heroes'][:1]
    compile_pack = runpy.run_path(str(ROOT / 'tools/compile-packs.py'))['compile_pack']
    (work / 'one-hero.hqp').write_bytes(compile_pack(one_hero))
    (work / 'one-hero.tab').write_text(';character-pack one-hero.hqp\n',encoding='ascii')
    (work / 'check.c').write_text(HARNESS)
    exclude = {'main.obj', 'maindm.obj', 'winddm.obj', 'game.obj'}
    objects = [str(p) for p in (ROOT / 'obj/msvc').glob('*.obj') if p.name not in exclude]
    subprocess.run(['cl', '/nologo', '/D_CRT_SECURE_NO_WARNINGS', '/I'+str(ROOT),
                    '/I'+str(ROOT/'my_lib'), '/I'+str(ROOT/'my_lib/windows'), '/I'+str(ROOT/'rsh'),
                    'check.c', '/Fe:check.exe', *objects, str(ROOT/'obj/msvc/menu.res'), str(ROOT/'obj/msvc/grafic.res'),
                    'user32.lib', 'gdi32.lib', 'shell32.lib', 'comdlg32.lib', 'version.lib', 'winspool.lib', 'ole32.lib', 'advapi32.lib', 'comctl32.lib'], cwd=work, check=True)
    subprocess.run([str(work/'check.exe'), str(ROOT/'tables'), str(ROOT/'obj/monster-preview.bmp'),str(work/'one-hero.tab')], cwd=work, check=True, timeout=60)
