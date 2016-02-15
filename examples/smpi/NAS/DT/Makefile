BENCHMARK=dt
include ../config/make.def
include ../sys/make.common

OBJS = DGraph.o ${COMMON}/c_print_results.o ${COMMON}/c_timers.o ${COMMON}/randdp.o

${PROGRAM}: config dt.o dt-folding.o ${OBJS}
	${CLINK} ${CLINKFLAGS} -o $(BINDIR)/dt.${CLASS} dt.o ${OBJS} ${CMPI_LIB}
	${CLINK} ${CLINKFLAGS} -o ${BINDIR}/dt-folding.${CLASS} dt-folding.o ${OBJS} ${CMPI_LIB}

.c.o:
	${CCOMPILE} $<

dt.o:             dt.c npbparams.h
dt-folding.o:     dt-folding.c npbparams.h
DGraph.o:         DGraph.c DGraph.h

clean:
	- rm -f *.o *~ npbparams.h
