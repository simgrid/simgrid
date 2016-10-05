/* Copyright (c) 2007-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/ns3/ns3_simulator.h"
#include "xbt/dict.h"
#include "xbt/log.h"
#include "xbt/sysdep.h"

static void delete_mysocket(void *p)
{
  delete (SgFlow *)p;
}
xbt_dict_t flowFromSock = xbt_dict_new_homogeneous(delete_mysocket);; // ns3::sock -> SgFlow

static void receive_callback(ns3::Ptr<ns3::Socket> socket);
static void send_callback(ns3::Ptr<ns3::Socket> sock, uint32_t txSpace);
static void datasent_callback(ns3::Ptr<ns3::Socket> socket, uint32_t dataSent);

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ns3);

SgFlow::SgFlow(uint32_t totalBytes, simgrid::surf::NetworkNS3Action * action) {
  totalBytes_ = totalBytes;
  remaining_ = totalBytes;
  action_ = action;
}

static SgFlow* getFlowFromSocket(ns3::Ptr<ns3::Socket> socket) {
	return (SgFlow*)xbt_dict_get_or_null(flowFromSock, transformSocketPtr(socket));
}

static void receive_callback(ns3::Ptr<ns3::Socket> socket){
  SgFlow* flow = getFlowFromSocket(socket);

  if (flow->finished_ == false){
    flow->finished_ = true;
    XBT_DEBUG("recv_cb of F[%p, %p, %d]", flow, flow->action_, flow->totalBytes_);
    XBT_DEBUG("Stop simulator at %f seconds", ns3::Simulator::Now().GetSeconds());
    ns3::Simulator::Stop(ns3::Seconds(0.0));
    ns3::Simulator::Run();
  }
}

static void send_callback(ns3::Ptr<ns3::Socket> sock, uint32_t txSpace){
	SgFlow* flow = getFlowFromSocket(sock);

	if (flow->remaining_ == 0) // all data was already buffered (and socket was already closed)
	  return;

	uint8_t *data = (uint8_t*)malloc(sizeof(uint8_t)*txSpace);

	while (flow->bufferedBytes_ < flow->totalBytes_ && sock->GetTxAvailable () > 0) {

      uint32_t toWrite = std::min ({flow->remaining_, txSpace, sock->GetTxAvailable ()});
      int amountSent = sock->Send (data, toWrite, 0);

      if(amountSent < 0)
    	  return;
      flow->bufferedBytes_ += amountSent;
      flow->remaining_ -= amountSent;

      XBT_DEBUG("send_cb of F[%p, %p, %d] (%d/%d) %d buffered", flow, flow->action_, flow->totalBytes_,
          flow->remaining_, flow->totalBytes_, amountSent);
    }
	free(data);

	if ((flow->bufferedBytes_) >= flow->totalBytes_)
		sock->Close();
}

static void datasent_callback(ns3::Ptr<ns3::Socket> socket, uint32_t dataSent){
  SgFlow* flow = getFlowFromSocket(socket);
  flow->sentBytes_ += dataSent;
  XBT_DEBUG("datasent_cb of F[%p, %p, %d] %d sent", flow, flow->action_, flow->totalBytes_, dataSent);
}

static void normalClose_callback(ns3::Ptr<ns3::Socket> socket){
  SgFlow* flow = getFlowFromSocket(socket);
  XBT_DEBUG("normalClose_cb of F[%p, %p, %d]", flow, flow->action_, flow->totalBytes_);
  receive_callback (socket);
}

static void errorClose_callback(ns3::Ptr<ns3::Socket> socket){
  SgFlow* flow = getFlowFromSocket(socket);
  XBT_DEBUG("errorClose_cb of F[%p, %p, %d]", flow, flow->action_, flow->totalBytes_);
  xbt_die("NS3: a socket was closed anormally");
}

static void succeededConnect_callback(ns3::Ptr<ns3::Socket> socket){
  SgFlow* flow = getFlowFromSocket(socket);
  XBT_DEBUG("succeededConnect_cb of F[%p, %p, %d]", flow, flow->action_, flow->totalBytes_);
}

static void failedConnect_callback(ns3::Ptr<ns3::Socket> socket){
  SgFlow* mysocket = getFlowFromSocket(socket);
  XBT_DEBUG("failedConnect_cb of F[%p, %p, %d]", mysocket, mysocket->action_, mysocket->totalBytes_);
  xbt_die("NS3: a socket failed to connect");
}

void StartFlow(ns3::Ptr<ns3::Socket> sock, const char *to, uint16_t port_number)
{
  ns3::InetSocketAddress serverAddr (to, port_number);

  sock->Connect(serverAddr);
  sock->SetSendCallback (MakeCallback (&send_callback));
  sock->SetRecvCallback (MakeCallback (&receive_callback));
  sock->SetDataSentCallback (MakeCallback (&datasent_callback));
  sock->SetConnectCallback (MakeCallback (&succeededConnect_callback), MakeCallback (&failedConnect_callback));
  sock->SetCloseCallbacks (MakeCallback (&normalClose_callback), MakeCallback (&errorClose_callback));

  SgFlow* flow = getFlowFromSocket(sock);
  XBT_DEBUG("startFlow_cb of F[%p, %p, %d] dest=%s port=%d", flow, flow->action_, flow->totalBytes_, to, port_number);
}
