SHELL=/bin/sh
BENCHMARK=dt

include ../config/make.def

include ../sys/make.common
#Override PROGRAM
DTPROGRAM  = $(BINDIR)/$(BENCHMARK).$(CLASS)

OBJS = dt.o  DGraph.o \
	${COMMON}/c_print_results.o ${COMMON}/c_timers.o ${COMMON}/c_randdp.o

OBJS-F = dt-folding.o DGraph.o \
	${COMMON}/c_print_results.o ${COMMON}/c_timers.o ${COMMON}/c_randdp.o


${PROGRAM}: config ${OBJS} ${OBJS-F}
	${CLINK} ${CLINKFLAGS} -o ${DTPROGRAM} ${OBJS} ${CMPI_LIB}
	${CLINK} ${CLINKFLAGS} -o ${DTPROGRAM}-folding ${OBJS-F} ${CMPI_LIB}

.c.o:
	${CCOMPILE} $<

dt.o:             dt.c  npbparams.h
dt-folding.o:     dt-folding.c  npbparams.h
DGraph.o:	DGraph.c DGraph.h

clean:
	- rm -f *.o *~ mputil*
	- rm -f dt dt-folding npbparams.h
