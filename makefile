#
# makefile.bcc - top level makefile for Windows/Borland C
#

#
# TOP directory of source tree
#
TOP=		P:\hero
SRC_DIR=	$(TOP)

# make SYSTEM=WIN16  builds 16bit Windows version (default)
# make SYSTEM=WIN32  builds 32bit Windows version
# make SYSTEM=DOS    builds 16bit DOS version
#
# make COMPILER=BORLANDC set up commands for Borland C 4.52 (default)
# make COMPILER=MSVC20   set up commands for Visual C 2.0
# make COMPILER=MSVC40   set up commands for Visual C 4.0

# where to put executables
BIN_DIR=	$(TOP)\bin
# where to put objects
OBJ_WIN16_DIR=	$(TOP)\obj\win16
OBJ_WIN32_DIR=	$(TOP)\obj\win32
OBJ_DOS_DIR=	$(TOP)\obj\dos
OBJ_DLL16_DIR=	$(TOP)\obj\dll16
OBJ_DLL32_DIR=	$(TOP)\obj\dll32

#
# supported systems
#
SYSTEMS=	WIN16 DOS WIN32 DLL16 DLL32

#
# supported compilers
#
COMPILERS=	BORLANDC MSVC20 MSVC40

#----------------------------------------------------------------------------
# validate or select target operating system
#----------------------------------------------------------------------------

!ifndef SYSTEM
!ifdef WIN32
SYSTEM  = WIN32
!elseif defined(CON32)
SYSTEM  = CON32
!elseif defined(WIN16)
SYSTEM  = WIN16
!elseif defined(DOS16)
SYSTEM  = DOS
!else #default
SYSTEM  = WIN32
!endif
!endif

#----------------------------------------------------------------------------
# validate or select compiler
#----------------------------------------------------------------------------

!ifndef COMPILER
COMPILER= 	BORLANDC
#COMPILER= 	MSVC40
!endif

#----------------------------------------------------------------------------
# validate or select memory model
#----------------------------------------------------------------------------

!ifndef MODEL
MODEL=		l
!endif

#----------------------------------------------------------------------------
#
# Borland C tools
#
!if "$(COMPILER)"=="BORLANDC"
#  Borland C 4.52
BC_DIR=         p:\bc\bc45
CC_LIB_DIR=	$(BC_DIR)\lib
CC_INC_DIR=	$(BC_DIR)\include
CC_BIN_DIR=	$(BC_DIR)\bin

CFG16=		$(TOP)\bccw16.cfg
CC16=		$(CC_BIN_DIR)\bcc +$(CFG16) -c -o$@
CFG32=		$(TOP)\bccw32.cfg
CC32=		$(CC_BIN_DIR)\bcc32 +$(CFG32) -c -o$@
CFGDOS=		$(TOP)\bccdos.cfg
CCDOS=		$(CC_BIN_DIR)\bcc +$(CFGDOS) -c -o$@
CFGDLL16=	$(TOP)\bccd16.cfg
CCDLL16=	$(CC_BIN_DIR)\bcc +$(CFGDLL16) -c -o$@
CFGDLL32=	$(TOP)\bccd32.cfg
CCDLL32=	$(CC_BIN_DIR)\bcc32 +$(CFGDLL32) -c -o$@
TLINK16=	$(CC_BIN_DIR)\tlink
TLINK32=	p:\bc\bc\bin\tlink32
#TLINK32=	$(CC_BIN_DIR)\tlink32
TLIB=		$(CC_BIN_DIR)\tlib $@
STRIP16=	$(CC_BIN_DIR)\tdstrip
STRIP32=	$(CC_BIN_DIR)\tdstrp32
IMPLIB=		$(CC_BIN_DIR)\implib
BRC16=		$(CC_BIN_DIR)\brc -31 -D__BORLANDC__=0x452
BRC32=		$(CC_BIN_DIR)\brc32 -D__WIN32__ -D__BORLANDC__=0x452 -w32
HC31=		$(CC_BIN_DIR)\hc31

CC_DEFINES=	-D__STDC__=1

# /C = case-sensitive library
# /0 = purge comment records
# /E = create extended dictionary
# /P = library page size
TLIBFLAGS=	/C /0 /E /P64

# -x = no link map
# -s = full link map
# -v = full symbolic debug information
# -c = case sensitive symbols
# -A:64 = segment alignment
# -L = library search path
# -m = map file with publics
# -s = detailed map of segments
LFLAGS0=	-c -x -v -A:64 -L$(CC_LIB_DIR)

# -C = case sensitive exports & imports
# -E = use extended dictionary
# -Twe = create windows executable
# -Tde = create dos executable
# -Tpe = create 32-bit protected mode executable
# -aa = create windows application
# -ap = create easywin application
# -P = pack code segments
# -ye = expanded memory swapping
# -yx = extended memory swapping
LFLAGS16=	$(LFLAGS0) -C -E -Twe /P=65000 -yx
LFLAGSDLL16=	$(LFLAGS0) -C -E -Twd /P=65000 -ye
LFLAGS32=	$(LFLAGS0) -Tpe -aa
LFLAGSDLL32=	$(LFLAGS0) -Tpd -aa
LFLAGSDOS=	$(LFLAGS0) -C -E -Tde /P=65000 -yx
!endif

!if "$(COMPILER)"=="MSVC20"
# Microsoft Visual C 2.0
VC_DIR=         c:\msvc20
CC_LIB_DIR=	$(VC_DIR)\lib
CC_INC_DIR=	$(VC_DIR)\include
CC_BIN_DIR=	$(VC_DIR)\bin

CFG16=		$(TOP)\vccw16.cfg
CC16=		$(CC_BIN_DIR)\cl @$(CFG16) -c -Fo$@
CFG32=		$(TOP)\vccw32.cfg
CC32=		$(CC_BIN_DIR)\cl @$(CFG32) -c -Fo$@
CFGDOS=		$(TOP)\vccdos.cfg
CCDOS=		$(CC_BIN_DIR)\cldos @$(CFGDOS) -c -Fo$@
TLINK16=	tlink
TLINK32=	tlink
TLIB=		$(CC_BIN_DIR)\lib /OUT:$@
STRIP16=	tdstrip
STRIP32=	$(CC_BIN_DIR)\cvpack /strip
IMPLIB=		implib
BRC16=		brc
BRC32=		$(CC_BIN_DIR)\rc -D__WIN32__
HC31=		hc31

CC_DEFINES=	

TO=		-Fo$@

TLIBFLAGS=	/C /0 /E /P64

LFLAGS0=	-x -v -c -A:64 -L$(CC_LIB_DIR)

LFLAGS16=	$(LFLAGS0) -C -E -Twe /P=65000 -ye
LFLAGS32=	$(LFLAGS0) -Tpe -aa
LFLAGSDOS=	$(LFLAGS0) -C -E -Tde /P=65000 -yx
!endif


!if "$(COMPILER)"=="MSVC40"
# Microsoft Visual C 4.0
VC_DIR=         i:\msdev
CC_LIB_DIR=	$(VC_DIR)\lib
CC_INC_DIR=	$(VC_DIR)\include
CC_BIN_DIR=	$(VC_DIR)\bin

CFG16=		$(TOP)\vccw16.cfg
CC16=		$(CC_BIN_DIR)\cl @$(CFG16) -c -Fo$@
CFG32=		$(TOP)\vccw32.cfg
CC32=		$(CC_BIN_DIR)\cl /nologo @$(CFG32) -c -Fo$@
CFGDOS=		$(TOP)\vccdos.cfg
CCDOS=		$(CC_BIN_DIR)\cl /nologo @$(CFGDOS) -c -Fo$@
TLINK16=	$(CC_BIN_DIR)\link
TLINK32=	$(CC_BIN_DIR)\link
TLIB=		$(CC_BIN_DIR)\lib /OUT:$@
STRIP16=	tdstrip
STRIP32=	$(CC_BIN_DIR)\cvpack /strip
IMPLIB=		implib
BRC16=		brc
BRC32=		$(CC_BIN_DIR)\rc -D_MSC_VER=1000
HC31=		hc31
RC_INCLUDES=	-I. -I$(TOP)

CC_DEFINES=	

TO=		-Fo$@

TLIBFLAGS=	

LFLAGS0=	/debug /out:$@ /machine:I386

LFLAGS16=	$(LFLAGS0) -C -E -Twe /P=65000 -ye
LFLAGS32=	$(LFLAGS0) /subsystem:windows
LFLAGSDOS=	$(LFLAGS0) /subsystem:console
!endif



UDO=		c:\bin\udo.exe
RM=		del
CP=		copy /y

# Swapping `make' out of memory makes linking much faster.

# .swap


#
# Directories
#

BIN_DIR=	$(TOP)\bin
RSH_DIR=	$(SRC_DIR)\rsh

MYLIB_DIR=	$(TOP)\my_lib
SYSPORTAB_DIR=	$(MYLIB_DIR)\windows

DELREG=		$(BIN_DIR)\delreg.com

#
#Options
#

INCLUDES=	-I. -I$(MYLIB_DIR) -I$(SYSPORTAB_DIR) -I$(CC_INC_DIR)

DEFINES=	$(CC_DEFINES) \
		-DSTDC_HEADERS=1 \
		-DHAVE_DIRENT_H=1 \
		-DSTRICT=1

SMALL=		s

####################################################################

!if "$(SYSTEM)"=="DOS"
CFGFILE=	$(CFGDOS)
CCC=		$(CCDOS)
OBJ_DIR=	$(OBJ_DOS_DIR)^\
TLINK=		$(TLINK16)
LFLAGS=		$(LFLAGSDOS)
TARGET=		
STRIP=		$(STRIP16)
!if "$(MODEL)"=="$(SMALL)"
LIBS=		fp87.lib+math$(MODEL).lib+c$(MODEL).lib
!elseif "$(COMPILER)"=="BORLANDC"
LIBS=		math$(MODEL).lib+emu.lib+c$(MODEL).lib
!elseif "$(COMPILER)"=="MSVC40"
LIBS=		libc.lib kernel32.lib user32.lib
!endif
DEP_LIBS=	$(GEMLIB)
C0OBJ=		$(CC_LIB_DIR)\c0$(MODEL).obj

!elseif "$(SYSTEM)"=="WIN16"
CFGFILE=	$(CFG16)
CCC=		$(CC16)
OBJ_DIR=	$(OBJ_WIN16_DIR)^\
TLINK=		$(TLINK16)
LFLAGS=		$(LFLAGS16)
#TARGET=		16
TARGET=		
STRIP=		$(STRIP16)
DEP_LIBS=	$(GEMLIB)
LIBS=		import.lib+mathw$(MODEL).lib+cw$(MODEL).lib
C0OBJ=		$(CC_LIB_DIR)\c0w$(MODEL).obj
BRC=		$(BRC16)

!elseif "$(SYSTEM)"=="WIN32"
CFGFILE=	$(CFG32)
CCC=		$(CC32)
OBJ_DIR=	$(OBJ_WIN32_DIR)^\
TLINK=		$(TLINK32)
LFLAGS=		$(LFLAGS32)
#TARGET=		32
TARGET=		
STRIP=		$(STRIP32)
!if "$(COMPILER)"=="BORLANDC"
DEP_LIBS=	$(GEMLIB)
LIBS=		import32.lib+ole2w32.lib+cw32.lib
!endif
!if "$(COMPILER)"=="MSVC40"
LIBS=		libc.lib kernel32.lib user32.lib gdi32.lib shell32.lib comdlg32.lib version.lib
!endif
C0OBJ=		$(CC_LIB_DIR)\c0w32.obj
BRC=		$(BRC32)

!elseif "$(SYSTEM)"=="DLL16"
CFGFILE=	$(CFGDLL16)
CCC=		$(CCDLL16)
OBJ_DIR=	$(OBJ_DLL16_DIR)^\
TLINK=		$(TLINK16)
LFLAGS=		$(LFLAGSDLL16)
TARGET=		
STRIP=		$(STRIP16)
DEP_LIBS=	$(GEMLIB)
LIBS=		import.lib+cw$(MODEL).lib
C0OBJ=		$(CC_LIB_DIR)\c0d$(MODEL).obj
BRC=		$(BRC16)

!elseif "$(SYSTEM)"=="DLL32"
CFGFILE=	$(CFGDLL32)
CCC=		$(CCDLL32)
OBJ_DIR=	$(OBJ_DLL32_DIR)^\
TLINK=		$(TLINK32)
LFLAGS=		$(LFLAGSDLL32)
TARGET=		32
STRIP=		$(STRIP32)
DEP_LIBS=	$(GEMLIB)
LIBS=		import32.lib+cw32.lib
C0OBJ=		$(CC_LIB_DIR)\c0d32.obj
BRC=		$(BRC32)

!else
! error $(SYSTEM) system not supported
!endif

#.PATH.obj=	$(OBJ_DIR)
#.PATH.res=	$(OBJ_DIR)
#.PATH.lib=	$(OBJ_DIR)

#
# command for recursive makes
#
RUNMAKE=	$(MAKE) -N TOP=$(TOP)
RMAKE=		$(RUNMAKE) SYSTEM=$(SYSTEM) COMPILER=$(COMPILER)

#.SUFFIXES: .obj .c


.c.obj:
		$(CCC) $(EXTRA_DEFINES) {$? }



GEMLIB=		$(OBJ_DIR)gem.lib
DBLIBDIR=	$(OBJ_DIR)ctreedir.lib
DBLIBSRV=	$(OBJ_DIR)ctreesrv.lib
DBDLL=		$(BIN_DIR)\ctsrv$(TARGET).dll
DBNETDLL=	$(BIN_DIR)\ctnsrv$(TARGET).dll

####################################################################

all::
		@

####################################################################

delreg::	$(DELREG)


$(DELREG):	$(OBJ_DOS_DIR)\delreg.obj $(CFGDOS)
!if "$(COMPILER)"=="BORLANDC"
		$(TLINK16) -x -Tdc $(CC_LIB_DIR)\c0t.obj+$(OBJ_DOS_DIR)\delreg.obj,$@,,$(CC_LIB_DIR)\ct.lib
!endif
!if "$(COMPILER)"=="MSVC40"
		$(TLINK16) $(LFLAGSDOS) /OUT:$@ $(OBJ_DOS_DIR)\delreg.obj
!endif

$(OBJ_DOS_DIR)\delreg.obj:	$(GENERIC_DIR)\delreg.c $(CFGDOS)
!if "$(COMPILER)"=="BORLANDC"
		$(CCDOS) -mt -f- $(GENERIC_DIR)\delreg.c
!endif
!if "$(COMPILER)"=="MSVC40"
		$(CCDOS) $(GENERIC_DIR)\delreg.c
!endif

####################################################################
#
#Compiler configuration files (Windows 16-bit executable)
#
$(TOP)\bccw16.cfg :
   copy << $@
$(INCLUDES)
$(DEFINES)
-WS # Windows Smart Callbacks
-m$(MODEL)
-R # Debug-Informationen in obj
-v # source debugging on
-vi # inline-Expandierung ein
-K # default char unsigned
-d- # doppelte Strings
-3 # 386-code
-a # Ausrichtung auf Byte
-Ff # automatische Far-Daten
-Ff=64 # Schwellenwert fuer FAR-Daten
-Z # Registeroptimierung
-O  # Spruenge optimieren
-Oe # automatische Register
-Og # redundante Ausdruecke
-Ol # Schleifen verdichten
-Ob # ueberfluessigen Code entfernen
-O-W # inc bp/dec nicht unterdruecken
-k  # Standard Stackframe ausschalten
-X # suppress auto dependencies
-i64 # bezeichnerlaenge
-g1 # stop after 1 warning
-wamb
-wamp
-wasm
-waus
-wbbf
-wbei
-wbig
-wccc
-wcln
-wcpt
-wdef
-wdpu
-wdup
-weas
-weff
-wext
-will
-winl
-wnak
-wncf
-wnci
-wnod
-wnsf
-wnst
-wnvf
-wobs
-wpar
-wpch
-wpia
-wpin
-wpre
-wpro
-wrch
-wret
-wrng
-wrvl
-wsig
-w-stu
-wstv
-wsus
-wucp
-wuse
-wvoi
-wzdi
<<NOKEEP


#
#Compiler configuration file (Windows 32-bit executable)
#
$(TOP)\bccw32.cfg :
   copy << $@
$(INCLUDES)
$(DEFINES)
-W # Win32 exe
-R # Debug-Informationen in obj
-v # source debugging on
-vi # inline-Expandierung ein
-K # default char unsigned
-d- # doppelte Strings
-3 # 386-code
-a2 # Ausrichtung auf Wort
-Z- # Registeroptimierung aus
-O  # Spruenge optimieren
-Oe # automatische Register
-Og # redundante Ausdruecke
-Ol # Schleifen verdichten
-Ob # ueberfluessigen Code entfernen
-O-W # inc bp/dec nicht unterdruecken
-k-  # Standard Stackframe ausschalten
-i64 # bezeichnerlaenge
-g1 # Stop after 1 warning
-X # suppress auto dependencies
-wamb
-wamp
-wasm
-wbbf
-wbei
-wbig
-wccc
-wcln
-wcpt
-wdef
-wdpu
-wdup
-weas
-weff
-wext
-will
-winl
-wnak
-wncf
-wnci
-wnod
-wnsf
-wnst
-wnvf
-wobs
-wpar
-wpch
-wpia
-wpin
-wpre
-wpro
-wrch
-wret
-wrng
-wrvl
-w-sig
-w-stu
-wstv
-wsus
-wucp
-wuse
-wvoi
-wzdi
<<NOKEEP


#
#Compiler configuration files (Windows 16-bit dll)
#
$(TOP)\bccd16.cfg :
   copy << $@
$(INCLUDES)
$(DEFINES)
-WD
-tWD # Windows dll
-m$(MODEL)
-R # Debug-Informationen in obj
-v # source debugging on
-vi # inline-Expandierung ein
# -H
# -H=orcs.csm
-K # default char unsigned
-d- # doppelte Strings
-3 # 386-code
-a # Ausrichtung auf Byte
-Ff # automatische Far-Daten
-Ff=64 # Schwellenwert fuer FAR-Daten
-Z # Registeroptimierung
-O  # Spruenge optimieren
-Oe # automatische Register
-Og # redundante Ausdruecke
-Ol # Schleifen verdichten
-Ob # ueberfluessigen Code entfernen
-O-W # inc bp/dec nicht unterdruecken
-k-  # Standard Stackframe ausschalten
-i64 # bezeichnerlaenge
-g1 # stop after 1 warning
-wamb
-wamp
-wasm
-waus
-wbbf
-wbei
-wbig
-wccc
-wcln
-wcpt
-wdef
-wdpu
-wdup
-weas
-weff
-wext
-will
-winl
-wnak
-wncf
-wnci
-wnod
-wnsf
-wnst
-wnvf
-wobs
-wpar
-wpch
-wpia
-wpin
-wpre
-wpro
-wrch
-wret
-wrng
-wrvl
-wsig
-w-stu
-wstv
-wsus
-wucp
-wuse
-wvoi
-wzdi
<<NOKEEP


#
#Compiler configuration file (Windows 32-bit dll)
#
$(TOP)\bccd32.cfg :
   copy << $@
$(INCLUDES)
$(DEFINES)
-WD # Win32 dll
-tWD
-R # Debug-Informationen in obj
-v # source debugging on
-vi # inline-Expandierung ein
-K # default char unsigned
-d- # doppelte Strings
-3 # 386-code
-a2 # Ausrichtung auf Wort
-Z- # Registeroptimierung aus
-O  # Spruenge optimieren
-Oe # automatische Register
-Og # redundante Ausdruecke
-Ol # Schleifen verdichten
-Ob # ueberfluessigen Code entfernen
-O-W # inc bp/dec nicht unterdruecken
-k-  # Standard Stackframe ausschalten
-i64 # bezeichnerlaenge
-g1 # Stop after 1 warning
-wamb
-wamp
-wasm
-wbbf
-wbei
-wbig
-wccc
-wcln
-wcpt
-wdef
-wdpu
-wdup
-weas
-weff
-wext
-will
-winl
-wnak
-wncf
-wnci
-wnod
-wnsf
-wnst
-wnvf
-wobs
-wpar
-wpch
-wpia
-wpin
-wpre
-wpro
-wrch
-wret
-wrng
-wrvl
-wsig
-w-stu
-wstv
-wsus
-wuse
-wucp
-wvoi
-wzdi
<<NOKEEP


#
#Compiler configuration file (DOS executable)
#
$(TOP)\bccdos.cfg :
   copy << $@
$(INCLUDES)
$(DEFINES)
-W- # DOS EXE
-m$(MODEL)
-f- # no floating point
-R # Debug-Informationen in obj
-v # source debugging on
-vi # inline-Expandierung ein
-K # default char unsigned
-d- # doppelte Strings
-3 # 386-code
-a # Ausrichtung auf Byte
-Ff # automatische Far-Daten
-Ff=64 # Schwellenwert fuer FAR-Daten
-Z- # Registeroptimierung aus
-O  # Spruenge optimieren
-Oe # automatische Register
-Og # redundante Ausdruecke
-Ol # Schleifen verdichten
-Ob # ueberfluessigen Code entfernen
-O-W # inc bp/dec nicht unterdruecken
-k-  # Standard Stackframe ausschalten
-i64 # bezeichnerlaenge
-g1 # stop after 1 warning
-wamb
-wamp
-wasm
-waus
-wbbf
-wbei
-wbig
-wccc
-wcln
-wcpt
-wdef
-wdpu
-wdup
-weas
-weff
-wext
-will
-winl
-wnak
-wncf
-wnci
-wnod
-wnsf
-wnst
-wnvf
-wobs
-wpar
-wpch
-wpia
-wpin
-wpre
-wpro
-wrch
-wret
-wrng
-wrvl
-wsig
-w-stu
-wstv
-wsus
-wucp
-wuse
-wvoi
-wzdi
<<NOKEEP




$(TOP)\vccw16.cfg :
   copy << $@
$(INCLUDES)
$(DEFINES)
-Og # enable global optimziation
-Oi # enable intrinsic functions
-Os # favor code space
-Oy # enable frame pointer omission
-O1 # minimize space
-Gs # disable stack checking
-G3 # optimize for 386
-Za # disable extensions
-Zp2 # pack structs on 2-byte boundary
-J # default char unsigned
-W3 # enable all warnings
-nologo # suppress copyright message
<<NOKEEP



$(TOP)\vccw32.cfg :
   copy << $@
$(INCLUDES)
$(DEFINES)
#-Og # enable global optimziation
#-Oi # enable intrinsic functions
#-Os # favor code space
#-Oy # enable frame pointer omission
-O1 # minimize space
# -GF # enable read-only string pooling
# -Gs # disable stack checking
#-G3 # optimize for 386
# -Gm # enable minimal rebuild
# -Gi # enable incremental compilation
#-Za # disable extensions
-Ze # enable extensions
# -Zi # generate debug info
-Zp2 # pack structs on 2-byte boundary
# -J # default char unsigned
-W3 # enable all warnings
-WX # treat warnings as errors
-ML # link with libc.lib
-nologo # suppress copyright message
<<NOKEEP


$(TOP)\vccdos.cfg :
   copy << $@
$(INCLUDES)
$(DEFINES)
-D__MSDOS__
-D__FLAT__
-Og # enable global optimziation
-Oi # enable intrinsic functions
-Os # favor code space
-Oy # enable frame pointer omission
-O1 # minimize space
-Gs # disable stack checking
-G3 # optimize for 386
#-Za # disable extensions
-Ze # enable extensions
#-Zi # generate debug info
-Zp2 # pack structs on 2-byte boundary
-J # default char unsigned
-W3 # enable all warnings
-WX # treat warnings as errors
-ML # link with libc.lib
-nologo # suppress copyright message
<<NOKEEP


all::	hq_map hq_mapdm

HQ_MAP_MAIN_OBJS=+$(OBJ_DIR)main.obj \
		+$(OBJ_DIR)wind.obj

HQ_MAP_DEMO_OBJS=+$(OBJ_DIR)maindm.obj \
		+$(OBJ_DIR)winddm.obj

HQ_MAP_ALWAYS_OBJS=+$(OBJ_DIR)pice.obj \
		+$(OBJ_DIR)move.obj \
		+$(OBJ_DIR)rolldice.obj \
		+$(OBJ_DIR)table.obj \
		+$(OBJ_DIR)features.obj \
		+$(OBJ_DIR)random.obj \
		+$(OBJ_DIR)stairs.obj \
		+$(OBJ_DIR)test.obj \
		+$(OBJ_DIR)queue.obj \
		+$(OBJ_DIR)set.obj \
		+$(OBJ_DIR)map.obj \
		+$(OBJ_DIR)liste.obj \
		+$(OBJ_DIR)text.obj \
		+$(OBJ_DIR)makemap.obj \
		+$(OBJ_DIR)icon.obj \
		+$(OBJ_DIR)image.obj \
		+$(OBJ_DIR)pcx.obj \
		+$(OBJ_DIR)bmp.obj \
		+$(OBJ_DIR)mem.obj \
		\
		+$(OBJ_DIR)path.obj \
		+$(OBJ_DIR)termproc.obj \
		+$(OBJ_DIR)grect.obj \
		+$(OBJ_DIR)routine.obj \
		\
		+$(OBJ_DIR)w_mouse.obj \
		+$(OBJ_DIR)w_dialog.obj \
		+$(OBJ_DIR)memory.obj \
		+$(OBJ_DIR)file_io.obj \
		+$(OBJ_DIR)boxf.obj \
		+$(OBJ_DIR)mfdb.obj \
		+$(OBJ_DIR)filesel.obj \
		+$(OBJ_DIR)openwork.obj \
		+$(OBJ_DIR)window.obj \
		+$(OBJ_DIR)ro_help.obj \
		+$(OBJ_DIR)w_draw.obj \
		+$(OBJ_DIR)w_print.obj \
		+$(OBJ_DIR)profile.obj

HQ_MAP_OBJS=	$(HQ_MAP_MAIN_OBJS) \
		$(HQ_MAP_ALWAYS_OBJS)

HQ_MAP_DEP_OBJS=	$(HQ_MAP_OBJS:+=)

HQ_MAP_DM_OBJS=	$(HQ_MAP_DEMO_OBJS) \
		$(HQ_MAP_ALWAYS_OBJS)

HQ_MAP_DM_DEP_OBJS=	$(HQ_MAP_DM_OBJS:+=)

hq_map:		dirs $(BIN_DIR)\hq_map$(TARGET).exe

!if "$(COMPILER)"=="BORLANDC"
$(BIN_DIR)\hq_map$(TARGET).exe: $(CFGFILE) $(HQ_MAP_DEP_OBJS) $(OBJ_DIR)menu.res $(OBJ_DIR)grafic.res $(MAKEFILE)
		@cd $(OBJ_DIR)
		$(TLINK) @<<
			$(LFLAGS) +
			$(C0OBJ)+
			$(HQ_MAP_OBJS)
			$@
			$*
			$(LIBS)
			$(SRC_DIR)\hq_map$(TARGET).def
			$(OBJ_DIR)menu.res+$(OBJ_DIR)grafic.res
<<NOKEEP
		@cd $(SRC_DIR)
!endif



hq_mapdm:		dirs $(BIN_DIR)\hq_mapdm$(TARGET).exe

!if "$(COMPILER)"=="BORLANDC"
$(BIN_DIR)\hq_mapdm$(TARGET).exe: $(CFGFILE) $(HQ_MAP_DM_DEP_OBJS) $(OBJ_DIR)menu.res $(OBJ_DIR)grafic.res $(MAKEFILE)
		@cd $(OBJ_DIR)
		$(TLINK) @<<
			$(LFLAGS) +
			$(C0OBJ)+
			$(HQ_MAP_DM_OBJS)
			$@
			$*
			$(LIBS)
			$(SRC_DIR)\hq_map$(TARGET).def
			$(OBJ_DIR)menu.res+$(OBJ_DIR)grafic.res
<<NOKEEP
		@cd $(SRC_DIR)
!endif

strip:
		$(STRIP) $(BIN_DIR)\hq_map$(TARGET).exe
		$(STRIP) $(BIN_DIR)\hq_mapdm$(TARGET).exe

$(OBJ_DIR)menu.res:	$(RSH_DIR)\menu.rc $(RSH_DIR)\menu.rh version.h
!if "$(COMPILER)"=="BORLANDC"
		$(BRC) -R -FO$@ $(RSH_DIR)\menu.rc @<<
			$(INCLUDES)
			$(EXTRA_DEFINES)
			$(DEFINES)
<<NOKEEP
!endif

$(OBJ_DIR)grafic.res:	$(RSH_DIR)\grafic.rc $(RSH_DIR)\grafic.rh version.h
!if "$(COMPILER)"=="BORLANDC"
		$(BRC) -R -FO$@ $(RSH_DIR)\grafic.rc @<<
			$(INCLUDES)
			$(EXTRA_DEFINES)
			$(DEFINES)
<<NOKEEP
!endif



####################################################################

$(OBJ_DIR)main.obj:	$(SRC_DIR)\main.c $(RSH_DIR)\menu.rh $(SRC_DIR)\demo.h
		$(CCC) $(EXTRA_DEFINES) -I$(RSH_DIR) $(SRC_DIR)\main.c
$(OBJ_DIR)wind.obj:	$(SRC_DIR)\wind.c $(RSH_DIR)\menu.rh $(SRC_DIR)\demo.h
		$(CCC) $(EXTRA_DEFINES) -I$(RSH_DIR) $(SRC_DIR)\wind.c
$(OBJ_DIR)maindm.obj:	$(SRC_DIR)\maindm.c $(SRC_DIR)\main.c $(RSH_DIR)\menu.rh $(SRC_DIR)\demo.h
		$(CCC) $(EXTRA_DEFINES) -I$(RSH_DIR) $(SRC_DIR)\maindm.c
$(OBJ_DIR)winddm.obj:	$(SRC_DIR)\winddm.c $(SRC_DIR)\wind.c $(RSH_DIR)\menu.rh $(SRC_DIR)\demo.h
		$(CCC) $(EXTRA_DEFINES) -I$(RSH_DIR) $(SRC_DIR)\winddm.c
$(OBJ_DIR)icon.obj:	$(SRC_DIR)\icon.c $(RSH_DIR)\menu.rh
		$(CCC) $(EXTRA_DEFINES) -I$(RSH_DIR) $(SRC_DIR)\icon.c
$(OBJ_DIR)mem.obj:	$(SRC_DIR)\mem.c
		$(CCC) $(EXTRA_DEFINES) $(SRC_DIR)\mem.c
$(OBJ_DIR)pice.obj:	$(SRC_DIR)\pice.c
		$(CCC) $(EXTRA_DEFINES) $(SRC_DIR)\pice.c
$(OBJ_DIR)move.obj:	$(SRC_DIR)\move.c
		$(CCC) $(EXTRA_DEFINES) $(SRC_DIR)\move.c
$(OBJ_DIR)rolldice.obj:	$(SRC_DIR)\rolldice.c
		$(CCC) $(EXTRA_DEFINES) $(SRC_DIR)\rolldice.c
$(OBJ_DIR)table.obj:	$(SRC_DIR)\table.c
		$(CCC) $(EXTRA_DEFINES) $(SRC_DIR)\table.c
$(OBJ_DIR)features.obj:	$(SRC_DIR)\features.c
		$(CCC) $(EXTRA_DEFINES) $(SRC_DIR)\features.c
$(OBJ_DIR)random.obj:	$(SRC_DIR)\random.c
		$(CCC) $(EXTRA_DEFINES) $(SRC_DIR)\random.c
$(OBJ_DIR)stairs.obj:	$(SRC_DIR)\stairs.c
		$(CCC) $(EXTRA_DEFINES) $(SRC_DIR)\stairs.c
$(OBJ_DIR)test.obj:	$(SRC_DIR)\test.c
		$(CCC) $(EXTRA_DEFINES) $(SRC_DIR)\test.c
$(OBJ_DIR)text.obj:	$(SRC_DIR)\text.c
		$(CCC) $(EXTRA_DEFINES) $(SRC_DIR)\text.c
$(OBJ_DIR)map.obj:	$(SRC_DIR)\map.c
		$(CCC) $(EXTRA_DEFINES) $(SRC_DIR)\map.c
$(OBJ_DIR)queue.obj:	$(SRC_DIR)\queue.c
		$(CCC) $(EXTRA_DEFINES) $(SRC_DIR)\queue.c
$(OBJ_DIR)set.obj:	$(SRC_DIR)\set.c
		$(CCC) $(EXTRA_DEFINES) $(SRC_DIR)\set.c
$(OBJ_DIR)liste.obj:	$(SRC_DIR)\liste.c
		$(CCC) $(EXTRA_DEFINES) $(SRC_DIR)\liste.c
$(OBJ_DIR)makemap.obj:	$(SRC_DIR)\makemap.c
		$(CCC) $(EXTRA_DEFINES) $(SRC_DIR)\makemap.c
$(OBJ_DIR)image.obj:	$(SRC_DIR)\image.c
		$(CCC) $(EXTRA_DEFINES) $(SRC_DIR)\image.c
$(OBJ_DIR)pcx.obj:	$(SRC_DIR)\pcx.c
		$(CCC) $(EXTRA_DEFINES) $(SRC_DIR)\pcx.c
$(OBJ_DIR)bmp.obj:	$(SRC_DIR)\bmp.c
		$(CCC) $(EXTRA_DEFINES) $(SRC_DIR)\bmp.c

$(OBJ_DIR)grect.obj:	$(MYLIB_DIR)\grect.c
		$(CCC) $(EXTRA_DEFINES) $(MYLIB_DIR)\grect.c
$(OBJ_DIR)path.obj:	$(MYLIB_DIR)\path.c
		$(CCC) $(EXTRA_DEFINES) $(MYLIB_DIR)\path.c
$(OBJ_DIR)routine.obj:	$(MYLIB_DIR)\routine.c
		$(CCC) $(EXTRA_DEFINES) $(MYLIB_DIR)\routine.c
$(OBJ_DIR)termproc.obj:	$(MYLIB_DIR)\termproc.c
		$(CCC) $(EXTRA_DEFINES) $(MYLIB_DIR)\termproc.c

$(OBJ_DIR)w_mouse.obj:	$(SYSPORTAB_DIR)\w_mouse.c
		$(CCC) $(EXTRA_DEFINES) $(SYSPORTAB_DIR)\w_mouse.c
$(OBJ_DIR)w_dialog.obj:	$(SYSPORTAB_DIR)\w_dialog.c
		$(CCC) $(EXTRA_DEFINES) $(SYSPORTAB_DIR)\w_dialog.c
$(OBJ_DIR)memory.obj:	$(SYSPORTAB_DIR)\memory.c
		$(CCC) $(EXTRA_DEFINES) $(SYSPORTAB_DIR)\memory.c
$(OBJ_DIR)file_io.obj:	$(SYSPORTAB_DIR)\file_io.c
		$(CCC) $(EXTRA_DEFINES) $(SYSPORTAB_DIR)\file_io.c
$(OBJ_DIR)boxf.obj:	$(SYSPORTAB_DIR)\boxf.c
		$(CCC) $(EXTRA_DEFINES) $(SYSPORTAB_DIR)\boxf.c
$(OBJ_DIR)mfdb.obj:	$(SYSPORTAB_DIR)\mfdb.c
		$(CCC) $(EXTRA_DEFINES) $(SYSPORTAB_DIR)\mfdb.c
$(OBJ_DIR)openwork.obj:	$(SYSPORTAB_DIR)\openwork.c
		$(CCC) $(EXTRA_DEFINES) $(SYSPORTAB_DIR)\openwork.c
$(OBJ_DIR)window.obj:	$(SYSPORTAB_DIR)\window.c
		$(CCC) $(EXTRA_DEFINES) $(SYSPORTAB_DIR)\window.c
$(OBJ_DIR)ro_help.obj:	$(SYSPORTAB_DIR)\ro_help.c
		$(CCC) $(EXTRA_DEFINES) $(SYSPORTAB_DIR)\ro_help.c
$(OBJ_DIR)w_draw.obj:	$(SYSPORTAB_DIR)\w_draw.c
		$(CCC) $(EXTRA_DEFINES) $(SYSPORTAB_DIR)\w_draw.c
$(OBJ_DIR)profile.obj:	$(SYSPORTAB_DIR)\profile.c
		$(CCC) $(EXTRA_DEFINES) $(SYSPORTAB_DIR)\profile.c
$(OBJ_DIR)filesel.obj:	$(SYSPORTAB_DIR)\filesel.c
		$(CCC) $(EXTRA_DEFINES) $(SYSPORTAB_DIR)\filesel.c
$(OBJ_DIR)w_print.obj:	$(SYSPORTAB_DIR)\w_print.c
		$(CCC) $(EXTRA_DEFINES) $(SYSPORTAB_DIR)\w_print.c




dirs::
	@if not exist obj\nul md obj
	@if not exist obj\win16\nul md obj\win16
	@if not exist obj\win32\nul md obj\win32
	@if not exist bin\nul md bin

clean::
	-if exist obj\win16\*.obj del obj\win16\*.obj
	-if exist obj\win32\*.obj del obj\win32\*.obj
	-if exist obj\win16\*.res del obj\win16\*.res
	-if exist obj\win32\*.res del obj\win32\*.res
	-if exist obj\win16\*.lib del obj\win16\*.lib
	-if exist obj\win32\*.lib del obj\win32\*.lib
	-if exist obj\win16\make*.@@@ del obj\win16\make*.@@@
	-if exist obj\win32\make*.@@@ del obj\win32\make*.@@@
	-if exist bin\*.exe del bin\*.exe
	-if exist bin\*.com del bin\*.com
	-if exist bin\*.fts attrib -h bin\*.fts
	-if exist bin\*.fts del bin\*.fts
	-if exist bin\*.gid attrib -h bin\*.gid
	-if exist bin\*.gid del bin\*.gid
	-if exist *.cfg del *.cfg
	-if exist debug.out del debug.out
	-if exist bin\debug.out del bin\debug.out
