# Temporary Makefile for SimDag testing

INSTALL_PATH = $(HOME)
CC = gcc
PEDANTIC_PARANOID_FREAK =       -O0 -Wshadow -Wcast-align \
				-Waggregate-return -Wmissing-prototypes -Wmissing-declarations \
				-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
				-Wmissing-noreturn -Wredundant-decls -Wnested-externs \
				-Wpointer-arith -Wwrite-strings -finline-functions
REASONABLY_CAREFUL_DUDE =	-Wall
NO_PRAYER_FOR_THE_WICKED =	-w -O2 
WARNINGS = 			$(REASONABLY_CAREFUL_DUDE)
CFLAGS = -g $(WARNINGS)

INCLUDES = -I../../include -I../../src/include
DEFS = -L$(INSTALL_PATH)/lib
LIBS = -lm -lsimgrid

all: sd_test
sd_test: sd_test.o ../../src/simdag/global.o  ../../src/simdag/link.o  ../../src/simdag/task.o  ../../src/simdag/workstation.o

%: %.o
	$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) $^ $(LIBS) $(LDADD) -o $@

%.o: %.c
	$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *~ ../../src/simdag/*.o ../../src/simdag/*.~
.SUFFIXES:
.PHONY : clean
