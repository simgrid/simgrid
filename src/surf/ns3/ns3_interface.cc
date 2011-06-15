/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "ns3_interface.h"
#include "ns3/core-module.h"
#include "ns3/simulator-module.h"
#include "ns3/node-module.h"
#include "ns3/helper-module.h"
#include "ns3/global-route-manager.h"
#include "ns3/global-route-manager.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

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

void * ns3_add_host(char * id)
{
	ns3_nodes_t host  = xbt_new0(s_ns3_nodes_t,1);
	XBT_INFO("Interface ns3 add host[%d] '%s'",number_of_nodes,id);
	Ptr<Node> node =  CreateObject<Node> (0);
	stack.Install(node);
	nodes.Add(node);
	host->node_num = number_of_nodes;
	host->type = NS3_NETWORK_ELEMENT_HOST;
	host->data = node;
	number_of_nodes++;
	return host;
}

void * ns3_add_host_cluster(char * id)
{
	ns3_nodes_t host  = xbt_new0(s_ns3_nodes_t,1);
	XBT_INFO("Interface ns3 add host[%d] '%s'",number_of_nodes,id);
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

void * ns3_add_router(char * id)
{
	ns3_nodes_t router  = xbt_new0(s_ns3_nodes_t,1);
	XBT_INFO("Interface ns3 add router[%d] '%s'",number_of_nodes,id);
	Ptr<Node> node =  CreateObject<Node> (0);
	stack.Install(node);
	nodes.Add(node);
	router->node_num = number_of_nodes;
	router->type = NS3_NETWORK_ELEMENT_ROUTER;
	router->data = node;
	number_of_nodes++;
	return router;
}

void * ns3_add_cluster(char * bw,char * lat,char *id)
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

	if(Nodes.GetN() > 254)
		xbt_die("Cluster with NS3 is limited to 254 nodes");
	CsmaHelper csma;
	csma.SetChannelAttribute ("DataRate", StringValue (bw));
	csma.SetChannelAttribute ("Delay", StringValue (lat));
	NetDeviceContainer devices = csma.Install (Nodes);
	XBT_DEBUG("Create CSMA");


	char * adr = bprintf("10.%d.%d.0",number_of_networks,number_of_links);
	XBT_DEBUG("Assign IP Addresses %s to CSMA.",adr);
	Ipv4AddressHelper ipv4;
	ipv4.SetBase (adr, "255.255.255.0");
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

void * ns3_add_AS(char * id)
{
	XBT_INFO("Interface ns3 add AS '%s'",id);
	return NULL;
}

void * ns3_add_link(int src,int dst,char * bw,char * lat)
{
	if(number_of_links == 1 ) {
		LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
		LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
	}

	PointToPointHelper pointToPoint;
	NetDeviceContainer netA;
	Ipv4AddressHelper address;

	Ptr<Node> a = nodes.Get(src);
	Ptr<Node> b = nodes.Get(dst);

	XBT_DEBUG("\tAdd PTP from %d to %d bw:'%s' lat:'%s'",src,dst,bw,lat);
	pointToPoint.SetDeviceAttribute ("DataRate", StringValue (bw));
	pointToPoint.SetChannelAttribute ("Delay", StringValue (lat));
	//pointToPoint.EnablePcapAll("test_ns3_trace"); //DEBUG

	netA.Add(pointToPoint.Install (a, b));

	char * adr = bprintf("10.%d.%d.0",number_of_networks,number_of_links);
	address.SetBase (adr, "255.255.255.0");
	XBT_DEBUG("\tInterface stack '%s'",adr);
	interfaces.Add(address.Assign (netA));

	XBT_DEBUG(" ");
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
	XBT_INFO("InitializeRoutes");
	GlobalRouteManager::BuildGlobalRoutingDatabase();
	GlobalRouteManager::InitializeRoutes();
	//TODO REMOVE ;)
	Ptr<Node> a = nodes.Get(0);
	Ptr<Node> b = nodes.Get(11);
	Ptr<Node> c = nodes.Get(12);
	Ptr<Node> d = nodes.Get(13);

	UdpEchoServerHelper echoServer (9);

	ApplicationContainer serverApps = echoServer.Install (a);
	serverApps.Start (Seconds (1.0));
	serverApps.Stop (Seconds (20.0));

	UdpEchoClientHelper echoClient (interfaces.GetAddress (0), 9);
	echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
	echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.)));
	echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
	ApplicationContainer clientApps_b = echoClient.Install (b);
	clientApps_b.Start (Seconds (2.0));
	clientApps_b.Stop (Seconds (10.0));

	UdpEchoClientHelper echoClient2 (interfaces.GetAddress (0), 9);
	echoClient2.SetAttribute ("MaxPackets", UintegerValue (1));
	echoClient2.SetAttribute ("Interval", TimeValue (Seconds (1.)));
	echoClient2.SetAttribute ("PacketSize", UintegerValue (512));
	ApplicationContainer clientApps_c = echoClient2.Install (c);
	clientApps_c.Start (Seconds (3.0));
	clientApps_c.Stop (Seconds (10.0));

	UdpEchoClientHelper echoClient3 (interfaces.GetAddress (0), 9);
	echoClient3.SetAttribute ("MaxPackets", UintegerValue (1));
	echoClient3.SetAttribute ("Interval", TimeValue (Seconds (1.)));
	echoClient3.SetAttribute ("PacketSize", UintegerValue (256));
	ApplicationContainer clientApps_d = echoClient3.Install (d);
	clientApps_d.Start (Seconds (4.0));
	clientApps_d.Stop (Seconds (10.0));

	Simulator::Run ();
	Simulator::Destroy ();

	//HEEEEEEE
}
