# This works mainly on my box for now
export LD_LIBRARY_PATH=$(GRAS_ROOT)/lib

all: 1-bones 2-simple

veryclean: clean
	rm *.output

# Lesson 1: simple bones of project
########################################

1-bones: 1-bones.output
1-bones.output: 1-bones_client 1-bones_server 1-bones_simulator
	echo '$$ ./test_client'                           > $@ 
	./1-bones_client                                 >> $@ 2>&1
	echo '$$ ./test_server'                          >> $@
	./1-bones_server                                 >> $@ 2>&1
	echo '$$ ./test_simulator platform.xml test.xml' >> $@ 
	./1-bones_simulator gtut-platform.xml test.xml   >> $@ 2>&1
	echo '$$'                                        >> $@ 

1-bones_client 1-bones_server 1-bones_simulator: _1-bones_client.c _1-bones_server.c _1-bones_simulator.c
	make -f 1-bones.mk

_1-bones_client.c _1-bones_server.c _1-bones_simulator.c: 1-bones.c test.xml
	../../tools/gras/gras_stub_generator 1-bones test.xml >/dev/null

clean::
	if [ -e 1-bones.mk ] ; then make -f 1-bones.mk clean; fi
	rm -f _1-bones_client.c _1-bones_server.c _1-bones_simulator.c 1-bones.trace 1-bones.mk

# Lesson 2: simple message exchange
########################################

2-simple: 2-simple.output
2-simple.output: 2-simple_client 2-simple_server 2-simple_simulator
	echo '$$ ./test_simulator platform.xml test.xml'  > $@
	./2-simple_simulator gtut-platform.xml test.xml  >> $@ 2>&1
	echo '$$'                                        >> $@ 

2-simple_client 2-simple_server 2-simple_simulator: _2-simple_client.c _2-simple_server.c _2-simple_simulator.c
	make -f 2-simple.mk

_2-simple_client.c _2-simple_server.c _2-simple_simulator.c: 2-simple.c test.xml
	../../tools/gras/gras_stub_generator 2-simple test.xml >/dev/null

clean::
	if [ -e 2-simple.mk ] ; then make -f 2-simple.mk clean; fi
	rm -f _2-simple_client.c _2-simple_server.c _2-simple_simulator.c 2-simple.trace 2-simple.mk

.PHONY: 1-bones 2-simple
