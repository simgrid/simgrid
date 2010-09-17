SHELL=/bin/sh
BENCHMARK=ep
BENCHMARKU=EP

include ../config/make.def

#OBJS = ep-trace.o ${COMMON}/print_results.o ${COMMON}/${RAND}.o ${COMMON}/timers.o
OBJS = ep-trace.o randlc.o

include ../sys/make.common

${PROGRAM}: config ${OBJS}
#	${FLINK} ${FLINKFLAGS} -o ${PROGRAM} ${OBJS} ${FMPI_LIB}
	${CLINK} ${CLINKFLAGS} -o ${PROGRAM} ${OBJS} ${CMPI_LIB}


#ep-trace.o:		ep-trace.f  mpinpb.h npbparams.h
#	${FCOMPILE} ep-trace.f

ep-trace.o:	ep-trace.c randlc.c mpinpb.h npbparams.h
	${CCOMPILE} ep-trace.c

clean:
	- rm -f *.o *~ 
	- rm -f npbparams.h core



