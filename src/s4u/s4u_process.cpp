/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "msg/msg_private.h"
#include "msg/msg_mailbox.h"

#include "simgrid/s4u/host.hpp"
#include "simgrid/s4u/process.hpp"
#include "simgrid/s4u/channel.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_process,"S4U processes");

/* C main function of a process, running this->main */
static int s4u_process_runner(int argc, char **argv) {

	smx_process_t smx_proc = SIMIX_process_self();
	simgrid::s4u::Process *proc = (simgrid::s4u::Process*) SIMIX_process_self_get_data(smx_proc);
	int res= proc->main(argc,argv);
	return res;
}



using namespace simgrid;

s4u::Process::Process(const char *procname, s4u::Host *host, int argc, char **argv)
    : s4u::Process::Process(procname,host, argc,argv, -1) {
}
s4u::Process::Process(const char *procname, s4u::Host *host, int argc, char **argv, double killTime) {
	p_smx_process = simcall_process_create(procname, s4u_process_runner, this, host->getName(), killTime, argc, argv, NULL/*properties*/,0);

	xbt_assert(p_smx_process,"Cannot create the process");
//	TRACE_msg_process_create(procname, simcall_process_get_PID(p_smx_process), host->getInferior());
//	simcall_process_on_exit(p_smx_process,(int_f_pvoid_pvoid_t)TRACE_msg_process_kill,p_smx_process);
}

s4u::Process *s4u::Process::current() {
	smx_process_t smx_proc = SIMIX_process_self();
	return (simgrid::s4u::Process*) SIMIX_process_self_get_data(smx_proc);
}
s4u::Process *s4u::Process::byPid(int pid) {
	return (simgrid::s4u::Process*) SIMIX_process_self_get_data(SIMIX_process_from_PID(pid));
}

void s4u::Process::setAutoRestart(bool autorestart) {
	simcall_process_auto_restart_set(p_smx_process,autorestart);
}

s4u::Host *s4u::Process::getHost() {
	return s4u::Host::byName(sg_host_name(simcall_process_get_host(p_smx_process)));
}
const char* s4u::Process::getName() {
	return simcall_process_get_name(p_smx_process);
}
int s4u::Process::getPid(){
	return simcall_process_get_PID(p_smx_process);
}

void s4u::Process::setKillTime(double time) {
	simcall_process_set_kill_time(p_smx_process,time);
}
double s4u::Process::getKillTime() {
	return simcall_process_get_kill_time(p_smx_process);
}
void s4u::Process::killAll() {
	simcall_process_killall(1);
}
void s4u::Process::kill() {
	simcall_process_kill(p_smx_process);
}

void s4u::Process::sleep(double duration) {
	simcall_process_sleep(duration);
}

void s4u::Process::execute(double flops) {
	simcall_process_execute(NULL,flops,1.0/*priority*/,0./*bound*/, 0L/*affinity*/);
}

char *s4u::Process::recvstr(Channel &chan) {
	char *res=NULL;
	size_t res_size=sizeof(res);

	simcall_comm_recv(chan.getInferior(),&res,&res_size,NULL,NULL,NULL,-1 /* timeout */,-1 /*rate*/);

    return res;
}
void s4u::Process::sendstr(Channel &chan, const char*msg) {
	char *msg_cpy=xbt_strdup(msg);
	smx_synchro_t comm = simcall_comm_isend(p_smx_process, chan.getInferior(), strlen(msg),
			-1/*rate*/, msg_cpy, sizeof(void *),
			NULL, NULL, NULL,NULL/*data*/, 0);
	simcall_comm_wait(comm, -1/*timeout*/);
}

