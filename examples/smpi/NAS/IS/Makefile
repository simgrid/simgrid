BENCHMARK=is

include ../config/make.def
include ../sys/make.common

OBJS = is.o ${COMMON}/c_print_results.o

${PROGRAM}: config ${OBJS} 
	${CLINK} ${CLINKFLAGS} -o ${PROGRAM} ${OBJS} ${CMPI_LIB}

.c.o: 
	${CCOMPILE} $<

is.o: is.c npbparams.h

clean:
	- rm -f *.o *~ is npbparams.h
