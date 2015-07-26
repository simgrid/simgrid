/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "msg/msg_private.h"
#include "msg/msg_mailbox.h"

#include "simgrid/s4u/channel.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_channel,"S4U Communication Channels");


using namespace simgrid;

boost::unordered_map <std::string, s4u::Channel *> *s4u::Channel::channels = new boost::unordered_map<std::string, s4u::Channel*> ();


s4u::Channel::Channel(const char*name, smx_rdv_t inferior) {
	p_inferior = inferior;
	channels->insert({name, this});
}
s4u::Channel *s4u::Channel::byName(const char*name) {
	s4u::Channel * res;
	try {
		res = channels->at(name);
	} catch (std::out_of_range& e) {
		res = new Channel(name,simcall_rdv_create(name));
	}
	return res;
}
