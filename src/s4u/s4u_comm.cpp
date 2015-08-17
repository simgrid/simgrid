/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "msg/msg_private.h"
#include "msg/msg_mailbox.h"

#include "simgrid/s4u/comm.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_comm,s4u_async,"S4U asynchronous communications");
using namespace simgrid;

s4u::Comm::~Comm() {

}

s4u::Comm &s4u::Comm::send_init(s4u::Actor *sender, s4u::Mailbox &chan) {
	s4u::Comm *res = new s4u::Comm();
	res->p_sender = sender;
	res->p_mailbox = &chan;

	return *res;
}
s4u::Comm &s4u::Comm::recv_init(s4u::Actor *receiver, s4u::Mailbox &chan) {
	s4u::Comm *res = new s4u::Comm();
	res->p_receiver = receiver;
	res->p_mailbox = &chan;

	return *res;
}

void s4u::Comm::setRate(double rate) {
	xbt_assert(p_state==inited);
	p_rate = rate;
}

void s4u::Comm::setSrcData(void * buff) {
	xbt_assert(p_state==inited);
	xbt_assert(p_dstBuff == NULL, "Cannot set the src and dst buffers at the same time");
	p_srcBuff = buff;
}
void s4u::Comm::setSrcDataSize(size_t size){
	xbt_assert(p_state==inited);
	p_srcBuffSize = size;
}
void s4u::Comm::setSrcData(void * buff, size_t size) {
	xbt_assert(p_state==inited);

	xbt_assert(p_dstBuff == NULL, "Cannot set the src and dst buffers at the same time");
	p_srcBuff = buff;
	p_srcBuffSize = size;
}
void s4u::Comm::setDstData(void ** buff) {
	xbt_assert(p_state==inited);
	xbt_assert(p_srcBuff == NULL, "Cannot set the src and dst buffers at the same time");
	p_dstBuff = buff;
}
size_t s4u::Comm::getDstDataSize(){
	xbt_assert(p_state==finished);
	return p_dstBuffSize;
}
void s4u::Comm::setDstData(void ** buff, size_t size) {
	xbt_assert(p_state==inited);

	xbt_assert(p_srcBuff == NULL, "Cannot set the src and dst buffers at the same time");
	p_dstBuff = buff;
	p_dstBuffSize = size;
}

void s4u::Comm::start() {
	xbt_assert(p_state == inited);

	if (p_srcBuff != NULL) { // Sender side
		p_inferior = simcall_comm_isend(p_sender->getInferior(), p_mailbox->getInferior(), p_remains, p_rate,
				p_srcBuff, p_srcBuffSize,
				p_matchFunction, p_cleanFunction, p_copyDataFunction,
				p_userData, p_detached);
	} else if (p_dstBuff != NULL) { // Receiver side
		p_inferior = simcall_comm_irecv(p_receiver->getInferior(), p_mailbox->getInferior(), p_dstBuff, &p_dstBuffSize,
				p_matchFunction, p_copyDataFunction,
				p_userData, p_rate);

	} else {
		xbt_die("Cannot start a communication before specifying whether we are the sender or the receiver");
	}
	p_state = started;
}
void s4u::Comm::wait() {
	xbt_assert(p_state == started || p_state == inited);

	if (p_state == started)
		simcall_comm_wait(p_inferior, -1/*timeout*/);
	else {// p_state == inited. Save a simcall and do directly a blocking send/recv
		if (p_srcBuff != NULL) {
			simcall_comm_send(p_sender->getInferior(), p_mailbox->getInferior(), p_remains, p_rate,
					p_srcBuff, p_srcBuffSize,
					p_matchFunction, p_copyDataFunction,
					p_userData, -1 /*timeout*/);
		} else {
			simcall_comm_recv(p_receiver->getInferior(), p_mailbox->getInferior(), p_dstBuff, &p_dstBuffSize,
					p_matchFunction, p_copyDataFunction,
					p_userData, -1/*timeout*/, p_rate);
		}
	}
	p_state = finished;
}
void s4u::Comm::wait(double timeout) {
	xbt_assert(p_state == started || p_state == inited);

	if (p_state == started) {
		simcall_comm_wait(p_inferior, timeout);
		p_state = finished;
		return;
	}

	// It's not started yet. Do it in one simcall
	if (p_srcBuff != NULL) {
		simcall_comm_send(p_sender->getInferior(), p_mailbox->getInferior(), p_remains, p_rate,
				p_srcBuff, p_srcBuffSize,
				p_matchFunction, p_copyDataFunction,
				p_userData, timeout);
	} else { // Receiver
		simcall_comm_recv(p_receiver->getInferior(), p_mailbox->getInferior(), p_dstBuff, &p_dstBuffSize,
				p_matchFunction, p_copyDataFunction,
				p_userData, timeout, p_rate);
	}
	p_state = finished;
}

s4u::Comm &s4u::Comm::send_async(s4u::Actor *sender, Mailbox &dest, void *data, int simulatedSize) {
	s4u::Comm &res = s4u::Comm::send_init(sender, dest);

	res.setRemains(simulatedSize);
	res.p_srcBuff = data;
	res.p_srcBuffSize = sizeof(void*);

	res.start();
	return res;
}

s4u::Comm &s4u::Comm::recv_async(s4u::Actor *receiver, Mailbox &dest, void **data) {
	s4u::Comm &res = s4u::Comm::recv_init(receiver, dest);

	res.setDstData(data);

	res.start();
	return res;
}

