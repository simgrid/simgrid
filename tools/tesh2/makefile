PREFIX = /home/mcherier/com/loria/tesh
INCLUDE_DIR = $(PREFIX)/include/
SRC_DIR = $(PREFIX)/src/
OBJECTS = main.o command.o context.o reader.o runner.o signal.o timer.o suite.o unit.o writer.o list.o lstrings.o allocator.o vector.o htable.o dictionary.o units.o directory.o directories.o fstream.o fstreams.o excludes.o error.o
CC = gcc
CCFLAGS = -ggdb -g -I$(INCLUDE_DIR) -I/home/mcherier/svn/simgrid/include/ -I/home/mcherier/svn/simgrid/src/ -I/home/mcherier/svn/simgrid/src/include/ -Wall -pedantic
LDLIBS = simgrid 

PROG = tesh

all: $(PROG)

$(PROG): $(OBJECTS)
	$(CC) -o $(PROG) $(OBJECTS) -L/home/mquinson/simgrid-svn/src/.libs/ -l$(LDLIBS)

%.o: $(SRC_DIR)%.c $(INCLUDE_DIR)*.h
	$(CC) -c $(CCFLAGS) $(SRC_DIR)$*.c
	
clean:
	rm -f $(OBJECTS) $(PROG)
	
	