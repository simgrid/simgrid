
####
#### THIS FILE WAS GENERATED, DO NOT EDIT BEFORE RENAMING IT
####


## Variable declarations
PROJECT_NAME=gras_prop
DISTDIR=gras-$(PROJECT_NAME)

# Set the GRAS_ROOT environment variable to the path under which you installed SimGrid
# Compilation will fail if you don't do so
GRAS_ROOT?= $(shell echo "\"<<<< GRAS_ROOT undefined !!! >>>>\"")

# You can fiddle the following to make it fit your taste
INCLUDES = -I$(GRAS_ROOT)/include
CFLAGS ?= -O3 -w -g
LIBS_SIM = -lm  -L$(GRAS_ROOT)/lib/ -lsimgrid
LIBS_RL = -lm  -L$(GRAS_ROOT)/lib/ -lgras
LIBS = 

PRECIOUS_C_FILES ?= gras_prop.c
GENERATED_C_FILES = _gras_prop_simulator.c _gras_prop_client.c _gras_prop_server.c 
OBJ_FILES = $(patsubst %.c,%.o,$(PRECIOUS_C_FILES))
BIN_FILES = gras_prop_simulator gras_prop_client gras_prop_server 

## By default, build all the binaries
all: $(BIN_FILES)


## generate temps: regenerate the source file each time the deployment file changes
_gras_prop_client.c _gras_prop_server.c _gras_prop_simulator.c: gras_prop_deployment.xml
	gras_stub_generator gras_prop gras_prop_deployment.xml >/dev/null

## Generate the binaries
gras_prop_simulator: _gras_prop_simulator.o $(OBJ_FILES)
	$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) $^ $(LIBS_SIM) $(LIBS) $(LDADD) -o $@ 
gras_prop_client : _gras_prop_client.o $(OBJ_FILES)
	$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) $^ $(LIBS_RL) $(LIBS) $(LDADD) -o $@ 
gras_prop_server : _gras_prop_server.o $(OBJ_FILES)
	$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) $^ $(LIBS_RL) $(LIBS) $(LDADD) -o $@ 

%: %.o
	$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) $^ $(LIBS) $(LDADD) -o $@ 

%.o: %.c
	$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) -c -o $@ $<

## Rules for tarballs and cleaning
DIST_FILES= $(EXTRA_DIST) $(GENERATED_C_FILES) $(PRECIOUS_C_FILES) gras_prop.mk 
distdir: $(DIST_FILES)
	rm -rf $(DISTDIR)
	mkdir -p $(DISTDIR)
	cp $^ $(DISTDIR)

dist: clean distdir
	tar c $(DISTDIR) | gzip -c9 > $(DISTDIR).tar.gz

clean:
	rm -f $(CLEANFILES) $(BIN_FILES) $(OBJ_FILES) *~ gras_prop.o _gras_prop_simulator.o _gras_prop_client.o _gras_prop_server.o
	rm -rf $(DISTDIR)

.SUFFIXES:
.PHONY : clean

