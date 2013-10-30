/* Copyright (c) 2007-2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "ns3_interface.h"
#include "ns3_simulator.h"
#include "xbt/lib.h"
#include "xbt/log.h"
#include "xbt/dynar.h"


using namespace ns3;

extern xbt_lib_t host_lib;
extern int NS3_HOST_LEVEL;		//host node for ns3
extern xbt_dynar_t IPV4addr;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(interface_ns3, surf,
                                "Logging specific to the SURF network NS3 module");

InternetStackHelper stack;
NodeContainer nodes;
NodeContainer Cluster_nodes;
Ipv4InterfaceContainer interfaces;

int number_of_nodes = 0;
int number_of_clusters_nodes = 0;
int number_of_links = 1;
int number_of_networks = 1;
int port_number = 1025; //Port number is limited from 1025 to 65 000

static NS3Sim* ns3_sim = 0;

void ns3_simulator(double min){
			ns3_sim->simulator_start(min);
}

void* ns3_get_socket_action(void *socket){
		return ns3_sim->get_action_from_socket(socket);
}

double ns3_get_socket_remains(void *socket){
		return ns3_sim->get_remains_from_socket(socket);
}

double ns3_get_socket_sent(void *socket){
  return ns3_sim->get_sent_from_socket(socket);
}

char ns3_get_socket_is_finished(void *socket){
		return ns3_sim->get_finished(socket);
}

double ns3_time(){
	return Simulator::Now().GetSeconds();
}

int ns3_create_flow(const char* a,const char *b,double start,u_int32_t TotalBytes,void * action)
{
	ns3_nodes_t node1 = (ns3_nodes_t) xbt_lib_get_or_null(host_lib,a,NS3_HOST_LEVEL);
	ns3_nodes_t node2 = (ns3_nodes_t) xbt_lib_get_or_null(host_lib,b,NS3_HOST_LEVEL);

	Ptr<Node> src_node = nodes.Get(node1->node_num);
	Ptr<Node> dst_node = nodes.Get(node2->node_num);

	char* addr = (char*)xbt_dynar_get_as(IPV4addr,node2->node_num,char*);

	XBT_DEBUG("ns3_create_flow %d Bytes from %d to %d with Interface %s",TotalBytes, node1->node_num, node2->node_num,addr);
	ns3_sim->create_flow_NS3(src_node,
			dst_node,
			port_number,
			start,
			addr,
			TotalBytes,
			action);

	port_number++;
	if(port_number >= 65001 ) xbt_die("Too many connections! Port number is saturated.");
	return 0;
}

// clean up
int ns3_finalize(void){
	if (!ns3_sim) return -1;
	delete ns3_sim;
	ns3_sim = 0;
	return 0;
}

// initialize the NS3 interface and environment
int ns3_initialize(const char* TcpProtocol){
  xbt_assert(!ns3_sim, "ns3 already initialized");
  ns3_sim = new NS3Sim();

//  tcpModel are:
//  "ns3::TcpNewReno"
//  "ns3::TcpReno"
//  "ns3::TcpTahoe"

  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1024)); // 1024-byte packet for easier reading
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));

  if(!strcmp(TcpProtocol,"default")){
	  return 0;
  }
  if(!strcmp(TcpProtocol,"Reno")){
	  XBT_INFO("Switching Tcp protocol to '%s'",TcpProtocol);
	  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpReno"));
	  return 0;
  }
  if(!strcmp(TcpProtocol,"NewReno")){
	  XBT_INFO("Switching Tcp protocol to '%s'",TcpProtocol);
	  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));
	  return 0;
  }
  if(!strcmp(TcpProtocol,"Tahoe")){
	  XBT_INFO("Switching Tcp protocol to '%s'",TcpProtocol);
	  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpTahoe"));
	  return 0;
  }

  XBT_ERROR("The ns3/TcpModel must be : NewReno or Reno or Tahoe");
}

void * ns3_add_host(const char * id)
{
	ns3_nodes_t host  = xbt_new0(s_ns3_nodes_t,1);
	XBT_DEBUG("Interface ns3 add host[%d] '%s'",number_of_nodes,id);
	Ptr<Node> node =  CreateObject<Node> (0);
	stack.Install(node);
	nodes.Add(node);
	host->node_num = number_of_nodes;
	host->type = NS3_NETWORK_ELEMENT_HOST;
	host->data = GetPointer(node);
	number_of_nodes++;
	return host;
}

void * ns3_add_host_cluster(const char * id)
{
	ns3_nodes_t host  = xbt_new0(s_ns3_nodes_t,1);
	XBT_DEBUG("Interface ns3 add host[%d] '%s'",number_of_nodes,id);
	Ptr<Node> node =  CreateObject<Node> (0);
	stack.Install(node);
	Cluster_nodes.Add(node);
	nodes.Add(node);
	host->node_num = number_of_nodes;
	host->type = NS3_NETWORK_ELEMENT_HOST;
	host->data = node;
	number_of_nodes++;
	return host;
}

void * ns3_add_router(const char * id)
{
	ns3_nodes_t router  = xbt_new0(s_ns3_nodes_t,1);
	XBT_DEBUG("Interface ns3 add router[%d] '%s'",number_of_nodes,id);
	Ptr<Node> node =  CreateObject<Node> (0);
	stack.Install(node);
	nodes.Add(node);
	router->node_num = number_of_nodes;
	router->type = NS3_NETWORK_ELEMENT_ROUTER;
	router->data = node;
	number_of_nodes++;
	return router;
}

void * ns3_add_cluster(char * bw,char * lat,const char *id)
{

	XBT_DEBUG("cluster_id: %s",id);
	XBT_DEBUG("bw: %s lat: %s",bw,lat);
	XBT_DEBUG("Number of %s nodes: %d",id,Cluster_nodes.GetN() - number_of_clusters_nodes);

	NodeContainer Nodes;
	int i;

	for(i = number_of_clusters_nodes; i < Cluster_nodes.GetN() ; i++){
		Nodes.Add(Cluster_nodes.Get(i));
		XBT_DEBUG("Add node %d to cluster",i);
	}
	number_of_clusters_nodes = Cluster_nodes.GetN();

	XBT_DEBUG("Add router %d to cluster",nodes.GetN()-Nodes.GetN()-1);
	Nodes.Add(nodes.Get(nodes.GetN()-Nodes.GetN()-1));

	if(Nodes.GetN() > 65000)
		xbt_die("Cluster with NS3 is limited to 65000 nodes");
	CsmaHelper csma;
	csma.SetChannelAttribute ("DataRate", StringValue (bw));
	csma.SetChannelAttribute ("Delay", StringValue (lat));
	NetDeviceContainer devices = csma.Install (Nodes);
	XBT_DEBUG("Create CSMA");

	char * adr = bprintf("%d.%d.0.0",number_of_networks,number_of_links);
	XBT_DEBUG("Assign IP Addresses %s to CSMA.",adr);
	Ipv4AddressHelper ipv4;
	ipv4.SetBase (adr, "255.255.0.0");
	free(adr);
	interfaces.Add(ipv4.Assign (devices));

	if(number_of_links == 255){
		if(number_of_networks == 255)
			xbt_die("Number of links and networks exceed 255*255");
		number_of_links = 1;
		number_of_networks++;
	}else{
		number_of_links++;
	}
	XBT_DEBUG("Number of nodes in Cluster_nodes: %d",Cluster_nodes.GetN());
}

void * ns3_add_AS(const char * id)
{
	XBT_DEBUG("Interface ns3 add AS '%s'",id);
	return NULL;
}

static char* transformIpv4Address (Ipv4Address from){
	std::stringstream sstream;
		sstream << from ;
		std::string s = sstream.str();
		return bprintf("%s",s.c_str());
}

void * ns3_add_link(int src, e_ns3_network_element_type_t type_src,
					int dst, e_ns3_network_element_type_t type_dst,
					char * bw,char * lat)
{
	if(number_of_links == 1 ) {
		LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
		LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
	}


	MyPointToPointHelper pointToPoint;

	NetDeviceContainer netA;
	Ipv4AddressHelper address;

	Ptr<Node> a = nodes.Get(src);
	Ptr<Node> b = nodes.Get(dst);

	XBT_DEBUG("\tAdd PTP from %d to %d bw:'%s' lat:'%s'",src,dst,bw,lat);
	pointToPoint.SetDeviceAttribute ("DataRate", StringValue (bw));
	pointToPoint.SetChannelAttribute ("Delay", StringValue (lat));
	//pointToPoint.EnablePcapAll("test_ns3_trace"); //DEBUG

	netA.Add(pointToPoint.Install (a, type_src, b, type_dst));

	char * adr = bprintf("%d.%d.0.0",number_of_networks,number_of_links);
	address.SetBase (adr, "255.255.0.0");
	XBT_DEBUG("\tInterface stack '%s'",adr);
	free(adr);
	interfaces.Add(address.Assign (netA));

	char *tmp = transformIpv4Address(interfaces.GetAddress(interfaces.GetN()-2));
	xbt_dynar_set_as(IPV4addr,src,char*,tmp);
	XBT_DEBUG("Have write '%s' for Node '%d'",(char*)xbt_dynar_get_as(IPV4addr,src,char*),src);

	tmp = transformIpv4Address(interfaces.GetAddress(interfaces.GetN()-1));
	xbt_dynar_set_as(IPV4addr,dst,char*,tmp);
	XBT_DEBUG("Have write '%s' for Node '%d'",(char*)xbt_dynar_get_as(IPV4addr,dst,char*),dst);

	if(number_of_links == 255){
		if(number_of_networks == 255)
			xbt_die("Number of links and networks exceed 255*255");
		number_of_links = 1;
		number_of_networks++;
	}else{
		number_of_links++;
	}
}

void * ns3_end_platform(void)
{
	XBT_DEBUG("InitializeRoutes");
	GlobalRouteManager::BuildGlobalRoutingDatabase();
	GlobalRouteManager::InitializeRoutes();
}
