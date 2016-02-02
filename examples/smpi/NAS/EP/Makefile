SHELL=/bin/sh
BENCHMARK=ep
BENCHMARKU=EP

include ../config/make.def

OBJS = ep.o randlc.o
OBJS-S = ep-sampling.o randlc.o

include ../sys/make.common

${PROGRAM}: config ${OBJS} ${OBJS-S}
	${CLINK} ${CLINKFLAGS} -o ${PROGRAM} ${OBJS} ${CMPI_LIB} -lm
	${CLINK} ${CLINKFLAGS} -o ${PROGRAM}-sampling ${OBJS-S} ${CMPI_LIB} -lm

ep.o:	ep.c randlc.c mpinpb.h npbparams.h
	${CCOMPILE} ep.c
ep-sampling.o:	ep-sampling.c randlc.c mpinpb.h npbparams.h
	${CCOMPILE} ep-sampling.c

clean:
	- rm -f *.o *~ 
	- rm -f npbparams.h



