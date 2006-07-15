# This works mainly on my box for now
export LD_LIBRARY_PATH=$(GRAS_ROOT)/lib

all: 1-bones.output 2-simple.output 3-args.output 4-callback.output

veryclean: clean
	rm *.output

# Lesson 1: simple bones of project
########################################

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

# Lesson 3: passing args to processes
########################################

3-args.output: 3-args_client 3-args_server 3-args_simulator
	echo '$$ ./test_server 12345 & ./test_client 127.0.0.1 12345'  > $@ 
	./3-args_server 12345                                         >> $@ 2>&1&
	./3-args_client 127.0.0.1 12345                               >> $@ 2>&1
	sleep 1
	echo '$$ ./test_simulator platform.xml test.xml'              >> $@
	./3-args_simulator gtut-platform.xml 3-args.xml               >> $@ 2>&1
	echo '$$'                                                     >> $@ 

3-args_client 3-args_server 3-args_simulator: _3-args_client.c _3-args_server.c _3-args_simulator.c
	make -f 3-args.mk

_3-args_client.c _3-args_server.c _3-args_simulator.c: 3-args.c 3-args.xml
	../../tools/gras/gras_stub_generator 3-args 3-args.xml >/dev/null

clean::
	if [ -e 3-args.mk ] ; then make -f 3-args.mk clean; fi
	rm -f _3-args_client.c _3-args_server.c _3-args_simulator.c 3-args.trace 3-args.mk

# Lesson 4: callbacks
########################################

4-callback.output: 4-callback_client 4-callback_server 4-callback_simulator
	echo '$$ ./test_server 12345 & ./test_client 127.0.0.1 12345'  > $@ 
	./4-callback_server 12345                                     >> $@ 2>&1&
	./4-callback_client 127.0.0.1 12345                           >> $@ 2>&1
	sleep 1
	echo '$$ ./test_simulator platform.xml test.xml'              >> $@
	./4-callback_simulator gtut-platform.xml 3-args.xml           >> $@ 2>&1
	echo '$$'                                                     >> $@ 

4-callback_client 4-callback_server 4-callback_simulator: _4-callback_client.c _4-callback_server.c _4-callback_simulator.c
	make -f 4-callback.mk

_4-callback_client.c _4-callback_server.c _4-callback_simulator.c: 4-callback.c 3-args.xml
	../../tools/gras/gras_stub_generator 4-callback 3-args.xml >/dev/null

clean::
	if [ -e 4-callback.mk ] ; then make -f 4-callback.mk clean; fi
	rm -f _4-callback_client.c _4-callback_server.c _4-callback_simulator.c 4-callback.trace 4-callback.mk

