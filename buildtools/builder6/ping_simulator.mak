
.autodepend

!if !$d(BCB)
BCB = $(MAKEDIR)\..
!endif

PROJECT = libgras\examples\ping_simulator.exe
OBJFILES = _ping_simulator.obj ping.obj
LIBFILES = simgrid\lib\debug\simgrid.lib

USERDEFINES = _DEBUG
SYSDEFINES = NO_STRICT;_NO_VCL;_RTLDLL
INCLUDEPATH = $(BCB)\include;$(BCB)\include\vcl;..\..\include;..\..\src\include;..\..\src
LIBPATH = $(BCB)\lib\obj;$(BCB)\lib;libgras\lib\debug;libgras\examples\obj
WARNINGS= -w-sus -w-rvl -w-rch -w-pia -w-pch -w-par -w-csu -w-ccc -w-aus
SRCDIR = .;..\..\examples\gras\ping

CFLAGS = -Od -H=$(BCB)\lib\vcl60.csm -Hc -Vx -Ve -X- -r- -a8 -b- -k -y -v -vi- -tWC -tWM- -c -p- -nlibgras\examples\obj

LFLAGS = -Iobj -ap -Tpe -x -Gn -v

ALLOBJ = c0x32.obj $(OBJFILES)

ALLLIB = $(LIBFILES) import32.lib cw32i.lib

!if !$d(BCC32)
BCC32 = bcc32
!endif


!if !$d(LINKER)
LINKER = ilink32
!endif

!if $d(SRCDIR)
.path.c   = $(SRCDIR)
!endif

$(PROJECT): $(OBJFILES) 
    $(BCB)\BIN\$(LINKER) @&&!
    $(LFLAGS) -L$(LIBPATH) +
    $(ALLOBJ), +
    $(PROJECT),, +
    $(ALLLIB)
!

.c.obj:
    $(BCB)\BIN\$(BCC32) $(CFLAGS) $(WARNINGS) -I$(INCLUDEPATH) -D$(USERDEFINES);$(SYSDEFINES) {$< }






