LIBNAME = libSimGridX
PREFIX = /home/mcherier/com/loria/SimGridX/lib
INCLUDEDIR = $(PREFIX)/
SRCDIR = $(PREFIX)/
OBJECTS = Application.o ApplicationHandler.o BadAllocException.o ClassNotFoundException.o Environment.o Exception.o FileNotFoundException.o Host.o HostNotFoundException.o \
	InvalidArgumentException.o LogicException.o Msg.o MsgException.o NullPointerException.o Object.o OutOfBoundsException.o Process.o ProcessNotFoundException.o Simulation.o \
	StringHelper.o Task.o
CXX = g++
CXXFLAGS = -I$(INCLUDEDIR) -I/home/mcherier/svn/simgrid/include/ -I/home/mcherier/svn/simgrid/src/ -I/home/mcherier/svn/simgrid/src/include/ -D_DEBUG -DSIMGRIDX_EXPORTS -fPIC -Wall -ansi -pedantic
 
all: $(OBJECTS) 
	$(CXX) $(OBJECTS) -shared -o $(LIBNAME).so -L/home/mquinson/simgrid-svn-pthread/src/.libs/ -lsimgrid

	
%.o: $(SRCDIR)/%.cxx
	$(CXX) -c $(CXXFLAGS) $(SRCDIR)/$*.cxx
	
clean:
	rm *.o

