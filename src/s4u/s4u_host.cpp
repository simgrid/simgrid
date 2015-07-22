/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "msg/msg_private.h"
#include "simix/smx_process_private.h"

#include "simgrid/s4u/host.hpp"

using namespace simgrid;

boost::unordered_map<std::string, simgrid::s4u::Host*> *simgrid::s4u::Host::hosts
		= new boost::unordered_map<std::string, simgrid::s4u::Host*>();

s4u::Host::Host(const char*name) {
	p_sghost = sg_host_by_name(name);
	if (p_sghost==NULL)
		xbt_die("No such host: %s",name); //FIXME: raise an exception
}

s4u::Host *s4u::Host::byName(std::string name) {
	s4u::Host * res = NULL;
	try {
		res = hosts->at(name);
	} catch (std::out_of_range& e) {}

	if (res==NULL) {
		res = new s4u::Host(name.c_str());
		hosts->insert({name,res});
	}
	return res;
}
s4u::Host *s4u::Host::current(){
	smx_process_t smx_proc = SIMIX_process_self();
	if (smx_proc == NULL)
		xbt_die("Cannot call Host::current() from the maestro context");

	return s4u::Host::byName(SIMIX_host_get_name(SIMIX_process_get_host(smx_proc)));
}

const char* s4u::Host::getName() {
	return sg_host_name(p_sghost);
}

void s4u::Host::turnOn() {
	simcall_host_on(p_sghost);
}
void s4u::Host::turnOff() {
	simcall_host_off(p_sghost);
}
bool s4u::Host::isOn() {
	return simcall_host_get_state(p_sghost);
}
