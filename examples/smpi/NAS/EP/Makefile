SHELL=/bin/sh
BENCHMARK=ep
BENCHMARKU=EP

include ../config/make.def

OBJS = ep.o randlc.o

include ../sys/make.common

${PROGRAM}: config ${OBJS}
	${CLINK} ${CLINKFLAGS} -o ${PROGRAM} ${OBJS} ${CMPI_LIB}

ep.o:	ep.c randlc.c mpinpb.h npbparams.h
	${CCOMPILE} ep.c

clean:
	- rm -f *.o *~ 
	- rm -f npbparams.h core



