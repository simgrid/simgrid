SHELL=/bin/sh
BENCHMARK=ep
BENCHMARKU=EP

include ../config/make.def

#OBJS = ep.o ${COMMON}/print_results.o ${COMMON}/${RAND}.o ${COMMON}/timers.o
OBJS = ep.o randlc.o

include ../sys/make.common

${PROGRAM}: config ${OBJS}
#	${FLINK} ${FLINKFLAGS} -o ${PROGRAM} ${OBJS} ${FMPI_LIB}
	${CLINK} ${CLINKFLAGS} -o ${PROGRAM} ${OBJS} ${CMPI_LIB}


#ep.o:		ep.f  mpinpb.h npbparams.h
#	${FCOMPILE} ep.f

ep.o:	ep.c randlc.c mpinpb.h npbparams.h
	${CCOMPILE} ep.c

clean:
	- rm -f *.o *~ 
	- rm -f npbparams.h core



