/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.h"

using namespace simgrid;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "a sample log category");

class Worker : s4u::Process {
public:
	Worker(const char*procname, s4u::Host *host,int argc, char **argv)
			: s4u::Process(procname,host,argc,argv){}

	int main(int argc, char **argv) {
		XBT_INFO("Hello s4u, I'm ready to serve");

		char *msg = recvstr("worker");
		XBT_INFO("I received '%s'",msg);
		XBT_INFO("I'm done. See you.");
		return 1;
	}
};

class Master : s4u::Process {
public:
	Master(const char*procname, s4u::Host *host,int argc, char **argv)
			: s4u::Process(procname,host,argc,argv){}

	int main(int argc, char **argv) {
		XBT_INFO("Hello s4u, I have something to send");
		sendstr("worker","GaBuZoMeu");

		XBT_INFO("I'm done. See you.");
		return 1;
	}
};


int main(int argc, char **argv) {
	s4u::Engine *e = new s4u::Engine(&argc,argv);
	e->loadPlatform("../../platforms/two_hosts_platform.xml");

	new Worker("worker", s4u::Host::byName("host0"), 0, NULL);
	new Master("master", s4u::Host::byName("host1"), 0, NULL);
	e->run();
	return 0;
}
