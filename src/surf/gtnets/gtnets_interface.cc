/* 	$Id$	 */
/* Copyright (c) 2007 Kayo Fujiwara. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "gtnets_simulator.h"
#include "gtnets_interface.h"
#ifdef DEBUG0
	#undef DEBUG0
#endif
#include "xbt/log.h"
#include "xbt/asserts.h"

static GTSim* gtnets_sim = 0;


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_network_gtnets_interface, surf_network_gtnets,
                                "Logging specific to the SURF network GTNetS interface");



// initialize the GTNetS interface and environment
int gtnets_initialize(){

  DEBUG0("Using logging.");

  xbt_assert0(!gtnets_sim, "gtnets already initialized");

  gtnets_sim = new GTSim();
  return 0;
}

// add a link (argument link is just an index starting at 0... 
// add link 0, add link 1, etc.)
int gtnets_add_link(int id, double bandwidth, double latency){
  return gtnets_sim->add_link(id, bandwidth, latency);
}

// add a route between a source and a destination as an array of link indices
// (note that there is no gtnets_add_network_card(), as we discover them
// on the fly via calls to gtnets_add_route()
int gtnets_add_route(int src, int dst, int* links, int nlink){
  return gtnets_sim->add_route(src, dst, links, nlink);
}

// add a router
int gtnets_add_router(int id){
  return gtnets_sim->add_router(id);
}

// add a route between a source and a destination as an array of link indices
// (note that there is no gtnets_add_network_card(), as we discover them
// on the fly via calls to gtnets_add_route()
int gtnets_add_onehop_route(int src, int dst, int link){
  return gtnets_sim->add_onehop_route(src, dst, link);
}

// create a new flow on a route
// one can attach arbitrary metadata to a flow
int gtnets_create_flow(int src, int dst, long datasize, void* metadata){
  return gtnets_sim->create_flow(src, dst, datasize, metadata);
}

// get the time (double) until a flow completes (the first such flow)
// if no flows exist, return -1.0
double gtnets_get_time_to_next_flow_completion(){
  ofstream file;
  streambuf* sbuf;
  double value;

  if (!XBT_LOG_ISENABLED(surf_network_gtnets_interface, xbt_log_priority_debug)) {
	  file.open ("/dev/null");
	  sbuf = cout.rdbuf();
	  cout.rdbuf(file.rdbuf());
	  DEBUG0("Enable GTNetS library quite mode");
  }else {
	  DEBUG0("Disable GTNetS library quite mode");
  }

  value = gtnets_sim->get_time_to_next_flow_completion();

  if (!XBT_LOG_ISENABLED(surf_network_gtnets_interface, xbt_log_priority_debug)) {
	  cout.rdbuf(sbuf);
	  file.close();
  }
  return value;
}

// run until a flow completes (returns that flow's metadata)
int gtnets_run_until_next_flow_completion(void ***metadata, int *number_of_flows){
  ofstream file;
  streambuf* sbuf;
  double value;

  if (!XBT_LOG_ISENABLED(surf_network_gtnets_interface, xbt_log_priority_debug)) {
	  file.open ("/dev/null");
	  sbuf = cout.rdbuf();
	  cout.rdbuf(file.rdbuf());
	  DEBUG0("Enable GTNetS library quite mode");
  }else {
	  DEBUG0("Disable GTNetS library quite mode");
  }

  value = gtnets_sim->run_until_next_flow_completion(metadata, number_of_flows);

  if (!XBT_LOG_ISENABLED(surf_network_gtnets_interface, xbt_log_priority_debug)) {
	  cout.rdbuf(sbuf);
	  file.close();
  }
  return value;
}

// get the total received in bytes using the TCPServer object totRx field
double gtnets_get_flow_rx(void *metadata){
  return gtnets_sim->gtnets_get_flow_rx(metadata);
}


// run for a given time (double)
int gtnets_run(Time_t deltat){
  gtnets_sim->run(deltat);
  return 0;
}

// clean up
int gtnets_finalize(){
  if (!gtnets_sim) return -1;
  delete gtnets_sim;
  gtnets_sim = 0;
  return 0;
}

// print topology
void gtnets_print_topology(void){
  gtnets_sim->print_topology();
}

