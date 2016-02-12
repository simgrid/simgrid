SHELL=/bin/sh
CLASS=S
NPROCS=1
SUBTYPE=
VERSION=

default: header
	@ sys/print_instructions

IS: is
is: header
	cd IS; $(MAKE) NPROCS=$(NPROCS) CLASS=$(CLASS)


EP: ep
ep: header
	cd EP; $(MAKE) NPROCS=$(NPROCS) CLASS=$(CLASS)

DT: dt
dt: header
	cd DT; $(MAKE) CLASS=$(CLASS)

# It would be nice to make clean in each subdirectory (the targets
# are defined) but on a really clean system this will won't work
# because those makefiles need config/make.def
clean:
	- rm -f *~  */*~ */*.o */npbparams.h 
	- rm -f sys/setparams sys/setparams.h

veryclean: clean
	- rm -f config/make.def  
	- rm -f bin/is.* bin/ep.* bin/dt.*

header:
	@ sys/print_header
