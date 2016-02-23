BENCHMARK=ep
include ../config/make.def
include ../sys/make.common

${PROGRAM}: config ep.o ep-sampling.o ../common/randdp.o
	${CLINK} ${CLINKFLAGS} -o ${BINDIR}/ep.${CLASS}.${NPROCS} ep.o ../common/randdp.o ${CMPI_LIB} -lm
	${CLINK} ${CLINKFLAGS} -o ${BINDIR}/ep-sampling.${CLASS}.${NPROCS} ep-sampling.o ../common/randdp.o ${CMPI_LIB} -lm

ep.o:	ep.c npbparams.h
	${CCOMPILE} ep.c
ep-sampling.o:	ep-sampling.c npbparams.h
	${CCOMPILE} ep-sampling.c

clean:
	- rm -f *.o *~ npbparams.h
