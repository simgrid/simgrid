CXX = g++
PREFIX = /home/mcherier/com/loria/SimGridX
INCLUDES =-I$(PREFIX)/lib -I$(PREFIX)/examples/autoDestination
SRCDIR = $(PREFIX)/examples/autoDestination
CXXFLAGS = -Wall -pedantic -D_DEBUG $(INCLUDES)
LDFLAGS = -L$(PREFIX)/lib 
LDLIBS = -lSimGridX 

PROG = autoDestination

OBJECTS = BasicTask.o FinalizeTask.o Forwarder.o Slave.o Master.o Main.o

all: $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $(PROG) 

%.o: $(SRCDIR)/%.cxx
	$(CXX) -c $(CXXFLAGS) $(SRCDIR)/$*.cxx
	
clean:
	rm *.o