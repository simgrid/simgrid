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
PROJECT = simgrid\tests\datadesc_usage.exe
OBJFILES = ..\..\win32_testsuite\borland\builder6\realistic\obj\datadesc_usage.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\graphxml_parse.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\heap.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\log.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\log_default_appender.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\mallocator.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\set.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\snprintf.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\swag.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\sysdep.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\xbt_main.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\xbt_matrix.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\xbt_peer.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\asserts.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\config.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\cunit.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\dict.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\dict_cursor.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\dict_elm.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\dict_multi.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\dynar.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\ex.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\fifo.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\graph.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\gras.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\transport.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\datadesc.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\ddt_convert.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\ddt_create.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\ddt_exchange.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\ddt_parse.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\ddt_parse.yy.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\cbps.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\timer.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\msg.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\rpc.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\process.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\gras_module.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\amok_base.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\saturate.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\bandwidth.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\peermanagement.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\context_win32.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\context.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\rl_transport.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\transport_plugin_file.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\transport_plugin_tcp.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\rl_time.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\rl_dns.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\rl_emul.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\rl_process.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\rl_msg.obj \
    ..\..\win32_testsuite\borland\builder6\realistic\obj\datadesc_structs.obj
RESFILES = 
MAINSOURCE = datadesc_usage.bpf
RESDEPEN = $(RESFILES)
LIBFILES = 
IDLFILES = 
IDLGENFILES = 
LIBRARIES = 
PACKAGES = 
SPARELIBS = 
DEFFILE = 
OTHERFILES = 
# ---------------------------------------------------------------------------
DEBUGLIBPATH = $(BCB)\simgrid\lib\debug
RELEASELIBPATH = $(BCB)\lib\release
USERDEFINES = _DEBUG
SYSDEFINES = NO_STRICT;_NO_VCL
INCLUDEPATH = ..\..\win32_testsuite\borland\builder6\realistic\gras\datadesc_usage;..\..\testsuite\gras;..\..\src\gras\Virtu;..\..\src\gras\Transport;..\..\src\amok\PeerManagement;..\..\src\amok\Bandwidth;..\..\src\amok;..\..\src\simdag;..\..\src\msg;..\..\src\surf;..\..\src\gras\Msg;..\..\src\gras\DataDesc;..\..\src\gras;..\..\src\xbt;$(BCB)\include;$(BCB)\include\vcl;..\..\src;..\..\src\include;..\..\include
LIBPATH = ..\..\win32_testsuite\borland\builder6\realistic\gras\datadesc_usage;..\..\testsuite\gras;..\..\src\gras\Virtu;..\..\src\gras\Transport;..\..\src\amok\PeerManagement;..\..\src\amok\Bandwidth;..\..\src\amok;..\..\src\simdag;..\..\src\msg;..\..\src\surf;..\..\src\gras\Msg;..\..\src\gras\DataDesc;..\..\src\gras;..\..\src\xbt;$(BCB)\lib\obj;$(BCB)\lib
WARNINGS= -w-sus -w-rvl -w-rch -w-pia -w-pch -w-par -w-csu -w-ccc -w-aus
PATHCPP = .;..\..\testsuite\gras;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\gras;..\..\src\gras\Transport;..\..\src\gras\DataDesc;..\..\src\gras\DataDesc;..\..\src\gras\DataDesc;..\..\src\gras\DataDesc;..\..\src\gras\DataDesc;..\..\src\gras\DataDesc;..\..\src\gras\DataDesc;..\..\src\gras\Msg;..\..\src\gras\Msg;..\..\src\gras\Msg;..\..\src\gras\Virtu;..\..\src\gras\Virtu;..\..\src\amok;..\..\src\amok\Bandwidth;..\..\src\amok\Bandwidth;..\..\src\amok\PeerManagement;..\..\src\xbt;..\..\src\xbt;..\..\src\gras\Transport;..\..\src\gras\Transport;..\..\src\gras\Transport;..\..\src\gras\Virtu;..\..\src\gras\Virtu;..\..\src\gras\Virtu;..\..\src\gras\Virtu;..\..\src\gras\Msg;..\..\win32_testsuite\borland\builder6\realistic\gras\datadesc_usage
PATHASM = .;
PATHPAS = .;
PATHRC = .;
PATHOBJ = .;$(LIBPATH)
# ---------------------------------------------------------------------------
CFLAG1 = -Od -H=$(BCB)\lib\vcl60.csm -Hc -Vx -Ve -X- -r- -a1 -b- -k -y -v -vi- -tWC \
    -tWM- -c
IDLCFLAGS = -I..\..\win32_testsuite\borland\builder6\realistic\gras\datadesc_usage \
    -I..\..\testsuite\gras -I..\..\src\gras\Virtu -I..\..\src\gras\Transport \
    -I..\..\src\amok\PeerManagement -I..\..\src\amok\Bandwidth \
    -I..\..\src\amok -I..\..\src\simdag -I..\..\src\msg -I..\..\src\surf \
    -I..\..\src\gras\Msg -I..\..\src\gras\DataDesc -I..\..\src\gras \
    -I..\..\src\xbt -I$(BCB)\include -I$(BCB)\include\vcl -I..\..\src \
    -I..\..\src\include -I..\..\include -src_suffix cpp -D_DEBUG -boa
PFLAGS = -N2..\..\win32_testsuite\borland\builder6\realistic\obj \
    -N0..\..\win32_testsuite\borland\builder6\realistic\obj -$YD -$W -$O- -$A8 \
    -v -JPHNE -M
RFLAGS = 
AFLAGS = /mx /w2 /zd
LFLAGS = -I..\..\win32_testsuite\borland\builder6\realistic\obj -D"" -ap -Tpe -x \
    -Gn -v
# ---------------------------------------------------------------------------
ALLOBJ = c0x32.obj $(OBJFILES)
ALLRES = 
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




