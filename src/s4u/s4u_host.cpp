/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "src/msg/msg_private.h"
#include "src/simix/smx_process_private.h"

#include "simgrid/s4u/host.hpp"
#include "simgrid/s4u/storage.hpp"

namespace simgrid {
namespace s4u {

boost::unordered_map<std::string, Host*> *Host::hosts
		= new boost::unordered_map<std::string, Host*>();

Host::Host(const char*name) {
	p_inferior = sg_host_by_name(name);
	if (p_inferior==NULL)
		xbt_die("No such host: %s",name); //FIXME: raise an exception
}
Host::~Host() {
	if (mounts != NULL)
		delete mounts;
}

Host *Host::byName(std::string name) {
	Host * res = NULL;
	try {
		res = hosts->at(name);
	} catch (std::out_of_range& e) {}

	if (res==NULL) {
		res = new Host(name.c_str());
		hosts->insert({name,res});
	}
	return res;
}
Host *Host::current(){
	smx_process_t smx_proc = SIMIX_process_self();
	if (smx_proc == NULL)
		xbt_die("Cannot call Host::current() from the maestro context");

	return Host::byName(sg_host_get_name(SIMIX_process_get_host(smx_proc)));
}

const char* Host::name() {
	return sg_host_get_name(p_inferior);
}

void Host::turnOn() {
	simcall_host_on(p_inferior);
}
void Host::turnOff() {
	simcall_host_off(p_inferior);
}
bool Host::isOn() {
	return sg_host_get_state(p_inferior);
}

boost::unordered_map<std::string, Storage&> &Host::mountedStorages() {
	if (mounts == NULL) {
		mounts = new boost::unordered_map<std::string, Storage&> ();

		xbt_dict_t dict = simcall_host_get_mounted_storage_list(p_inferior);

		xbt_dict_cursor_t cursor;
		char *mountname;
		char *storagename;
		xbt_dict_foreach(dict, cursor, mountname, storagename) {
			mounts->insert({mountname, Storage::byName(storagename)});
		}
		xbt_dict_free(&dict);
	}

	return *mounts;
}


} // namespace simgrid
} // namespace s4u
