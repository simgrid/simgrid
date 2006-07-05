include ../Makedefs

default: all

MIXTESIM = ../bin/mixtesim

MIXTESIM_OBJECTS = ./heft.o\
		   ./main.o \
		   ./dag.o  \
		   ./list_scheduling_util.o

all: $(MIXTESIM) 

$(MIXTESIM):	$(MIXTESIM_OBJECTS)
		$(LD) $(MIXTESIM_OBJECTS) -o $(MIXTESIM) $(SIMGRIDLIB) \
		$(APPLESEEDSLIB) -lm

clean:
	/bin/rm -f *.o *~
