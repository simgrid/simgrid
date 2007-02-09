# ---------------------------------------------------------------------------
!if !$d(BCB)
BCB = $(MAKEDIR)\..
!endif

# ---------------------------------------------------------------------------
# IDE SECTION
# ---------------------------------------------------------------------------
# The following section of the project makefile is managed by the BCB IDE.
# It is recommended to use the IDE to change any of the values in this
# section.
# ---------------------------------------------------------------------------

VERSION = BCB.06.00
# ---------------------------------------------------------------------------
PROJECT = _stub_generator.exe
OBJFILES = _stub_generator.obj ..\..\src\surf\surfxml_parse.obj \
    ..\..\src\surf\trace_mgr.obj ..\..\src\xbt\snprintf.obj \
    ..\..\src\surf\surf.obj ..\..\src\surf\maxmin.obj
RESFILES = _stub_generator.res
MAINSOURCE = _stub_generator.bpf
RESDEPEN = $(RESFILES)
LIBFILES = simgrid.lib
IDLFILES = 
IDLGENFILES = 
LIBRARIES = rtl.lib vcl.lib
PACKAGES = vcl.bpi vclx.bpi bcbsmp.bpi dbrtl.bpi vcldb.bpi adortl.bpi ibsmp.bpi \
    bdertl.bpi vcldbx.bpi qrpt.bpi teeui.bpi teedb.bpi tee.bpi dss.bpi \
    teeqr.bpi ibxpress.bpi dsnap.bpi vclie.bpi inetdb.bpi inet.bpi nmfast.bpi \
    webdsnap.bpi bcbie.bpi dclocx.bpi bcb2kaxserver.bpi PackageLableDate.bpi
SPARELIBS = vcl.lib rtl.lib
DEFFILE = 
OTHERFILES = 
# ---------------------------------------------------------------------------
DEBUGLIBPATH = $(BCB)\lib\debug
RELEASELIBPATH = $(BCB)\lib\release
USERDEFINES = _DEBUG
SYSDEFINES = NO_STRICT;_NO_VCL
INCLUDEPATH = ..\..\src\gras;..\..\src\gras\Virtu;..\..\src\xbt;..\..\src\surf;..\..\..\tools\gras;$(BCB)\include;$(BCB)\include\vcl;C:\dev\cvs\simgrid\include;C:\dev\cvs\simgrid\src\include;C:\dev\cvs\simgrid\src;C:\dev\cvs\simgrid\src\gras;C:\dev\cvs\simgrid\inc
LIBPATH = ..\..\src\gras;..\..\src\gras\Virtu;..\..\src\xbt;..\..\src\surf;..\..\..\tools\gras;$(BCB)\lib;$(BCB)\lib\obj
WARNINGS= -w-rvl -w-rch -w-pia -w-pch -w-par -w-csu -w-ccc -w-aus
PATHCPP = .;..\..\src\surf;..\..\src\surf;..\..\src\xbt;..\..\src\surf;..\..\src\surf
PATHASM = .;
PATHPAS = .;
PATHRC = .;
PATHOBJ = .;$(LIBPATH)
# ---------------------------------------------------------------------------
CFLAG1 = -Od -H=c:\PROGRA~1\borland\CBUILD~1\lib\vcl60.csm -Hh=100 -Hc -Vx -Ve -Vmp \
    -X- -r- -ps -a1 -b- -k -y -v -vi- -tWC -tWM- -c
IDLCFLAGS = -I..\..\src\gras -I..\..\src\gras\Virtu -I..\..\src\xbt -I..\..\src\surf \
    -I..\..\..\tools\gras -I$(BCB)\include -I$(BCB)\include\vcl \
    -IC:\dev\cvs\simgrid\include -IC:\dev\cvs\simgrid\src\include \
    -IC:\dev\cvs\simgrid\src -IC:\dev\cvs\simgrid\src\gras \
    -IC:\dev\cvs\simgrid\inc -src_suffix cpp -D_DEBUG -boa
PFLAGS = -$Y+ -$W -$O- -$A8 -v -JPHNE -M
RFLAGS = 
AFLAGS = /mx /w2 /zi
LFLAGS = -D"" -c- -ap -Tpe -x -Gn -w -v
# ---------------------------------------------------------------------------
ALLOBJ = c0x32.obj $(OBJFILES)
ALLRES = $(RESFILES)
ALLLIB = $(LIBFILES) $(LIBRARIES) import32.lib cw32.lib
# ---------------------------------------------------------------------------
!ifdef IDEOPTIONS

[Version Info]
IncludeVerInfo=0
AutoIncBuild=0
MajorVer=1
MinorVer=0
Release=0
Build=0
Debug=0
PreRelease=0
Special=0
Private=0
DLL=0

[Version Info Keys]
CompanyName=
FileDescription=
FileVersion=1.0.0.0
InternalName=
LegalCopyright=
LegalTrademarks=
OriginalFilename=
ProductName=
ProductVersion=1.0.0.0
Comments=

[Debugging]
DebugSourceDirs=$(BCB)\source\vcl

!endif





# ---------------------------------------------------------------------------
# MAKE SECTION
# ---------------------------------------------------------------------------
# This section of the project file is not used by the BCB IDE.  It is for
# the benefit of building from the command-line using the MAKE utility.
# ---------------------------------------------------------------------------

.autodepend
# ---------------------------------------------------------------------------
!if "$(USERDEFINES)" != ""
AUSERDEFINES = -d$(USERDEFINES:;= -d)
!else
AUSERDEFINES =
!endif

!if !$d(BCC32)
BCC32 = bcc32
!endif

!if !$d(CPP32)
CPP32 = cpp32
!endif

!if !$d(DCC32)
DCC32 = dcc32
!endif

!if !$d(TASM32)
TASM32 = tasm32
!endif

!if !$d(LINKER)
LINKER = ilink32
!endif

!if !$d(BRCC32)
BRCC32 = brcc32
!endif


# ---------------------------------------------------------------------------
!if $d(PATHCPP)
.PATH.CPP = $(PATHCPP)
.PATH.C   = $(PATHCPP)
!endif

!if $d(PATHPAS)
.PATH.PAS = $(PATHPAS)
!endif

!if $d(PATHASM)
.PATH.ASM = $(PATHASM)
!endif

!if $d(PATHRC)
.PATH.RC  = $(PATHRC)
!endif

!if $d(PATHOBJ)
.PATH.OBJ  = $(PATHOBJ)
!endif
# ---------------------------------------------------------------------------
$(PROJECT): $(OTHERFILES) $(IDLGENFILES) $(OBJFILES) $(RESDEPEN) $(DEFFILE)
    $(BCB)\BIN\$(LINKER) @&&!
    $(LFLAGS) -L$(LIBPATH) +
    $(ALLOBJ), +
    $(PROJECT),, +
    $(ALLLIB), +
    $(DEFFILE), +
    $(ALLRES)
!
# ---------------------------------------------------------------------------
.pas.hpp:
    $(BCB)\BIN\$(DCC32) $(PFLAGS) -U$(INCLUDEPATH) -D$(USERDEFINES);$(SYSDEFINES) -O$(INCLUDEPATH) --BCB {$< }

.pas.obj:
    $(BCB)\BIN\$(DCC32) $(PFLAGS) -U$(INCLUDEPATH) -D$(USERDEFINES);$(SYSDEFINES) -O$(INCLUDEPATH) --BCB {$< }

.cpp.obj:
    $(BCB)\BIN\$(BCC32) $(CFLAG1) $(WARNINGS) -I$(INCLUDEPATH) -D$(USERDEFINES);$(SYSDEFINES) -n$(@D) {$< }

.c.obj:
    $(BCB)\BIN\$(BCC32) $(CFLAG1) $(WARNINGS) -I$(INCLUDEPATH) -D$(USERDEFINES);$(SYSDEFINES) -n$(@D) {$< }

.c.i:
    $(BCB)\BIN\$(CPP32) $(CFLAG1) $(WARNINGS) -I$(INCLUDEPATH) -D$(USERDEFINES);$(SYSDEFINES) -n. {$< }

.cpp.i:
    $(BCB)\BIN\$(CPP32) $(CFLAG1) $(WARNINGS) -I$(INCLUDEPATH) -D$(USERDEFINES);$(SYSDEFINES) -n. {$< }

.asm.obj:
    $(BCB)\BIN\$(TASM32) $(AFLAGS) -i$(INCLUDEPATH:;= -i) $(AUSERDEFINES) -d$(SYSDEFINES:;= -d) $<, $@

.rc.res:
    $(BCB)\BIN\$(BRCC32) $(RFLAGS) -I$(INCLUDEPATH) -D$(USERDEFINES);$(SYSDEFINES) -fo$@ $<



# ---------------------------------------------------------------------------




