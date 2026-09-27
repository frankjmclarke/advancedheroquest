"""Exercise all Sentinel encounter literals, five real decks, saves and native UI."""
from pathlib import Path
import ast
import json
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
tree = ast.parse((ROOT / 'tests/game-check.py').read_text())
base = next(ast.literal_eval(n.value) for n in tree.body
            if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'HARNESS' for t in n.targets))
encounters = set()
for name in ['monster', 'spires', 'holds', 'reactor', 'command', 'core']:
    text = (ROOT / f'tables/sentinel/{name}.tab').read_text(encoding='cp1252')
    encounters.update(s for s in re.findall(r'"([^"\n]*)"', text) if ' Points)' in s)

HARNESS = base[:base.index('int main(int argc')] + '\nstatic const char *encounters[]={\n' + ',\n'.join(json.dumps(s) for s in sorted(encounters)) + r'''
};
static int encounter_count;
static void count_monster(const char *name,const int *stats,int n,void *context) {
    const PACK_PROFILE *p=pack_monster(name);
    assert(p && stats[7]>0 && !strncmp(p->id,"sentinel:",9));
    encounter_count+=n;
}
static void crc_fix(unsigned char *bytes,size_t size) {
    unsigned long crc=0xffffffffUL; size_t i; int j;
    for(i=16;i<size;i++) { crc^=bytes[i]; for(j=0;j<8;j++) crc=(crc>>1)^((crc&1)?0xedb88320UL:0); }
    crc=~crc; for(j=0;j<4;j++) bytes[12+j]=(unsigned char)(crc>>(8*j));
}
static void validate_encounters(void) {
    int i,expected,n; const char *s,*end; char *contents; _UBYTE *lines[2];
    for(i=0;i<sizeof(encounters)/sizeof(encounters[0]);i++) {
        lines[0]=(_UBYTE*)encounters[i]; lines[1]=NULL; encounter_count=0;
        expected=0; s=encounters[i];
        do { while(*s==' ') s++; n=isdigit((unsigned char)*s)?atoi(s):1; expected+=n;
             end=strchr(s,','); if(end) s=end+1; } while(end);
        contents=(char*)room_contents_text(lines); assert(contents);
        if(strstr(contents,"No matching monster reference")) fprintf(stderr,"Unresolved: %s\n%s\n",encounters[i],contents);
        assert(!strstr(contents,"No matching monster reference")); free(contents);
        assert(!room_monsters(lines,count_monster,NULL)); assert(encounter_count==expected);
    }
}
int main(int argc,char **argv) {
    int deck,seed,i,j,room=-1,n,total; char quest[64],error[256]; HWND dialog;
    GAME_DATA d,*decoded; unsigned char *bytes,*again; size_t size,size2,old_size;
    CHARACTER_PACK *pack; HERO preserved; _UBYTE **saved_text;
    _UBYTE *leaders[]={"2 Farseer Ulvaneth of the Sentinel Host (120 Points)",NULL};
    _UBYTE *troops[]={"2 Chaos Androids (30 Points)",NULL};
    assert(mem_alloc(100,100,1000,4096)); heap_clear(); assert(SetCurrentDirectoryA(argv[1]));
    assert(!strcmp(pack_legacy_save()->id,"fantasy"));
    assert(pack_find_monster(pack_fantasy(),"  GOBLIN--ARCHERS  ")==pack_find_monster(pack_fantasy(),"goblin archer"));
    assert(pack_find_monster(pack_fantasy(),"goblin archer"));
    assert(pack_select_campaign("sentinel1.tab",error));
    assert(!strcmp(pack_current()->id,"sentinel"));
    dialog=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGPARTY),NULL,party_proc,0); assert(dialog);
    assert(SendDlgItemMessage(dialog,GPCLASS,CB_GETCOUNT,0,0)==pack_current()->hero_count);
    { char label[100];
      assert(SendDlgItemMessageA(dialog,GPCLASS,CB_GETLBTEXT,0,(LPARAM)label)>=0);
      assert(!strcmp(label,"Space Marine Commander"));
    }
    DestroyWindow(dialog);
    assert(pack_start_dungeon());
    assert(!strcmp(pack_current()->id,"sentinel")); validate_encounters();
    for(i=0;i<pack_current()->hero_count;i++) assert(pack_current()->heroes[i].melee.critical==12 && pack_current()->heroes[i].melee.fumble==1 && !pack_current()->heroes[i].melee.diagonal);
    assert(pack_monster("Orcs")==pack_monster("Ork"));
    /* Every truncation is rejected before any active state changes. */
    pack=pack_clone(pack_current()); assert(pack);
    for(i=0;i<(int)pack->size;i++) assert(!pack_decode(pack->bytes,i));
    pack->bytes[0]^=1; assert(!pack_decode(pack->bytes,pack->size)); pack_free(pack);
    for(deck=1;deck<=5;deck++) {
        total=0;
        sprintf(quest,"sentinel%d.tab",deck);
        assert(pack_select_campaign(quest,error)); assert(read_table(quest,NULL));
        for(seed=1;seed<=8;seed++) {
            new_rand(1000*deck+seed); if(!makemap(52,32,26,16,North)) continue;
            game_new_dungeon(quest); assert(!strcmp(pack_current()->id,"sentinel"));
            for(i=0;i<MAX_PICE;i++) if(strchr("SNHXAQBM",Pice[i].type) && Pice[i].type) {
                char *text=(char*)room_contents_text(Pice[i].text);
                if(text && strstr(text,"No matching monster reference")) fprintf(stderr,"Deck %d seed %d: %s\n",deck,seed,text);
                assert(!text || !strstr(text,"No matching monster reference")); free(text);
                game_fog()[i]=1; room=i;
            }
            monsters_reveal(); n=monsters.count; total+=n;
            monsters_reveal(); assert(monsters.count==n);
            for(i=0;i<monsters.count;i++) assert(!strncmp(monsters.tokens[i].profile_id,"sentinel:",9));
        }
        assert(room>=0 && total>0);
        printf("PASS: Sentinel deck %d generation and repeated reveals\n",deck);
    }
    /* Actual Heroes & Reserve creation: all marine loadouts and Commander. */
    hero_count=0; selected=-1;
    dialog=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FGPARTY),NULL,party_proc,0); assert(dialog);
    assert(SendDlgItemMessage(dialog,GPCLASS,CB_GETCOUNT,0,0)==pack_current()->hero_count);
    { char label[100];
      assert(SendDlgItemMessageA(dialog,GPCLASS,CB_GETLBTEXT,0,(LPARAM)label)>=0);
      assert(!strcmp(label,"Space Marine Commander"));
      assert(SendDlgItemMessageA(dialog,GPCLASS,CB_GETLBTEXT,1,(LPARAM)label)>=0);
      assert(!strcmp(label,"Space Marine - Bolter"));
    }
    for(i=0;i<pack_current()->hero_count;i++) {
        SendDlgItemMessage(dialog,GPCLASS,CB_SETCURSEL,i,0); SendMessage(dialog,WM_COMMAND,GPADD,0);
        assert(heroes[i].stats[3]>=10 && !strncmp(heroes[i].profile_id,"sentinel:",9));
        assert(heroes[i].ranged.kind==4 && heroes[i].ranged.dice>0 && heroes[i].melee.dice>0);
    }
    DestroyWindow(dialog); heroes[0].wounds=2; preserved=heroes[0];
    /* Hidden rooms use the saved pack even after the registry changes. */
    memset(&monsters,0,sizeof(monsters)); memset(game_fog(),0,MAX_PICE);
    saved_text=Pice[room].text; Pice[room].text=troops;
    assert(capture(&d)); assert(game_encode(&d,&bytes,&size)); free(d.cells);
    assert(pack_activate(pack_fantasy()));
    decoded=game_decode(bytes,size); assert(decoded && !strcmp(decoded->pack->id,"sentinel"));
    assert(!memcmp(&decoded->heroes[0],&preserved,sizeof(HERO)));
    assert(game_encode(decoded,&again,&size2) && size==size2 && !memcmp(bytes,again,size)); free(again);
    assert(pack_activate(decoded->pack));
    game_fog()[room]=1; monsters_reveal(); assert(monsters.count==2);
    monsters_reveal(); assert(monsters.count==2);
    assert(monsters.tokens[0].stats[3]==12 && monsters.tokens[0].ranged.dice==5);
    game_data_free(decoded); free(bytes);
    /* Named leaders spawn once, and cannot return after death or save/load. */
    memset(&monsters,0,sizeof(monsters)); Pice[room].text=leaders;
    monsters_reveal(); assert(monsters.count==1 && monsters.tokens[0].unique);
    assert(!strcmp(monsters.tokens[0].character_id,"sentinel:farseer-ulvaneth-of-the-sentinel-host"));
    monsters.tokens[0].wounds=0; monster_kill(0); assert(monsters.dead_count==1);
    monsters.seen[room]=0; monsters_reveal(); assert(monsters.count==1);
    assert(capture(&d)); assert(game_encode(&d,&bytes,&size)); free(d.cells);
    decoded=game_decode(bytes,size); assert(decoded && decoded->monsters.dead_count==1);
    game_data_free(decoded); free(bytes); Pice[room].text=saved_text;
    /* Load through the actual game lifecycle with the on-disk pack removed. */
    memset(&monsters,0,sizeof(monsters)); memset(game_fog(),0,MAX_PICE); Pice[room].text=troops;
    snprintf(recovery_path,sizeof(recovery_path),"%s\\pack-save.hqg",argv[2]);
    assert(write_game(recovery_path,0)); Pice[room].text=saved_text;
    assert(DeleteFileA("sentinel\\characters.hqp"));
    assert(!pack_select_campaign("sentinel1.tab",error));
    assert(!strcmp(pack_current()->id,"sentinel"));
    assert(pack_activate(pack_fantasy())); assert(game_load(2));
    assert(!strcmp(pack_current()->id,"sentinel") && !memcmp(&heroes[0],&preserved,sizeof(HERO)));
    game_fog()[room]=1; monsters_reveal(); assert(monsters.count==2);
    monsters_reveal(); assert(monsters.count==2);
    /* Legacy v6 fantasy migration is independent of the currently active pack. */
    assert(pack_activate(pack_fantasy())); hero_count=1; hero_defaults(&heroes[0],2,1);
    memset(&monsters,0,sizeof(monsters)); memset(game_fog(),0,MAX_PICE);
    assert(capture(&d)); assert(game_encode(&d,&bytes,&size)); free(d.cells);
    old_size=size-pack_extension(&d); bytes[8]=6; crc_fix(bytes,old_size);
    assert(pack_start_dungeon()); decoded=game_decode(bytes,old_size);
    assert(decoded && !strcmp(decoded->pack->id,"sentinel"));
    assert(!strcmp(decoded->heroes[0].profile_id,"sentinel:space-marine-heavy-bolter") && decoded->heroes[0].ranged.kind==1);
    game_data_free(decoded); free(bytes);
    /* Selecting another campaign never mutates an existing party. */
    heroes[0]=preserved; assert(pack_select_campaign("sonne2.tab",error)); game_new_dungeon("Fantasy next map");
    assert(!strcmp(pack_current()->id,"fantasy") && !memcmp(&heroes[0],&preserved,sizeof(HERO)));
    game_shutdown(); pack_shutdown(); mem_freeall();
    puts("PASS: all Sentinel encounter literals, native party presets, unique deaths, pack snapshots, v6 migration, and mixed-party preservation.");
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='hq-pack-test-') as tmp:
    work = Path(tmp)
    shutil.copytree(ROOT/'tables', work/'tables')
    (work / 'check.c').write_text(HARNESS)
    exclude = {'main.obj', 'maindm.obj', 'winddm.obj', 'game.obj'}
    objects = [str(p) for p in (ROOT / 'obj/msvc').glob('*.obj') if p.name not in exclude]
    subprocess.run(['cl', '/nologo', '/D_CRT_SECURE_NO_WARNINGS', '/I'+str(ROOT),
                    '/I'+str(ROOT/'my_lib'), '/I'+str(ROOT/'my_lib/windows'), '/I'+str(ROOT/'rsh'),
                    'check.c', '/Fe:check.exe', *objects, str(ROOT/'obj/msvc/menu.res'), str(ROOT/'obj/msvc/grafic.res'),
                    'user32.lib', 'gdi32.lib', 'shell32.lib', 'comdlg32.lib', 'version.lib', 'winspool.lib', 'ole32.lib', 'advapi32.lib'], cwd=work, check=True)
    subprocess.run([str(work/'check.exe'), str(work/'tables'), str(work)], cwd=work, check=True, timeout=90)
