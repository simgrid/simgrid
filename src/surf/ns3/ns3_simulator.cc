/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/ns3/ns3_simulator.h"
#include "xbt/dict.h"
#include "xbt/log.h"
#include "xbt/sysdep.h"

using namespace ns3;
using namespace std;

xbt_dict_t dict_socket = NULL;

NS3Sim SimulatorNS3;
static char socket_key[24];

static void receive_callback(Ptr<Socket> localSocket);
static void send_callback(Ptr<Socket> localSocket, uint32_t txSpace);
static void datasent_callback(Ptr<Socket> localSocket, uint32_t dataSent);
static void StartFlow(Ptr<Socket> sock,
    const char *to,
    uint16_t port_number);

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ns3);

// Constructor.
NS3Sim::NS3Sim(){
}
//Destructor.
NS3Sim::~NS3Sim(){
}

static inline void transformSocketPtr (Ptr<Socket> localSocket){
  std::stringstream sstream;
  sstream << localSocket ;
  std::string s = sstream.str();
  sprintf(socket_key,"%s",s.c_str());
}

static void delete_mysocket(void *p)
{
  MySocket *sock = (MySocket *)p;
  delete(sock);
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
 * 		totalBytes: number of bytes to transmit
 */
void NS3Sim::create_flow_NS3(
		Ptr<Node> src,
		Ptr<Node> dst,
		uint16_t port_number,
		double start,
		const char *addr,
		uint32_t totalBytes,
		void * action)
{
	if(!dict_socket) dict_socket = xbt_dict_new_homogeneous(delete_mysocket);

	PacketSinkHelper sink ("ns3::TcpSocketFactory",
							InetSocketAddress (Ipv4Address::GetAny(),
							port_number));
	sink.Install (dst);

	Ptr<Socket> sock = Socket::CreateSocket (src,
							TcpSocketFactory::GetTypeId());

	MySocket *mysocket = new MySocket();
	mysocket->totalBytes = totalBytes;
	mysocket->remaining = totalBytes;
	mysocket->bufferedBytes = 0;
	mysocket->sentBytes = 0;
	mysocket->finished = 0;
	mysocket->action = action;

	transformSocketPtr(sock);
	xbt_dict_set(dict_socket,socket_key, mysocket,NULL);

	sock->Bind(InetSocketAddress(port_number));
	XBT_DEBUG("Create flow starting to %fs + %fs = %fs",start-ns3_time(), ns3_time(), start);

	Simulator::Schedule (Seconds(start-ns3_time()),&StartFlow, sock, addr, port_number);
//	Simulator::Schedule (Seconds(0.0),&StartFlow, sock, addr, port_number);

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

double NS3Sim::get_sent_from_socket(void *socket){
  return ((MySocket *)socket)->sentBytes;
}

void NS3Sim::simulator_start(double min){
  if(min > 0.0)
    Simulator::Stop(Seconds(min));
  XBT_DEBUG("Start simulator '%f'",min);
  Simulator::Run ();
}

static MySocket* get_my_socket(Ptr<Socket> localSocket) {
	transformSocketPtr(localSocket);
	return (MySocket*)xbt_dict_get_or_null(dict_socket,socket_key);
}

static void receive_callback(Ptr<Socket> localSocket){
  MySocket* mysocket = get_my_socket(localSocket);

  if (mysocket->finished == 0){
    mysocket->finished = 1;
    XBT_DEBUG("recv_cb of F[%p, %p, %d]", mysocket, mysocket->action, mysocket->totalBytes);
    XBT_DEBUG("Stop simulator at %f seconds", Simulator::Now().GetSeconds());
    Simulator::Stop(Seconds(0.0));
    Simulator::Run();
  }
}

static void send_callback(Ptr<Socket> localSocket, uint32_t txSpace){
	MySocket* mysocket = get_my_socket(localSocket);

	if (mysocket->remaining == 0){
		  //all data was already buffered (and socket was already closed), just return
		  return;
	}

	uint8_t *data = (uint8_t*)malloc(sizeof(uint8_t)*txSpace);

	while (mysocket->bufferedBytes < mysocket->totalBytes
			&& localSocket->GetTxAvailable () > 0)
	{
      uint32_t toWrite = min ((mysocket->remaining), txSpace);
      toWrite = min (toWrite, localSocket->GetTxAvailable ());
      int amountSent = localSocket->Send (data, toWrite, 0);

      if(amountSent < 0)
    	  return;
      (mysocket->bufferedBytes) += amountSent;
      (mysocket->remaining) -= amountSent;
      XBT_DEBUG("send_cb of F[%p, %p, %d] (%d/%d) %d buffered", mysocket, mysocket->action, mysocket->totalBytes, mysocket->remaining, mysocket->totalBytes, amountSent);

    }

	free(data);

	if ((mysocket->bufferedBytes) >= mysocket->totalBytes){
		localSocket->Close();
	}
}

static void datasent_callback(Ptr<Socket> localSocket, uint32_t dataSent){
  MySocket* mysocket = get_my_socket(localSocket);
  mysocket->sentBytes += dataSent;
  XBT_DEBUG("datasent_cb of F[%p, %p, %d] %d sent", mysocket, mysocket->action, mysocket->totalBytes, dataSent);
}

static void normalClose_callback(Ptr<Socket> localSocket){
  MySocket* mysocket = get_my_socket(localSocket);
  XBT_DEBUG("normalClose_cb of F[%p, %p, %d]", mysocket, mysocket->action, mysocket->totalBytes);
  receive_callback (localSocket);
}

static void errorClose_callback(Ptr<Socket> localSocket){
  MySocket* mysocket = get_my_socket(localSocket);
  XBT_DEBUG("errorClose_cb of F[%p, %p, %d]", mysocket, mysocket->action, mysocket->totalBytes);
  xbt_die("NS3: a socket was closed anormally");
}

static void succeededConnect_callback(Ptr<Socket> localSocket){
  MySocket* mysocket = get_my_socket(localSocket);
  XBT_DEBUG("succeededConnect_cb of F[%p, %p, %d]", mysocket, mysocket->action, mysocket->totalBytes);
}

static void failedConnect_callback(Ptr<Socket> localSocket){
  MySocket* mysocket = get_my_socket(localSocket);
  XBT_DEBUG("failedConnect_cb of F[%p, %p, %d]", mysocket, mysocket->action, mysocket->totalBytes);
  xbt_die("NS3: a socket failed to connect");
}

static void StartFlow(Ptr<Socket> sock,
    const char *to,
    uint16_t port_number)
{
  InetSocketAddress serverAddr (to, port_number);

  sock->Connect(serverAddr);
  sock->SetSendCallback (MakeCallback (&send_callback));
  sock->SetRecvCallback (MakeCallback (&receive_callback));
  sock->SetDataSentCallback (MakeCallback (&datasent_callback));
  sock->SetConnectCallback (MakeCallback (&succeededConnect_callback), MakeCallback (&failedConnect_callback));
  sock->SetCloseCallbacks (MakeCallback (&normalClose_callback), MakeCallback (&errorClose_callback));

  MySocket* mysocket = get_my_socket(sock);
  XBT_DEBUG("startFlow_cb of F[%p, %p, %d] dest=%s port=%d", mysocket, mysocket->action, mysocket->totalBytes, to, port_number);
}
