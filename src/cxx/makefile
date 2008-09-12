LIBNAME = libSimGridX
PREFIX = /home/mcherier/com/loria/SimGridX/lib
INCLUDEDIR = $(PREFIX)/
SRCDIR = $(PREFIX)/
OBJECTS = MsgApplication.o MsgApplicationHandler.o BadAllocException.o ClassNotFoundException.o MsgEnvironment.o Exception.o FileNotFoundException.o MsgHost.o HostNotFoundException.o \
	InvalidArgumentException.o LogicException.o Msg.o MsgException.o NullPointerException.o Object.o OutOfBoundsException.o MsgProcess.o ProcessNotFoundException.o MsgSimulation.o \
	StringHelper.o MsgTask.o
CXX = g++
CXXFLAGS = -I$(INCLUDEDIR) -I/home/mcherier/svn/simgrid/include/ -I/home/mcherier/svn/simgrid/src/ -I/home/mcherier/svn/simgrid/src/include/ -D_DEBUG -DSIMGRIDX_EXPORTS -fPIC -Wall -ansi -pedantic
 
all: $(OBJECTS) 
	$(CXX) $(OBJECTS) -shared -o $(LIBNAME).so -L/home/mquinson/simgrid-svn-pthread/src/.libs/ -lsimgrid

	
%.o: $(SRCDIR)/%.cxx
	$(CXX) -c $(CXXFLAGS) $(SRCDIR)/$*.cxx
	
clean:
	rm *.o

