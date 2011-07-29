/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf/ns3/ns3_simulator.h"
#include "xbt/dict.h"
#include "xbt/log.h"

using namespace ns3;
using namespace std;

static const uint32_t writeSize  = 1024; // limit the amout of data to write
uint8_t data[writeSize];
xbt_dict_t dict_socket = NULL;

NS3Sim SimulatorNS3;

static void receive_callback(Ptr<Socket> localSocket);
static void send_callback(Ptr<Socket> localSocket, uint32_t txSpace);
static void StartFlow(Ptr<Socket> sock,
    const char *to,
    uint16_t port_number);

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simulator_ns3, surf,
                                "Logging specific to the SURF network NS3 module");

// Constructor.
NS3Sim::NS3Sim(){
}
//Destructor.
NS3Sim::~NS3Sim(){
}

/*
 * This function create a flow from src to dst
 *
 * Parameters
 * 		src: node source
 * 		dst: node destination
 * 		port_number: The port number to use
 * 		start: the time the communication start
 * 		addr:  ip address
 * 		TotalBytes: number of bytes to transmit
 */
void NS3Sim::create_flow_NS3(
		Ptr<Node> src,
		Ptr<Node> dst,
		uint16_t port_number,
		double start,
		const char *addr,
		uint32_t TotalBytes,
		void * action)
{
	if(!dict_socket) dict_socket = xbt_dict_new();
	PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny(), port_number));
	sink.Install (dst);
	Ptr<Socket> sock = Socket::CreateSocket (src, TypeId::LookupByName ("ns3::TcpSocketFactory"));
	MySocket *mysocket = new MySocket();
	mysocket->TotalBytes = TotalBytes;
	mysocket->remaining = TotalBytes;
	mysocket->sentBytes = 0;
	mysocket->finished = 0;
	mysocket->action = action;
	xbt_dict_set(dict_socket,(const char*)&sock, mysocket,NULL);
	sock->Bind(InetSocketAddress(port_number));
	Simulator::Schedule (Seconds(start),&StartFlow, sock, addr, port_number);
}

void* NS3Sim::get_action_from_socket(void *socket){
	return ((MySocket *)socket)->action;
}

char NS3Sim::get_finished(void *socket){
	return ((MySocket *)socket)->finished;
}

double NS3Sim::get_remains_from_socket(void *socket){
	return ((MySocket *)socket)->remaining;
}

void NS3Sim::simulator_stop(double min){
	if(min > 0.0)
		Simulator::Stop(Seconds(min));
	else
		Simulator::Stop();
}

void NS3Sim::simulator_start(void){
	XBT_DEBUG("Start simulator");
	Simulator::Run ();
}

static void receive_callback(Ptr<Socket> localSocket){
  Address addr;
  localSocket->GetSockName (addr);
  InetSocketAddress iaddr = InetSocketAddress::ConvertFrom (addr);
  MySocket* mysocket = (MySocket*)xbt_dict_get_or_null(dict_socket,(char*)&localSocket);
  mysocket->finished = 1;

  //cout << "[" << Simulator::Now ().GetSeconds() << "] " << "Received [" << mysocket->TotalBytes << "bytes],  from: " << iaddr.GetIpv4 () << " port: " << iaddr.GetPort () << endl;
	std::stringstream sstream;
		sstream << Simulator::Now ().GetSeconds();
		std::string s = sstream.str();
		size_t size = s.size() + 1;
		char * time_sec = new char[ size ];
		strncpy( time_sec, s.c_str(), size );
  XBT_DEBUG("Stop simulator at %s seconds",time_sec);
  Simulator::Stop();
}

static void send_callback(Ptr<Socket> localSocket, uint32_t txSpace){

	Address addr;
	localSocket->GetSockName (addr);
	InetSocketAddress iaddr = InetSocketAddress::ConvertFrom (addr);
	MySocket* mysocket = (MySocket*)xbt_dict_get_or_null(dict_socket,(char*)&localSocket);
	uint32_t totalBytes = mysocket->TotalBytes;
	while ((mysocket->sentBytes) < totalBytes && localSocket->GetTxAvailable () > 0){
      uint32_t toWrite = min ((mysocket->remaining), writeSize);
      toWrite = min (toWrite, localSocket->GetTxAvailable ());
      int amountSent = localSocket->Send (&data[0], toWrite, 0);

      if(amountSent < 0)
    	  return;
	  (mysocket->sentBytes) += amountSent;
	  (mysocket->remaining) -= amountSent;
	  //cout << "[" << Simulator::Now ().GetSeconds() << "] " << "Send one packet, remaining "<<  mysocket->remaining << " bytes!" << endl;
    }
	if ((mysocket->sentBytes) >= totalBytes){
		localSocket->Close();
	}

}

static void StartFlow(Ptr<Socket> sock,
    const char *to,
    uint16_t port_number)
{
  InetSocketAddress serverAddr (to, port_number);

  //cout << "[" <<  Simulator::Now().GetSeconds() << "] Starting flow to " << to << " using port " << port_number << endl;

  sock->Connect(serverAddr);
  sock->SetSendCallback (MakeCallback (&send_callback));
  sock->SetRecvCallback (MakeCallback (&receive_callback));
}
