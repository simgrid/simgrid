PREFIX = /home/mcherier/com/loria/tesh
INCLUDE_DIR = $(PREFIX)/include/
SRC_DIR = $(PREFIX)/src/
OBJECTS = is_cmd.o explode.o readline.o main.o command.o context.o reader.o runner.o xsignal.o timer.o unit.o writer.o units.o directory.o directories.o fstream.o fstreams.o excludes.o xerrno.o getpath.o variable.o str_replace.o
CC = gcc
CCFLAGS =  -D__CHKCMD -D__BUILTIN -ggdb -g -I$(INCLUDE_DIR) -I/home/mcherier/svn/simgrid/include/ -I/home/mcherier/svn/simgrid/src/ -I/home/mcherier/svn/simgrid/src/include/ -Wall -pedantic
LDLIBS = simgrid 

PROG = tesh

all: $(PROG)

$(PROG): $(OBJECTS)
	$(CC) -o $(PROG) $(OBJECTS) -L/home/mquinson/simgrid-svn-pthread/src/.libs/ -l$(LDLIBS)

%.o: $(SRC_DIR)%.c $(INCLUDE_DIR)*.h
	$(CC) -c $(CCFLAGS) $(SRC_DIR)$*.c
	
clean:
	rm -f $(OBJECTS) $(PROG)
	
	