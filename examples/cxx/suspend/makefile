CXX = g++
PREFIX = /home/mcherier/com/loria/SimGridX
INCLUDES =-I$(PREFIX)/lib -I$(PREFIX)/examples/suspend
SRCDIR = $(PREFIX)/examples/suspend
CXXFLAGS = -Wall -pedantic -D_DEBUG $(INCLUDES)
LDFLAGS = -L$(PREFIX)/lib 
LDLIBS = -lSimGridX 

PROG = suspend

OBJECTS = DreamMaster.o LazyGuy.o Main.o

all: $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $(PROG) 

%.o: $(SRCDIR)/%.cxx
	$(CXX) -c $(CXXFLAGS) $(SRCDIR)/$*.cxx
	
clean:
	rm *.o