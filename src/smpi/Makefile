CC=gcc #-DDEBUG
CFLAGS=-O3
SRCDIR=./src
BINDIR=./bin
LIBDIR=./lib
INCLUDEDIR=./include
BUILDDIR=./build

smpi: $(LIBDIR)/libsmpi.a $(BINDIR)/smpicc $(BINDIR)/smpirun

$(LIBDIR)/libsmpi.a: $(LIBDIR) $(BUILDDIR)/smpi_base.o $(BUILDDIR)/smpi_mpi.o Makefile
	ar rc $(LIBDIR)/libsmpi.a $(BUILDDIR)/smpi_base.o $(BUILDDIR)/smpi_mpi.o
	ranlib $(LIBDIR)/libsmpi.a

$(LIBDIR):
	mkdir -p $(LIBDIR)

$(BUILDDIR)/smpi_base.o: src/smpi_base.c include/smpi.h Makefile $(BUILDDIR)
	$(CC) $(CFLAGS) -c -I ../include -I $(INCLUDEDIR) -I ${SIMGRID_HOME}/include src/smpi_base.c -o $(BUILDDIR)/smpi_base.o

$(BUILDDIR)/smpi_mpi.o: src/smpi_mpi.c include/smpi.h Makefile $(BUILDDIR)
	$(CC) $(CFLAGS) -c -I ../include -I $(INCLUDEDIR) -I ${SIMGRID_HOME}/include src/smpi_mpi.c -o $(BUILDDIR)/smpi_mpi.o

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BINDIR)/smpicc: scripts/smpicc Makefile $(BINDIR)
	cp -p scripts/smpicc $(BINDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

$(BINDIR)/smpirun: scripts/smpirun Makefile $(BINDIR)
	cp -p scripts/smpirun $(BINDIR)

clean:
	rm -rf $(BINDIR) $(LIBDIR) $(BUILDDIR)
