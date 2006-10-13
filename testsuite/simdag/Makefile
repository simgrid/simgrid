# Temporary Makefile for SimDag testing

INSTALL_PATH = $(HOME)
CC = gcc
PEDANTIC_PARANOID_FREAK =       -O0 -Wshadow -Wcast-align \
				-Waggregate-return -Wmissing-prototypes -Wmissing-declarations \
				-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
				-Wmissing-noreturn -Wnested-externs \
				-Wpointer-arith -Wwrite-strings -finline-functions
REASONABLY_CAREFUL_DUDE =	-Wall -Werror
NO_PRAYER_FOR_THE_WICKED =	-w -O2 
WARNINGS = 			$(PEDANTIC_PARANOID_FREAK)
CFLAGS = -g $(WARNINGS)

INCLUDES = -I$(INSTALL_PATH)/include
DEFS = -L$(INSTALL_PATH)/lib
LIBS = -lm -lsimgrid

all: sd_test
sd_test: sd_test.o

%: %.o
	$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) $^ $(LIBS) $(LDADD) -o $@

%.o: %.c
	$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *~
.SUFFIXES:
.PHONY : clean
