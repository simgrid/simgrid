/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "msg/msg_private.h"
#include "msg/msg_mailbox.h"

#include "simgrid/s4u/actor.hpp"
#include "simgrid/s4u/comm.hpp"
#include "simgrid/s4u/host.hpp"
#include "simgrid/s4u/mailbox.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_actor,"S4U actors");

/* C main function of a actor, running this->main */
static int s4u_actor_runner(int argc, char **argv) {

	smx_process_t smx_proc = SIMIX_process_self();
	simgrid::s4u::Actor *actor = (simgrid::s4u::Actor*) SIMIX_process_self_get_data(smx_proc);
	int res = actor->main(argc,argv);
	return res;
}



using namespace simgrid;

s4u::Actor::Actor(const char *name, s4u::Host *host, int argc, char **argv)
    : s4u::Actor::Actor(name,host, argc,argv, -1) {
}
s4u::Actor::Actor(const char *name, s4u::Host *host, int argc, char **argv, double killTime) {
	p_smx_process = simcall_process_create(name, s4u_actor_runner, this, host->getName(), killTime, argc, argv, NULL/*properties*/,0);

	xbt_assert(p_smx_process,"Cannot create the actor");
//	TRACE_msg_process_create(procname, simcall_process_get_PID(p_smx_process), host->getInferior());
//	simcall_process_on_exit(p_smx_process,(int_f_pvoid_pvoid_t)TRACE_msg_process_kill,p_smx_process);
}

s4u::Actor *s4u::Actor::current() {
	smx_process_t smx_proc = SIMIX_process_self();
	return (simgrid::s4u::Actor*) SIMIX_process_self_get_data(smx_proc);
}
s4u::Actor *s4u::Actor::byPid(int pid) {
	return (simgrid::s4u::Actor*) SIMIX_process_self_get_data(SIMIX_process_from_PID(pid));
}

void s4u::Actor::setAutoRestart(bool autorestart) {
	simcall_process_auto_restart_set(p_smx_process,autorestart);
}

s4u::Host *s4u::Actor::getHost() {
	return s4u::Host::byName(sg_host_name(simcall_process_get_host(p_smx_process)));
}
const char* s4u::Actor::getName() {
	return simcall_process_get_name(p_smx_process);
}
int s4u::Actor::getPid(){
	return simcall_process_get_PID(p_smx_process);
}

void s4u::Actor::setKillTime(double time) {
	simcall_process_set_kill_time(p_smx_process,time);
}
double s4u::Actor::getKillTime() {
	return simcall_process_get_kill_time(p_smx_process);
}
void s4u::Actor::killAll() {
	simcall_process_killall(1);
}
void s4u::Actor::kill() {
	simcall_process_kill(p_smx_process);
}

void s4u::Actor::sleep(double duration) {
	simcall_process_sleep(duration);
}

void s4u::Actor::execute(double flops) {
	simcall_process_execute(NULL,flops,1.0/*priority*/,0./*bound*/, 0L/*affinity*/);
}

void *s4u::Actor::recv(Mailbox &chan) {
	void *res=NULL;

	Comm c = Comm::recv_init(this, chan);
	c.setDstData(&res,sizeof(res));
	c.wait();

    return res;
}
void s4u::Actor::send(Mailbox &chan, void *payload, size_t simulatedSize) {
	Comm c = Comm::send_init(this,chan);
	c.setRemains(simulatedSize);
	c.setSrcData(payload);
	c.wait();
}

s4u::Comm &s4u::Actor::send_init(Mailbox &chan) {
	return s4u::Comm::send_init(this, chan);
}
