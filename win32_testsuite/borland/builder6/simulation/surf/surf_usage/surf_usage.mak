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
PROJECT = ..\..\bin\surf_usage.exe
OBJFILES = ..\..\obj\surf_usage.obj ..\..\obj\graphxml_parse.obj ..\..\obj\heap.obj \
    ..\..\obj\log.obj ..\..\obj\log_default_appender.obj \
    ..\..\obj\mallocator.obj ..\..\obj\set.obj ..\..\obj\snprintf.obj \
    ..\..\obj\swag.obj ..\..\obj\sysdep.obj ..\..\obj\xbt_main.obj \
    ..\..\obj\xbt_matrix.obj ..\..\obj\xbt_peer.obj ..\..\obj\asserts.obj \
    ..\..\obj\config.obj ..\..\obj\cunit.obj ..\..\obj\dict.obj \
    ..\..\obj\dict_cursor.obj ..\..\obj\dict_elm.obj ..\..\obj\dict_multi.obj \
    ..\..\obj\dynar.obj ..\..\obj\ex.obj ..\..\obj\fifo.obj \
    ..\..\obj\graph.obj ..\..\obj\gras.obj ..\..\obj\transport.obj \
    ..\..\obj\datadesc.obj ..\..\obj\ddt_convert.obj ..\..\obj\ddt_create.obj \
    ..\..\obj\ddt_exchange.obj ..\..\obj\ddt_parse.obj \
    ..\..\obj\ddt_parse.yy.obj ..\..\obj\cbps.obj ..\..\obj\timer.obj \
    ..\..\obj\msg.obj ..\..\obj\rpc.obj ..\..\obj\process.obj \
    ..\..\obj\gras_module.obj ..\..\obj\workstation_KCCFLN05.obj \
    ..\..\obj\cpu.obj ..\..\obj\maxmin.obj ..\..\obj\network.obj \
    ..\..\obj\network_dassf.obj ..\..\obj\surf.obj ..\..\obj\surf_timer.obj \
    ..\..\obj\surfxml_parse.obj ..\..\obj\trace_mgr.obj \
    ..\..\obj\workstation.obj ..\..\obj\deployment.obj \
    ..\..\obj\environment.obj ..\..\obj\global.obj ..\..\obj\gos.obj \
    ..\..\obj\host.obj ..\..\obj\m_process.obj ..\..\obj\msg_config.obj \
    ..\..\obj\task.obj ..\..\obj\sd_workstation.obj ..\..\obj\sd_global.obj \
    ..\..\obj\sd_link.obj ..\..\obj\sd_task.obj \
    ..\..\obj\transport_plugin_sg.obj ..\..\obj\sg_transport.obj \
    ..\..\obj\sg_time.obj ..\..\obj\sg_dns.obj ..\..\obj\sg_emul.obj \
    ..\..\obj\sg_process.obj ..\..\obj\sg_msg.obj ..\..\obj\amok_base.obj \
    ..\..\obj\saturate.obj ..\..\obj\bandwidth.obj \
    ..\..\obj\peermanagement.obj ..\..\obj\context_win32.obj \
    ..\..\obj\context.obj
RESFILES = surf_usage.res
MAINSOURCE = surf_usage.bpf
RESDEPEN = $(RESFILES)
LIBFILES = 
IDLFILES = 
IDLGENFILES = 
LIBRARIES = 
PACKAGES = vcl.bpi rtl.bpi dbrtl.bpi adortl.bpi vcldb.bpi vclx.bpi bdertl.bpi \
    vcldbx.bpi ibxpress.bpi dsnap.bpi cds.bpi bdecds.bpi qrpt.bpi teeui.bpi \
    teedb.bpi tee.bpi dss.bpi teeqr.bpi dsnapcrba.bpi dsnapcon.bpi bcbsmp.bpi \
    vclie.bpi xmlrtl.bpi inet.bpi inetdbbde.bpi inetdbxpress.bpi inetdb.bpi \
    nmfast.bpi webdsnap.bpi bcbie.bpi websnap.bpi soaprtl.bpi dclocx.bpi \
    dbexpress.bpi dbxcds.bpi indy.bpi bcb2kaxserver.bpi
SPARELIBS = 
DEFFILE = 
OTHERFILES = 
# ---------------------------------------------------------------------------
DEBUGLIBPATH = $(BCB)\lib\debug
RELEASELIBPATH = $(BCB)\lib\release
USERDEFINES = _DEBUG
SYSDEFINES = NO_STRICT;_NO_VCL;_RTLDLL
INCLUDEPATH = ..\..\..\..\..\..\testsuite\surf;..\..\..\..\..\..\src\amok\PeerManagement;..\..\..\..\..\..\src\amok\Bandwidth;..\..\..\..\..\..\src\amok;..\..\..\..\..\..\src\simdag;..\..\..\..\..\..\src\msg;..\..\..\..\..\..\src\surf;..\..\..\..\..\..\src\gras\Virtu;..\..\..\..\..\..\src\gras\Msg;..\..\..\..\..\..\src\gras\DataDesc;..\..\..\..\..\..\src\gras\Transport;..\..\..\..\..\..\src\gras;..\..\..\..\..\..\src\xbt;$(BCB)\include;$(BCB)\include\vcl;..\..\..\..\..\..\include;..\..\..\..\..\..\src;..\..\..\..\..\..\src\include
LIBPATH = ..\..\..\..\..\..\testsuite\surf;..\..\..\..\..\..\src\amok\PeerManagement;..\..\..\..\..\..\src\amok\Bandwidth;..\..\..\..\..\..\src\amok;..\..\..\..\..\..\src\simdag;..\..\..\..\..\..\src\msg;..\..\..\..\..\..\src\surf;..\..\..\..\..\..\src\gras\Virtu;..\..\..\..\..\..\src\gras\Msg;..\..\..\..\..\..\src\gras\DataDesc;..\..\..\..\..\..\src\gras\Transport;..\..\..\..\..\..\src\gras;..\..\..\..\..\..\src\xbt;$(BCB)\lib\obj;$(BCB)\lib
WARNINGS= -w-rvl -w-rch -w-pia -w-pch -w-par -w-csu -w-ccc -w-aus
PATHCPP = .;..\..\..\..\..\..\testsuite\surf;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\gras;..\..\..\..\..\..\src\gras\Transport;..\..\..\..\..\..\src\gras\DataDesc;..\..\..\..\..\..\src\gras\DataDesc;..\..\..\..\..\..\src\gras\DataDesc;..\..\..\..\..\..\src\gras\DataDesc;..\..\..\..\..\..\src\gras\DataDesc;..\..\..\..\..\..\src\gras\DataDesc;..\..\..\..\..\..\src\gras\DataDesc;..\..\..\..\..\..\src\gras\Msg;..\..\..\..\..\..\src\gras\Msg;..\..\..\..\..\..\src\gras\Msg;..\..\..\..\..\..\src\gras\Virtu;..\..\..\..\..\..\src\gras\Virtu;..\..\..\..\..\..\src\surf;..\..\..\..\..\..\src\surf;..\..\..\..\..\..\src\surf;..\..\..\..\..\..\src\surf;..\..\..\..\..\..\src\surf;..\..\..\..\..\..\src\surf;..\..\..\..\..\..\src\surf;..\..\..\..\..\..\src\surf;..\..\..\..\..\..\src\surf;..\..\..\..\..\..\src\surf;..\..\..\..\..\..\src\msg;..\..\..\..\..\..\src\msg;..\..\..\..\..\..\src\msg;..\..\..\..\..\..\src\msg;..\..\..\..\..\..\src\msg;..\..\..\..\..\..\src\msg;..\..\..\..\..\..\src\msg;..\..\..\..\..\..\src\msg;..\..\..\..\..\..\src\simdag;..\..\..\..\..\..\src\simdag;..\..\..\..\..\..\src\simdag;..\..\..\..\..\..\src\simdag;..\..\..\..\..\..\src\gras\Transport;..\..\..\..\..\..\src\gras\Transport;..\..\..\..\..\..\src\gras\Virtu;..\..\..\..\..\..\src\gras\Virtu;..\..\..\..\..\..\src\gras\Virtu;..\..\..\..\..\..\src\gras\Virtu;..\..\..\..\..\..\src\gras\Msg;..\..\..\..\..\..\src\amok;..\..\..\..\..\..\src\amok\Bandwidth;..\..\..\..\..\..\src\amok\Bandwidth;..\..\..\..\..\..\src\amok\PeerManagement;..\..\..\..\..\..\src\xbt;..\..\..\..\..\..\src\xbt
PATHASM = .;
PATHPAS = .;
PATHRC = .;
PATHOBJ = .;$(LIBPATH)
# ---------------------------------------------------------------------------
CFLAG1 = -Od -H=$(BCB)\lib\vcl60.csm -Hc -Vx -Ve -X- -r- -a8 -b- -k -y -v -vi- -tWC \
    -tWM- -c
IDLCFLAGS = -src_suffix cpp -D_DEBUG -I..\..\..\..\..\..\testsuite\surf \
    -I..\..\..\..\..\..\src\amok\PeerManagement \
    -I..\..\..\..\..\..\src\amok\Bandwidth -I..\..\..\..\..\..\src\amok \
    -I..\..\..\..\..\..\src\simdag -I..\..\..\..\..\..\src\msg \
    -I..\..\..\..\..\..\src\surf -I..\..\..\..\..\..\src\gras\Virtu \
    -I..\..\..\..\..\..\src\gras\Msg -I..\..\..\..\..\..\src\gras\DataDesc \
    -I..\..\..\..\..\..\src\gras\Transport -I..\..\..\..\..\..\src\gras \
    -I..\..\..\..\..\..\src\xbt -I$(BCB)\include -I$(BCB)\include\vcl \
    -I..\..\..\..\..\..\include -I..\..\..\..\..\..\src \
    -I..\..\..\..\..\..\src\include -boa
PFLAGS = -N2..\..\obj -N0..\..\obj -$YD -$W -$O- -$A8 -v -JPHNE -M
RFLAGS = 
AFLAGS = /mx /w2 /zd
LFLAGS = -I..\..\obj -D"" -ap -Tpe -x -Gn -v
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




