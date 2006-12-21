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
PROJECT = bin\maxmin_usage.exe
OBJFILES = ..\..\win32_testsuite\borland\builder6\simulation\obj\maxmin_usage.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\graphxml_parse.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\heap.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\log.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\log_default_appender.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\mallocator.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\set.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\snprintf.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\swag.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\sysdep.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\xbt_main.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\xbt_matrix.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\xbt_peer.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\asserts.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\config.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\cunit.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\dict.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\dict_cursor.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\dict_elm.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\dict_multi.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\dynar.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\ex.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\fifo.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\graph.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\gras.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\transport.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\datadesc.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\ddt_convert.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\ddt_create.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\ddt_exchange.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\ddt_parse.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\ddt_parse.yy.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\cbps.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\timer.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\msg.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\rpc.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\process.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\gras_module.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\workstation_KCCFLN05.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\cpu.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\maxmin.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\network.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\network_dassf.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\surf.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\surf_timer.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\surfxml_parse.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\trace_mgr.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\workstation.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\deployment.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\environment.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\global.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\gos.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\host.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\m_process.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\msg_config.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\task.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\sd_workstation.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\sd_global.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\sd_link.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\sd_task.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\transport_plugin_sg.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\sg_transport.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\sg_time.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\sg_dns.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\sg_emul.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\sg_process.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\sg_msg.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\amok_base.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\saturate.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\bandwidth.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\peermanagement.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\context_win32.obj \
    ..\..\win32_testsuite\borland\builder6\simulation\obj\context.obj
RESFILES = 
MAINSOURCE = maxmin_usage.bpf
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
DEBUGLIBPATH = $(BCB)\lib\debug
RELEASELIBPATH = $(BCB)\lib\release
USERDEFINES = _DEBUG
SYSDEFINES = NO_STRICT;_NO_VCL;_RTLDLL
INCLUDEPATH = ..\..\testsuite\surf;..\..\src\amok\PeerManagement;..\..\src\amok\Bandwidth;..\..\src\amok;..\..\src\simdag;..\..\src\msg;..\..\src\surf;..\..\src\gras\Virtu;..\..\src\gras\Msg;..\..\src\gras\DataDesc;..\..\src\gras\Transport;..\..\src\gras;..\..\src\xbt;$(BCB)\include;$(BCB)\include\vcl;..\..\include;..\..\src;..\..\src\include
LIBPATH = ..\..\testsuite\surf;..\..\src\amok\PeerManagement;..\..\src\amok\Bandwidth;..\..\src\amok;..\..\src\simdag;..\..\src\msg;..\..\src\surf;..\..\src\gras\Virtu;..\..\src\gras\Msg;..\..\src\gras\DataDesc;..\..\src\gras\Transport;..\..\src\gras;..\..\src\xbt;$(BCB)\lib\obj;$(BCB)\lib
WARNINGS= -w-sus -w-rvl -w-rch -w-pia -w-pch -w-par -w-csu -w-ccc -w-aus
PATHCPP = .;..\..\testsuite\surf;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\xbt;..\..\src\gras;..\..\src\gras\Transport;..\..\src\gras\DataDesc;..\..\src\gras\DataDesc;..\..\src\gras\DataDesc;..\..\src\gras\DataDesc;..\..\src\gras\DataDesc;..\..\src\gras\DataDesc;..\..\src\gras\DataDesc;..\..\src\gras\Msg;..\..\src\gras\Msg;..\..\src\gras\Msg;..\..\src\gras\Virtu;..\..\src\gras\Virtu;..\..\src\surf;..\..\src\surf;..\..\src\surf;..\..\src\surf;..\..\src\surf;..\..\src\surf;..\..\src\surf;..\..\src\surf;..\..\src\surf;..\..\src\surf;..\..\src\msg;..\..\src\msg;..\..\src\msg;..\..\src\msg;..\..\src\msg;..\..\src\msg;..\..\src\msg;..\..\src\msg;..\..\src\simdag;..\..\src\simdag;..\..\src\simdag;..\..\src\simdag;..\..\src\gras\Transport;..\..\src\gras\Transport;..\..\src\gras\Virtu;..\..\src\gras\Virtu;..\..\src\gras\Virtu;..\..\src\gras\Virtu;..\..\src\gras\Msg;..\..\src\amok;..\..\src\amok\Bandwidth;..\..\src\amok\Bandwidth;..\..\src\amok\PeerManagement;..\..\src\xbt;..\..\src\xbt
PATHASM = .;
PATHPAS = .;
PATHRC = .;
PATHOBJ = .;$(LIBPATH)
# ---------------------------------------------------------------------------
CFLAG1 = -Od -H=$(BCB)\lib\vcl60.csm -Hc -Vx -Ve -X- -r- -a1 -b- -k -y -v -vi- -tWC \
    -tWM- -c
IDLCFLAGS = -I..\..\testsuite\surf -I..\..\src\amok\PeerManagement \
    -I..\..\src\amok\Bandwidth -I..\..\src\amok -I..\..\src\simdag \
    -I..\..\src\msg -I..\..\src\surf -I..\..\src\gras\Virtu \
    -I..\..\src\gras\Msg -I..\..\src\gras\DataDesc -I..\..\src\gras\Transport \
    -I..\..\src\gras -I..\..\src\xbt -I$(BCB)\include -I$(BCB)\include\vcl \
    -I..\..\include -I..\..\src -I..\..\src\include -src_suffix cpp -D_DEBUG \
    -boa
PFLAGS = -N2..\..\win32_testsuite\borland\builder6\simulation\obj \
    -N0..\..\win32_testsuite\borland\builder6\simulation\obj -$YD -$W -$O- \
    -$A8 -v -JPHNE -M
RFLAGS = 
AFLAGS = /mx /w2 /zd
LFLAGS = -I..\..\win32_testsuite\borland\builder6\simulation\obj -D"" -ap -Tpe -x \
    -Gn -v
# ---------------------------------------------------------------------------
ALLOBJ = c0x32.obj $(OBJFILES)
ALLRES = $(RESFILES)
ALLLIB = $(LIBFILES) $(LIBRARIES) import32.lib cw32i.lib
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




