//Kayo Fujiwara 1/8/2007

#ifndef _GTNETS_SIM_H
#define _GTNETS_SIM_H

#ifdef __cplusplus
#include "gtnets_topology.h"

#include <iostream>
#include <sys/wait.h>
#include <map>

//GTNetS include files
#include "simulator.h"      // Definitions for the Simulator Object
#include "node.h"           // Definitions for the Node Object
#include "linkp2p.h"        // Definitions for point-to-point link objects
#include "ratetimeparse.h"  // Definitions for Rate and Time objects
#include "application-tcpserver.h" // Definitions for TCPServer application
#include "application-tcpsend.h"   // Definitions for TCP Sending application
#include "tcp-tahoe.h"      // Definitions for TCP Tahoe
#include "tcp-reno.h"
#include "tcp-newreno.h"
#include "validation.h"
#include "event.h"

using namespace std;

//Simulator s;
class GTSim {

public:
  GTSim();
  ~GTSim();
public:   
  int add_link(int id, double bandwidth, double latency);
  int add_route(int src, int dst, int* links, int nlink);
  int create_flow(int src, int dst, long datasize, void* metadata);
  double get_time_to_next_flow_completion();
  int run_until_next_flow_completion(void*** metadata, int* number_of_flows);
  int run(double deltat);
  int finalize();

  void create_gtnets_topology();
private:
  Simulator* sim_;
  SGTopology* topo_;
  int nnode_;
  int is_topology_;
  int nflow_;

  map<int, Linkp2p*>   gtnets_links_;
  map<int, Node*>      gtnets_nodes_;
  map<int, TCPServer*> gtnets_servers_;
  map<int, TCPSend*>   gtnets_clients_;
  map<int, SGLink*>    tmp_links_;
  map<int, int>        gtnets_hosts_; //<hostid, nodeid>
  map<int, void*>      gtnets_metadata_;
};

#endif /* __cplusplus */

#endif


