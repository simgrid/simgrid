CXX = g++
PREFIX = /home/mcherier/com/loria/SimGridX
INCLUDES =-I$(PREFIX)/lib -I$(PREFIX)/examples/ping_pong
SRCDIR = $(PREFIX)/examples/ping_pong
CXXFLAGS = -Wall -pedantic -D_DEBUG $(INCLUDES)
LDFLAGS = -L$(PREFIX)/lib 
LDLIBS = -lSimGridX 

PROG = ping_pong

OBJECTS = PingPongTask.o Sender.o Receiver.o Main.o

all: $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $(PROG) 

%.o: $(SRCDIR)/%.cxx
	$(CXX) -c $(CXXFLAGS) $(SRCDIR)/$*.cxx
	
clean:
	rm *.o