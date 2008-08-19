CXX = g++
PREFIX = /home/mcherier/com/loria/SimGridX
INCLUDES =-I$(PREFIX)/lib -I$(PREFIX)/examples/comm_time
SRCDIR = $(PREFIX)/examples/comm_time
CXXFLAGS = -Wall -pedantic -D_DEBUG $(INCLUDES)
LDFLAGS = -L$(PREFIX)/lib 
LDLIBS = -lSimGridX 

PROG = comm_time

OBJECTS = CommTimeTask.o FinalizeTask.o Slave.o Master.o Main.o

all: $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $(PROG) 

%.o: $(SRCDIR)/%.cxx
	$(CXX) -c $(CXXFLAGS) $(SRCDIR)/$*.cxx
	
clean:
	rm *.o