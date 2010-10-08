# This works mainly on my box for now
export LD_LIBRARY_PATH=$(GRAS_ROOT)/lib
GRAS_STUB_GENERATOR=$(GRAS_ROOT)/bin/gras_stub_generator

all: 01-bones.output 02-simple.output 03-args.output 04-callback.output \
     05-globals.output 06-logs.output 07-timers.output 08-exceptions.output \
     09-simpledata.output 10-rpc.output 11-explicitwait.output

veryclean: clean
	rm *.output*

# Lesson 01: simple bones of project
########################################

01-bones.output: 01-bones_client 01-bones_server 01-bones_simulator
	echo '$$ ./test_client'                           > $@ 
	./01-bones_client                                 >> $@ 2>&1
	echo '$$ ./test_server'                          >> $@
	./01-bones_server                                 >> $@ 2>&1
	echo '$$'                                        >> $@ 
	echo '$$ ./test_simulator platform.xml test.xml' >> $@ 
	./01-bones_simulator gtut-platform.xml test.xml   >> $@ 2>&1
	echo '$$'                                        >> $@ 

01-bones_client 01-bones_server 01-bones_simulator: _01-bones_client.c _01-bones_server.c _01-bones_simulator.c
	make -f 01-bones.mk

_01-bones_client.c _01-bones_server.c _01-bones_simulator.c: 01-bones.c test.xml
	$(GRAS_STUB_GENERATOR) 01-bones test.xml >/dev/null

clean::
	if [ -e 01-bones.mk ] ; then make -f 01-bones.mk clean; fi
	rm -f _01-bones_client.c _01-bones_server.c _01-bones_simulator.c 01-bones.trace 01-bones.mk

# Lesson 02: simple message exchange
########################################

02-simple.output: 02-simple_client 02-simple_server 02-simple_simulator
	echo '$$ ./test_simulator platform.xml test.xml'  > $@
	./02-simple_simulator gtut-platform.xml test.xml  >> $@ 2>&1
	echo '$$'                                        >> $@ 

02-simple_client 02-simple_server 02-simple_simulator: _02-simple_client.c _02-simple_server.c _02-simple_simulator.c
	make -f 02-simple.mk

_02-simple_client.c _02-simple_server.c _02-simple_simulator.c: 02-simple.c test.xml
	$(GRAS_STUB_GENERATOR) 02-simple test.xml >/dev/null

clean::
	if [ -e 02-simple.mk ] ; then make -f 02-simple.mk clean; fi
	rm -f _02-simple_client.c _02-simple_server.c _02-simple_simulator.c 02-simple.trace 02-simple.mk

# Lesson 03: passing args to processes
########################################

03-args.output: 03-args_client 03-args_server 03-args_simulator
	echo '$$ ./test_server 12345 & ./test_client 127.0.0.1 12345'  > $@ 
	./03-args_server 12345                                         >> $@ 2>&1&
	./03-args_client 127.0.0.1 12345                               >> $@ 2>&1
	sleep 1
	echo '$$'                                                     >> $@
	echo '$$ ./test_simulator platform.xml test.xml'              >> $@
	./03-args_simulator gtut-platform.xml 03-args.xml               >> $@ 2>&1
	echo '$$'                                                     >> $@ 
	killall 03-args_server 03-args_client 2>/dev/null || true

03-args_client 03-args_server 03-args_simulator: _03-args_client.c _03-args_server.c _03-args_simulator.c
	make -f 03-args.mk

_03-args_client.c _03-args_server.c _03-args_simulator.c: 03-args.c 03-args.xml
	$(GRAS_STUB_GENERATOR) 03-args 03-args.xml >/dev/null

clean::
	if [ -e 03-args.mk ] ; then make -f 03-args.mk clean; fi
	rm -f _03-args_client.c _03-args_server.c _03-args_simulator.c 03-args.trace 03-args.mk

# Lesson 4: callbacks
########################################

04-callback.output: 04-callback_client 04-callback_server 04-callback_simulator
	echo '$$ ./test_server 23451 & ./test_client 127.0.0.1 23451'  > $@ 
	./04-callback_server 23451                                     >> $@ 2>&1&
	./04-callback_client 127.0.0.1 23451                           >> $@ 2>&1
	sleep 1
	echo '$$'                                                     >> $@
	echo '$$ ./test_simulator platform.xml test.xml'              >> $@
	./04-callback_simulator gtut-platform.xml 03-args.xml           >> $@ 2>&1
	echo '$$'                                                     >> $@
	killall 04-callback_server 04-callback_client 2>/dev/null || true

04-callback_client 04-callback_server 04-callback_simulator: _04-callback_client.c _04-callback_server.c _04-callback_simulator.c
	make -f 04-callback.mk

_04-callback_client.c _04-callback_server.c _04-callback_simulator.c: 04-callback.c 03-args.xml
	$(GRAS_STUB_GENERATOR) 04-callback 03-args.xml >/dev/null

clean::
	if [ -e 04-callback.mk ] ; then make -f 04-callback.mk clean; fi
	rm -f _04-callback_client.c _04-callback_server.c _04-callback_simulator.c 04-callback.trace 04-callback.mk

# Lesson 5: globals
########################################

05-globals.output: 05-globals_client 05-globals_server 05-globals_simulator
	echo '$$ ./test_server 12345 & ./test_client 127.0.0.1 12345'  > $@ 
	./05-globals_server 12345                                      >> $@ 2>&1&
	./05-globals_client 127.0.0.1 12345                            >> $@ 2>&1
	sleep 1
	echo '$$'                                                     >> $@
	echo '$$ ./test_simulator platform.xml test.xml'              >> $@
	./05-globals_simulator gtut-platform.xml 03-args.xml            >> $@ 2>&1
	echo '$$'                                                     >> $@ 
	killall 05-globals_server 05-globals_client 2>/dev/null || true

05-globals_client 05-globals_server 05-globals_simulator: _05-globals_client.c _05-globals_server.c _05-globals_simulator.c
	make -f 05-globals.mk

_05-globals_client.c _05-globals_server.c _05-globals_simulator.c: 05-globals.c 03-args.xml
	$(GRAS_STUB_GENERATOR) 05-globals 03-args.xml >/dev/null

clean::
	if [ -e 05-globals.mk ] ; then make -f 05-globals.mk clean; fi
	rm -f _05-globals_client.c _05-globals_server.c _05-globals_simulator.c 05-globals.trace 05-globals.mk

# Lesson 6: logs
########################################

06-logs.output: 06-logs_client 06-logs_server 06-logs_simulator \
        06-logs.output.fmt 06-logs.output.fmt-bt 06-logs.output.verbose 06-logs.output.error
	echo '$$ ./test_server 12345 & ./test_client 127.0.0.1 12345'  > $@ 
	./06-logs_server 12345                             2>&1 |sed s/06-logs/test/  >> $@ 2>&1&
	./06-logs_client 127.0.0.1 12345                   2>&1 |sed s/06-logs/test/  >> $@ 2>&1
	sleep 1
	echo '$$'                                                     >> $@
	echo '$$ ./test_simulator platform.xml test.xml'              >> $@
	./06-logs_simulator gtut-platform.xml 03-args.xml   2>&1 |sed s/06-logs/test/  >> $@ 2>&1
	echo '$$'                                                     >> $@ 
	killall 06-logs_server 06-logs_client 2>/dev/null || true

06-logs.output.fmt: 06-logs_client 06-logs_server 06-logs_simulator 
	echo '$$ ./test_server 12345 --log=test.fmt:%m%n & ./test_client 127.0.0.1 12345 --log=test.fmt:%m%n'  > $@
	./06-logs_server 12345  --log=test.fmt:%m%nn          2>&1 |sed s/06-logs/test/  >> $@ 2>&1&
	./06-logs_client 127.0.0.1 12345 --log=test.fmt:%m%n 2>&1 |sed s/06-logs/test/  >> $@ 2>&1
	sleep 1
	echo '$$'                                                     >> $@
	echo '$$ ./test_simulator platform.xml test.xml --log=test.fmt:%m%n'              >> $@
	./06-logs_simulator gtut-platform.xml 03-args.xml  --log=test.fmt:%m%n 2>&1 |sed s/06-logs/test/  >> $@ 2>&1
	echo '$$'                                                     >> $@ 
	killall 06-logs_server 06-logs_client 2>/dev/null || true

06-logs.output.fmt-bt: 06-logs_client 06-logs_server 06-logs_simulator 
	echo '$$ ./test_server 12345 --log=test.fmt:"[%r] %m%n%b%n%n" & ./test_client 127.0.0.1 12345 --log=test.fmt:"[%r] %m%n%b%n%n"'  > $@
	./06-logs_server 12345  --log=test.fmt:[%r]%m%n%b%n%n          2>&1 |sed s/06-logs/test/  >> $@ 2>&1&
	./06-logs_client 127.0.0.1 12345 --log=test.fmt:[%r]%m%n%b%n%n 2>&1 |sed s/06-logs/test/  >> $@ 2>&1
	sleep 1
	echo '$$'                                                     >> $@
	echo '$$ ./test_simulator platform.xml test.xml --log=test.fmt:[%r]%m%n%b%n%n'              >> $@
	./06-logs_simulator gtut-platform.xml 03-args.xml  --log=test.fmt:[%r]%m%n%b%n%n 2>&1 |sed s/06-logs/test/  >> $@ 2>&1
	echo '$$'                                                     >> $@ 
	killall 06-logs_server 06-logs_client 2>/dev/null || true

06-logs.output.verbose: 06-logs_client 06-logs_server 06-logs_simulator
	echo '$$ ./test_server 12345 --log=test.thres:verbose & ./test_client 127.0.0.1 12345 --log=test.thres:verbose'  > $@
	./06-logs_server 12345 --log=test.thres:verbose                            2>&1 |sed s/06-logs/test/  >> $@ 2>&1&
	./06-logs_client 127.0.0.1 12345 --log=test.thres:verbose                  2>&1 |sed s/06-logs/test/  >> $@ 2>&1
	sleep 1
	echo '$$'                                                     >> $@
	echo '$$ ./test_simulator platform.xml test.xml --log=test.thres:verbose'              >> $@
	./06-logs_simulator gtut-platform.xml 03-args.xml --log=test.thres:verbose  2>&1 |sed s/06-logs/test/  >> $@ 2>&1
	echo '$$'                                                     >> $@ 
	killall 06-logs_server 06-logs_client 2>/dev/null || true

06-logs.output.error: 06-logs_client 06-logs_server 06-logs_simulator
	echo '$$ ./test_server 12345 --log=root.thres:error & ./test_client 127.0.0.1 12345 --log=root.thres:error'  > $@
	./06-logs_server 12345 --log=root.thres:error                            2>&1 |sed s/06-logs/test/  >> $@ 2>&1&
	./06-logs_client 127.0.0.1 12345 --log=root.thres:error                  2>&1 |sed s/06-logs/test/  >> $@ 2>&1
	sleep 1
	echo '$$'                                                     >> $@
	echo '$$ ./test_simulator platform.xml test.xml --log=root.thres:error'              >> $@
	./06-logs_simulator gtut-platform.xml 03-args.xml --log=root.thres:error  2>&1 |sed s/06-logs/test/  >> $@ 2>&1
	echo '$$'                                                     >> $@ 
	killall 06-logs_server 06-logs_client 2>/dev/null || true


06-logs_client 06-logs_server 06-logs_simulator: _06-logs_client.c _06-logs_server.c _06-logs_simulator.c
	make -f 06-logs.mk

_06-logs_client.c _06-logs_server.c _06-logs_simulator.c: 06-logs.c 03-args.xml
	$(GRAS_STUB_GENERATOR) 06-logs 03-args.xml >/dev/null

clean::
	if [ -e 06-logs.mk ] ; then make -f 06-logs.mk clean; fi
	rm -f _06-logs_client.c _06-logs_server.c _06-logs_simulator.c 06-logs.trace 06-logs.mk


# Lesson 7: timers
########################################

07-timers.output: 07-timers_client 07-timers_server 07-timers_simulator
	echo '$$ ./test_server 12345 & ./test_client 127.0.0.1 12345'  > $@ 
	./07-timers_server 12345                             2>&1 |sed s/07-timers/test/  >> $@ 2>&1&
	./07-timers_client 127.0.0.1 12345                   2>&1 |sed s/07-timers/test/  >> $@ 2>&1
	sleep 1
	echo '$$'                                                     >> $@
	echo '$$ ./test_simulator platform.xml test.xml'              >> $@
	./07-timers_simulator gtut-platform.xml 03-args.xml   2>&1 |sed s/07-timers/test/  >> $@ 2>&1
	echo '$$'                                                     >> $@ 
	killall 07-timers_server 07-timers_client 2>/dev/null || true

07-timers_client 07-timers_server 07-timers_simulator: _07-timers_client.c _07-timers_server.c _07-timers_simulator.c
	make -f 07-timers.mk

_07-timers_client.c _07-timers_server.c _07-timers_simulator.c: 07-timers.c 03-args.xml
	$(GRAS_STUB_GENERATOR) 07-timers 03-args.xml >/dev/null

clean::
	if [ -e 07-timers.mk ] ; then make -f 07-timers.mk clean; fi
	rm -f _07-timers_client.c _07-timers_server.c _07-timers_simulator.c 07-timers.trace 07-timers.mk

# Lesson 8: exceptions
########################################

08-exceptions.output: 08-exceptions_client 08-exceptions_server 08-exceptions_simulator
	echo '$$ ./test_server & ./test_client 127.0.0.1 '             > $@
	./08-exceptions_server                                   2>&1 |sed s/08-exceptions/test/  >> $@ 2>&1&
	./08-exceptions_client 127.0.0.1                         2>&1 |sed s/08-exceptions/test/  >> $@ 2>&1
	sleep 1
	echo '$$'                                                     >> $@
	echo '$$ ./test_server --cheat & ./test_client 127.0.0.1 '    >> $@
	./08-exceptions_server --cheat                           2>&1 |sed s/08-exceptions/test/  >> $@ 2>&1&
	./08-exceptions_client 127.0.0.1                         2>&1 |sed s/08-exceptions/test/  >> $@ 2>&1
	sleep 1
	echo '$$ killall test_server'                                 >> $@
	killall 08-exceptions_server 08-exceptions_client 2>/dev/null || true
	echo '$$'                                                     >> $@
	echo '$$ ./test_simulator platform.xml test.xml'              >> $@
	./08-exceptions_simulator gtut-platform.xml 03-args.xml   2>&1 |sed s/08-exceptions/test/  >> $@ 2>&1
	echo '$$'                                                     >> $@ 

08-exceptions_client 08-exceptions_server 08-exceptions_simulator: _08-exceptions_client.c _08-exceptions_server.c _08-exceptions_simulator.c
	make -f 08-exceptions.mk

_08-exceptions_client.c _08-exceptions_server.c _08-exceptions_simulator.c: 08-exceptions.c 03-args.xml
	$(GRAS_STUB_GENERATOR) 08-exceptions 03-args.xml >/dev/null

clean::
	if [ -e 08-exceptions.mk ] ; then make -f 08-exceptions.mk clean; fi
	rm -f _08-exceptions_client.c _08-exceptions_server.c _08-exceptions_simulator.c 08-exceptions.trace 08-exceptions.mk 08-exceptions.output

# Lesson 9: simple data exchange
########################################
09-datatype-dump: 09-datatype-dump.c
	gcc -I$(GRAS_ROOT)/include -lgras -L$(GRAS_ROOT)/lib $^ -o $@ 

clean::
	rm -f 09-datatype-dump.o 09-datatype-dump

09-simpledata.output: 09-simpledata_client 09-simpledata_server 09-simpledata_simulator
	echo '$$ ./test_server 12345 & ./test_client 127.0.0.1 12345'  > $@ 
	./09-simpledata_server 12345                             2>&1 |sed s/09-simpledata/test/  >> $@ 2>&1&
	./09-simpledata_client 127.0.0.1 12345                   2>&1 |sed s/09-simpledata/test/  >> $@ 2>&1
	sleep 1
	echo '$$'                                                     >> $@
	echo '$$ ./test_simulator platform.xml test.xml'              >> $@
	./09-simpledata_simulator gtut-platform.xml 03-args.xml   2>&1 |sed s/09-simpledata/test/  >> $@ 2>&1
	echo '$$'                                                     >> $@ 
	killall 09-simpledata_server 09-simpledata_client 2>/dev/null || true

09-simpledata_client 09-simpledata_server 09-simpledata_simulator: _09-simpledata_client.c _09-simpledata_server.c _09-simpledata_simulator.c
	make -f 09-simpledata.mk

_09-simpledata_client.c _09-simpledata_server.c _09-simpledata_simulator.c: 09-simpledata.c 03-args.xml
	$(GRAS_STUB_GENERATOR) 09-simpledata 03-args.xml >/dev/null

clean::
	if [ -e 09-simpledata.mk ] ; then make -f 09-simpledata.mk clean; fi
	rm -f _09-simpledata_client.c _09-simpledata_server.c _09-simpledata_simulator.c 09-simpledata.trace 09-simpledata.mk

# Lesson 10: RPC
########################################
10-rpc.output: 10-rpc_client 10-rpc_server 10-rpc_simulator
	echo '$$ ./test_server & ./test_client 127.0.0.1 '             > $@
	./10-rpc_server 12345                             2>&1 |sed s/10-rpc/test/  >> $@ 2>&1&
	./10-rpc_client 127.0.0.1 12345                   2>&1 |sed s/10-rpc/test/  >> $@ 2>&1
	sleep 1
	echo '$$'                                                     >> $@
	echo '$$ killall test_server'                                 >> $@
	killall 10-rpc_server 10-rpc_client 2>/dev/null || true
	echo '$$'                                                     >> $@
	echo '$$ ./test_simulator platform.xml test.xml'              >> $@
	./10-rpc_simulator gtut-platform.xml 03-args.xml   2>&1 |sed s/10-rpc/test/  >> $@ 2>&1
	echo '$$'                                                     >> $@ 

10-rpc_client 10-rpc_server 10-rpc_simulator: _10-rpc_client.c _10-rpc_server.c _10-rpc_simulator.c
	make -f 10-rpc.mk

_10-rpc_client.c _10-rpc_server.c _10-rpc_simulator.c: 10-rpc.c 03-args.xml
	$(GRAS_STUB_GENERATOR) 10-rpc 03-args.xml >/dev/null

clean::
	if [ -e 10-rpc.mk ] ; then make -f 10-rpc.mk clean; fi
	rm -f _10-rpc_client.c _10-rpc_server.c _10-rpc_simulator.c 10-rpc.trace 10-rpc.mk


# Lesson 11: Explicit wait
########################################
11-explicitwait.output: 11-explicitwait_client 11-explicitwait_server 11-explicitwait_simulator
	(echo '$$ ./test_server & ./test_client 127.0.0.1 12345 & ./test_client 127.0.0.1 12345 '; \
	 ./11-explicitwait_client 127.0.0.1 12345            & \
	 ./11-explicitwait_client 127.0.0.1 12345            & \
	 ./11-explicitwait_server 12345                        \
	) 2>&1 | sed s/11-explicitwait/test/ > $@
	sleep 1
	echo '$$'                                                     >> $@
	killall 11-explicitwait_server 11-explicitwait_client 2>/dev/null || true
	echo '$$'                                                     >> $@
	echo '$$ ./test_simulator platform-3nodes.xml test.xml'              >> $@
	./11-explicitwait_simulator gtut-platform-3nodes.xml 11-explicitwait.xml   2>&1 |sed s/11-explicitwait/test/  >> $@ 2>&1
	echo '$$'                                                     >> $@ 

11-explicitwait_client 11-explicitwait_server 11-explicitwait_simulator: _11-explicitwait_client.c _11-explicitwait_server.c _11-explicitwait_simulator.c
	make -f 11-explicitwait.mk

_11-explicitwait_client.c _11-explicitwait_server.c _11-explicitwait_simulator.c: 11-explicitwait.c 03-args.xml
	$(GRAS_STUB_GENERATOR) 11-explicitwait 03-args.xml >/dev/null

clean::
	if [ -e 11-explicitwait.mk ] ; then make -f 11-explicitwait.mk clean; fi
	rm -f _11-explicitwait_client.c _11-explicitwait_server.c _11-explicitwait_simulator.c 11-explicitwait.trace 11-explicitwait.mk


