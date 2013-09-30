#include "network.hpp"

#ifndef NETWORK_GTNETS_HPP_
#define NETWORK_GTNETS_HPP_

#include "simulator.h"          // Definitions for the Simulator Object
#include "node.h"               // Definitions for the Node Object
#include "linkp2p.h"            // Definitions for point-to-point link objects
#include "ratetimeparse.h"      // Definitions for Rate and Time objects
#include "application-tcpserver.h"      // Definitions for TCPServer application
#include "application-tcpsend.h"        // Definitions for TCP Sending application
#include "tcp-tahoe.h"          // Definitions for TCP Tahoe
#include "tcp-reno.h"
#include "tcp-newreno.h"
#include "event.h"
#include "routing-manual.h"
#include "red.h"

xbt_dict_t network_card_ids;

/***********
 * Classes *
 ***********/
class NetworkGTNetsModel;
typedef NetworkGTNetsModel *NetworkGTNetsModelPtr;

class NetworkGTNetsLink;
typedef NetworkGTNetsLink *NetworkGTNetsLinkPtr;

class NetworkGTNetsAction;
typedef NetworkGTNetsAction *NetworkGTNetsActionPtr;

class NetworkGTNetsActionLmm;
typedef NetworkGTNetsActionLmm *NetworkGTNetsActionLmmPtr;

/*********
 * Model *
 *********/
class NetworkGTNetsModel : public NetworkCm02Model {
public:
  NetworkGTNetsModel() : NetworkCm02Model("constant time network") {};
  int addLink(int id, double bandwidth, double latency);
  int addOnehop_route(int src, int dst, int link);
  int addRoute(int src, int dst, int *links, int nlink);
  int addRouter(int id);
  int createFlow(int src, int dst, long datasize, void *metadata);
  double getTimeToNextFlowCompletion();
  int runUntilNextFlowCompletion(void ***metadata,
                                     int *number_of_flows);
  int run(double deltat);
  // returns the total received by the TCPServer peer of the given action
  double gtNetsGetFlowRx(void *metadata);
  void createGTNetsTopology();
  void printTopology();
  void setJitter(double);
  void setJitterSeed(int);
private:
  void addNodes();
  void nodeConnect();

  bool nodeInclude(int);
  bool linkInclude(int);
  Simulator *p_sim;
  GTNETS_Topology *p_topo;
  RoutingManual *p_rm;
  REDQueue *p_redQueue;
  int m_nnode;
  int m_isTopology;
  int m_nflow;
  double m_jitter;
  int m_jitterSeed;
   map<int, Uniform*> p_uniformJitterGenerator;

   map<int, TCPServer*> p_gtnetsServers;
   map<int, TCPSend*> p_gtnetsClients;
   map<int, Linkp2p*> p_gtnetsLinks_;
   map<int, Node*> p_gtnetsNodes;
   map<void*, int> p_gtnetsActionToFlow;

   map <int, void*> p_gtnetsMetadata;

   // From Topology
   int m_nodeID;
   map<int, GTNETS_Link*> p_links;
   vector<GTNETS_Node*> p_nodes;
   map<int, int> p_hosts;      //hostid->nodeid
   set<int > p_routers;
};

/************
 * Resource *
 ************/
class NetworkGTNetsLink : public NetworkCm02Link {
public:
  NetworkGTNetsLink(NetworkGTNetsModelPtr model, const char* name, double bw, double lat, xbt_dict_t properties);
  /* Using this object with the public part of
  model does not make sense */
  double m_bwCurrent;
  double m_latCurrent;
  int m_id;
};

/**********
 * Action *
 **********/
class NetworkGTNetsAction : public NetworkCm02Action {
public:
  NetworkGTNetsAction(NetworkGTNetsModelPtr model, double latency){};

  double m_latency;
  double m_latCurrent;
#ifdef HAVE_TRACING
  int m_lastRemains;
#endif
  lmm_variable_t p_variable;
  double m_rate;
  int m_suspended;
#ifdef HAVE_TRACING
  RoutingEdgePtr src;
  RoutingEdgePtr dst;
#endif //HAVE_TRACING
};

#endif /* NETWORK_GTNETS_HPP_ */
