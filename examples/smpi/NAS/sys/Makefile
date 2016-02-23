include ../config/make.def
all: setparams 

# setparams creates an npbparam.h file for each benchmark configuration. 
# npbparams.h also contains info about how a benchmark was compiled and linked

setparams: setparams.c ../config/make.def
	$(CC) ${CONVERTFLAG} -o setparams setparams.c

clean: 
	-rm -f setparams setparams.h npbparams.h *~ *.o

