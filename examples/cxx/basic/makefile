CXX = g++
PREFIX = /home/mcherier/com/loria/SimGridX
INCLUDES =-I$(PREFIX)/lib -I$(PREFIX)/examples/basic
SRCDIR = $(PREFIX)/examples/basic
CXXFLAGS = -Wall -pedantic -D_DEBUG $(INCLUDES)
LDFLAGS = -L$(PREFIX)/lib 
LDLIBS = -lSimGridX 

PROG = basic

OBJECTS = BasicTask.o FinalizeTask.o Slave.o Forwarder.o Master.o Main.o

all: $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $(PROG) 

%.o: $(SRCDIR)/%.cxx
	$(CXX) -c $(CXXFLAGS) $(SRCDIR)/$*.cxx
	
clean:
	rm *.o