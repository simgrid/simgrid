SHELL=/bin/sh
CLASS=S
NPROCS=1
default:
	@ sys/print_instructions

IS: is
is: 
	cd IS; $(MAKE) NPROCS=$(NPROCS) CLASS=$(CLASS)

EP: ep
ep:
	cd EP; $(MAKE) NPROCS=$(NPROCS) CLASS=$(CLASS)

DT: dt
dt: 
	cd DT; $(MAKE) CLASS=$(CLASS)

clean:
	- rm -f *~  */*~ */*.o */npbparams.h 
	- rm -f sys/setparams sys/setparams.h

veryclean: clean
	- rm -f bin/*