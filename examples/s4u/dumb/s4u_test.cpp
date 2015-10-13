/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.h"

using namespace simgrid;
using namespace s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "a sample log category");

class Worker : Actor {
public:
	Worker(const char*procname, Host *host,int argc, char **argv)
			: s4u::Actor(procname,host,argc,argv){}

	int main(int argc, char **argv) {
		XBT_INFO("Hello s4u, I'm ready to serve");

		char *msg = (char*)recv(*Mailbox::byName("worker"));
		XBT_INFO("I received '%s'",msg);
		XBT_INFO("I'm done. See you.");
		return 1;
	}
};

class Master : Actor {
public:
	Master(const char*procname, Host *host,int argc, char **argv)
			: Actor(procname,host,argc,argv){}

	int main(int argc, char **argv) {
		const char *msg = "GaBuZoMeu";
		XBT_INFO("Hello s4u, I have something to send");
		send(*Mailbox::byName("worker"), xbt_strdup(msg), strlen(msg));

		XBT_INFO("I'm done. See you.");
		return 1;
	}
};


int main(int argc, char **argv) {
	Engine *e = new Engine(&argc,argv);
	e->loadPlatform("../../platforms/two_hosts_platform.xml");

	new Worker("worker", Host::byName("host0"), 0, NULL);
	new Master("master", Host::byName("host1"), 0, NULL);
	e->run();
	return 0;
}
