"""Exercise the actual C builder and shipped parser. Run in an x86 MSVC shell."""
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / 'bin/hq_map.exe'

HARNESS = r'''
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
static const char *application;
static DWORD builder_exe(HMODULE m, LPSTR out, DWORD size) {
    (void)m; if (strlen(application)>=size) return 0;
    strcpy(out,application); return (DWORD)strlen(out);
}
#define GetModuleFileNameA builder_exe
#include "campaign.c"
#undef assert
#define assert(x) do { if(!(x)) { fprintf(stderr,"Assertion failed at %d: %s\n",__LINE__,#x); exit(3); } } while(0)
int main(int argc,char **argv) {
    Builder *b=(Builder*)calloc(1,sizeof(Builder)); int i,j,k,index,ok;
    application=argv[1]; strcpy(b->root,argv[2]);
    assert(scan_directory(b,"",0));
    b->base=find_file(b,argv[3]); assert(b->base>=0);
    if(argc>5 && !strcmp(argv[5],"ui")) {
        HWND dialog; RECT client,control; POINT corner; int ids[]={CBBASE,CBTITLE,CBSLUG,CBDESC,CBCREATE,CBCHECK,IDCANCEL};
        strcpy(b->title,argv[3]);
        dialog=CreateDialogParamA(GetModuleHandle(NULL),MAKEINTRESOURCEA(FCB),NULL,builder_proc,(LPARAM)b);
        assert(dialog!=NULL); assert(b->base==find_file(b,argv[3]));
        assert(combo_value(dialog,CBCATEGORY)==find_file(b,"faces\\room1_1.tab"));
        assert(combo_value(dialog,CBCATEGORY+1)==find_file(b,"faces\\chaos.tab"));
        assert(SendDlgItemMessage(dialog,CBBASE,CB_GETCOUNT,0,0)>30);
        for(i=0;i<CAT_COUNT;i++) assert(SendDlgItemMessage(dialog,CBCATEGORY+i,CB_GETCOUNT,0,0)>0);
        if(argc>6) {
            HDC screen=GetDC(NULL),dc=CreateCompatibleDC(screen); HBITMAP bitmap,old;
            BITMAPINFO info; BITMAPFILEHEADER file; void *bits; FILE *fp; RECT rect; int w,h;
            SetDlgItemTextA(dialog,CBTITLE,"The Goblin King's Fortress"); SetDlgItemTextA(dialog,CBSLUG,"goblinfort");
            SetDlgItemTextA(dialog,CBDESC,"Beneath the ruined fortress, a goblin king has gathered\r\nhis followers around a stolen relic...");
            combo_select(dialog,CBCATEGORY+1,find_file(b,"dark\\orc1.tab"));
            SetWindowPos(dialog,NULL,-10000,-10000,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
            ShowWindow(dialog,SW_SHOWNOACTIVATE); UpdateWindow(dialog);
            GetWindowRect(dialog,&rect); w=rect.right-rect.left; h=rect.bottom-rect.top;
            memset(&info,0,sizeof(info)); info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
            info.bmiHeader.biWidth=w; info.bmiHeader.biHeight=-h; info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32;
            bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&bits,NULL,0); assert(bitmap);
            old=(HBITMAP)SelectObject(dc,bitmap);
            PrintWindow(dialog,dc,0);
            memset(&file,0,sizeof(file)); file.bfType=0x4d42; file.bfOffBits=sizeof(file)+sizeof(info.bmiHeader); file.bfSize=file.bfOffBits+w*h*4;
            fp=fopen(argv[6],"wb"); assert(fp); fwrite(&file,sizeof(file),1,fp); fwrite(&info.bmiHeader,sizeof(info.bmiHeader),1,fp); fwrite(bits,w*h*4,1,fp); fclose(fp);
            SelectObject(dc,old); DeleteObject(bitmap); DeleteDC(dc); ReleaseDC(NULL,screen);
        }
        SetDlgItemTextA(dialog,CBTITLE,"My edited title"); SetDlgItemTextA(dialog,CBDESC,"My edited description");
        CheckDlgButton(dialog,CBFURNISH,BST_UNCHECKED);
        SendMessage(dialog,WM_COMMAND,CBREFRESH,0); read_form(dialog,b);
        assert(!strcmp(b->title,"My edited title")); assert(!strcmp(b->description,"My edited description")); assert(!b->furnish);
        GetClientRect(dialog,&client);
        for(i=0;i<sizeof(ids)/sizeof(ids[0]);i++) {
            assert(GetDlgItem(dialog,ids[i])!=NULL); GetWindowRect(GetDlgItem(dialog,ids[i]),&control);
            corner.x=control.right; corner.y=control.bottom; ScreenToClient(dialog,&corner);
            assert(corner.x<=client.right && corner.y<=client.bottom);
        }
        DestroyWindow(dialog); clear_catalog(b); free(b); return 0;
    }
    for(i=0;i<CAT_COUNT;i++) b->selected[i]=-1;
    for(j=0;j<b->files[b->base].nincludes;j++) {
        index=find_file(b,b->files[b->base].includes[j]); k=effective_mask(b,index,0);
        for(i=0;i<CAT_COUNT;i++) if(k & (1<<i)) b->selected[i]=index;
    }
    if(argc>5 && strcmp(argv[5],"-")) b->selected[1]=find_file(b,argv[5]);
    strcpy(b->title,"A new campaign"); strcpy(b->slug,argv[4]); strcpy(b->description,"A new adventure.\r\nIndependent tables."); b->furnish=1;
    if(argc>6) b->furnish=atoi(argv[6]);
    if(argc>7) {
        b->selected[2]=find_file(b,"oath\\hazard.tab");
        b->selected[4]=find_file(b,"standard\\trap.tab");
        b->selected[5]=find_file(b,"dark\\treasure.tab");
    }
    ok=run_campaign(b,0);
    if(ok) ok=run_campaign(b,1);
    if(!ok) fprintf(stderr,"%s\n",b->error);
    clear_catalog(b); free(b); return ok ? 0 : 1;
}
'''

with tempfile.TemporaryDirectory(prefix='hq-builder-test-') as tmp:
    tmp = Path(tmp)
    harness = tmp / 'check.c'
    harness.write_text(HARNESS)
    runner = tmp / 'check.exe'
    subprocess.run(['cl', '/nologo', '/W3', '/D_CRT_SECURE_NO_WARNINGS',
                    f'/I{ROOT}', f'/I{ROOT / "my_lib"}', f'/I{ROOT / "my_lib/windows"}', str(ROOT / 'pack.c'), str(harness), f'/Fe{runner}',
                    str(ROOT / 'obj/msvc/menu.res'), 'user32.lib', 'gdi32.lib'], cwd=tmp, check=True)
    tables = tmp / 'tables'
    shutil.copytree(ROOT / 'tables', tables)
    original = {p.relative_to(tables): p.read_bytes() for p in tables.rglob('*') if p.is_file()}
    subprocess.run([str(runner), str(EXE), str(tables), 'faces1_1.tab', 'uitest', 'ui', str(ROOT / 'obj/campaign-builder-preview.bmp')], check=True, timeout=15)

    def create(base, slug, monster='-', furnish=1, success=True, contains=None, mixed=False):
        p = subprocess.run([str(runner), str(EXE), str(tables), base, slug, monster, str(furnish)] + (['mixed'] if mixed else []),
                           capture_output=True, text=True, timeout=30)
        assert (p.returncode == 0) == success, (base, slug, p.returncode, p.stdout, p.stderr)
        if contains:
            assert contains in p.stderr, p.stderr
        assert not list(tables.glob('.hq-builder-*')), 'staging directory leaked'

    create('faces1_1.tab', 'goblinfort', r'dark\orc1.tab')
    create('faces1_1.tab', 'barefort', r'dark\orc1.tab', furnish=0)
    create('goblinfort.tab', 'copyfort')
    create('sentinel5.tab', 'sentinelcopy')
    assert (tables/'sentinelcopy/characters.hqp').read_bytes() == original[Path('sentinel/characters.hqp')]
    assert ';character-pack sentinelcopy\\characters.hqp' in (tables/'sentinelcopy.tab').read_text()
    create('sentinelcopy.tab', 'sentinelcopyagain')
    create('faces1_1.tab', 'wrongpack', r'sentinel\core.tab', success=False, contains='different character pack')
    create('ritual1.tab', 'ritualcopy')
    create('terror4.tab', 'terrorcopy')
    create('sonne2.tab', 'lichemaster', r'terror\beastman.tab', mixed=True)
    supports = list((tables / 'lichemaster/_support').glob('*.tab'))
    assert supports, 'Missing generated support files'
    wight = next(p.read_text() for p in supports if '\nWight\n' in p.read_text())
    assert 'sonne\\hazard2.tab' in wight and 'Wight (WS 6, T 5, W 3)' in wight
    assert 'Hazards-Matrix' not in wight, 'Copied unwanted hazard category'
    assert not (tables / 'lichemaster/sonne/hazard2.tab').exists()
    create('lichemaster.tab', 'lichecopy')
    # Every shipped entry which the native parser accepts can be cloned.
    verified = 0
    for i, base in enumerate(sorted(tables.glob('*.tab'))):
        if base.relative_to(tables) not in original:
            continue
        p = subprocess.run([str(EXE), '--check-campaign', base.name, 'baseline-errors.txt'], cwd=tables, timeout=15)
        if p.returncode == 0:
            slug = 'campaign' + chr(97 + i // 26) + chr(97 + i % 26)
            create(base.name, slug)
            verified += 1
    print(f'Cloned all {verified} valid shipped campaign entries.')
    create('faces1_1.tab', 'goblinfort', success=False, contains='already exists')
    for slug in ['../escape', 'CON', 'com', 'quest1', 'quest_', 'bad name']:
        # 'com' alone is a legal Windows name.
        if slug == 'com':
            continue
        create('faces1_1.tab', slug, success=False)

    # Broken dice coverage must be rejected by the real parser.
    (tables / 'dark/orc1.tab').write_text('Wandering-Monsters\n(1D12\n1-1 "Goblin"\n)\n')
    create('faces1_1.tab', 'brokenfort', r'dark\orc1.tab', success=False, contains='do not work together')
    assert not (tables / 'brokenfort').exists()
    (tables / 'dark/orc1.tab').write_bytes(original[Path('dark/orc1.tab')])

    # A new runtime-discovered component with an explicit nested include.
    (tables / 'custom').mkdir()
    (tables / 'custom/monsters.tab').write_text('include custom\\goblins.tab\n')
    (tables / 'custom/goblins.tab').write_bytes(original[Path('dark/orc1.tab')])
    create('faces1_1.tab', 'nestedfort', r'custom\monsters.tab')
    (tables / 'custom/monsters.tab').write_text('include custom\\monsters.tab\n')
    create('faces1_1.tab', 'cyclefort', r'custom\monsters.tab', success=False, contains='cycle')
    (tables / 'custom/monsters.tab').write_text('include ..\\outside.tab\n')
    create('faces1_1.tab', 'escapefort', r'custom\monsters.tab', success=False)
    shutil.rmtree(tables / 'custom')

    # All original files remain byte-for-byte unchanged.
    for relative, content in original.items():
        assert (tables / relative).read_bytes() == content, relative

    # Remove the sources; generated campaigns must stand on their own.
    for relative in original:
        (tables / relative).unlink()
    for slug in ['goblinfort', 'barefort', 'copyfort', 'ritualcopy', 'terrorcopy', 'nestedfort', 'lichemaster', 'lichecopy', 'sentinelcopy', 'sentinelcopyagain']:
        p = subprocess.run([str(EXE), '--check-campaign', slug + '.tab', 'check-errors.txt'],
                           cwd=tables, timeout=15)
        assert p.returncode == 0, (slug, (tables / 'check-errors.txt').read_text())
    print('Campaign builder: discovery, cross-campaign mixing, independent copies, nested includes,')
    print('parser rejection, furnishings toggle, source preservation and collision checks passed.')
