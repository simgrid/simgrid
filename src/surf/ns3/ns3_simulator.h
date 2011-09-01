/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _NS3_SIM_H
#define _NS3_SIM_H

#ifdef __cplusplus

#include "ns3/core-module.h"
#include "ns3/helper-module.h"
#include "ns3/simulator-module.h"
#include "ns3/node-module.h"
#include "ns3/helper-module.h"
#include "ns3/global-routing-module.h"
#include "ns3/tcp-socket-factory.h"

using namespace ns3;
using namespace std;

struct MySocket{
	uint32_t sentBytes;
	uint32_t remaining;
	uint32_t TotalBytes;
	uint32_t last_amount_sent;
	char finished;
	void* action;
};

//Simulator s;
class NS3Sim {

private:

public:
	NS3Sim();
	~NS3Sim();
	void create_flow_NS3(Ptr<Node> src,
						Ptr<Node> dst,
						uint16_t port_number,
						double start,
						const char *addr,
						uint32_t TotalBytes,
						void * action);
	void simulator_stop(double min);
	void simulator_start(void);
	void* get_action_from_socket(void *socket);
	double get_remains_from_socket(void *socket);
	double get_last_amount_sent_from_socket(void *socket);
	void reset_last_amount_sent_from_socket(void *socket);
	char get_finished(void *socket);
};

#endif                          /* __cplusplus */

#endif
