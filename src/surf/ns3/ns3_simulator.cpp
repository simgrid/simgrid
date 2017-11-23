/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/ns3/ns3_simulator.hpp"
#include "xbt/log.h"
#include "xbt/sysdep.h"

#include <algorithm>

std::map<std::string, SgFlow*> flowFromSock; // ns3::sock -> SgFlow

static void receive_callback(ns3::Ptr<ns3::Socket> socket);
static void datasent_callback(ns3::Ptr<ns3::Socket> socket, uint32_t dataSent);

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ns3);

SgFlow::SgFlow(uint32_t totalBytes, simgrid::surf::NetworkNS3Action* action)
{
  totalBytes_ = totalBytes;
  remaining_  = totalBytes;
  action_     = action;
}

static SgFlow* getFlowFromSocket(ns3::Ptr<ns3::Socket> socket)
{
  auto it = flowFromSock.find(transformSocketPtr(socket));
  return (it == flowFromSock.end()) ? nullptr : it->second;
}

static void receive_callback(ns3::Ptr<ns3::Socket> socket)
{
  SgFlow* flow = getFlowFromSocket(socket);

  if (flow->finished_ == false) {
    flow->finished_ = true;
    XBT_DEBUG("recv_cb of F[%p, %p, %u]", flow, flow->action_, flow->totalBytes_);
    XBT_DEBUG("Stop simulator at %f seconds", ns3::Simulator::Now().GetSeconds());
    ns3::Simulator::Stop(ns3::Seconds(0.0));
    ns3::Simulator::Run();
  }
}

static void WriteUntilBufferFull(ns3::Ptr<ns3::Socket> sock, uint32_t txSpace)
{
  SgFlow* flow = getFlowFromSocket(sock);

  if (flow->remaining_ == 0) // all data was already buffered (and socket was already closed)
    return;

  /* While not all is buffered and there remain space in the buffers */
  while (flow->bufferedBytes_ < flow->totalBytes_ && sock->GetTxAvailable() > 0) {

    uint32_t toWrite = std::min({flow->remaining_, sock->GetTxAvailable()});
    if (toWrite == 0) // buffer full
      return;
    int amountSent = sock->Send(0, toWrite, 0);

    xbt_assert(amountSent > 0, "Since TxAvailable>0, amountSent should also >0");
    flow->bufferedBytes_ += amountSent;
    flow->remaining_ -= amountSent;

    XBT_DEBUG("%f: sent %d bytes over flow %p (still %u to go)", ns3::Simulator::Now().GetSeconds(), amountSent, flow,
              flow->remaining_);
  }

  if (flow->bufferedBytes_ >= flow->totalBytes_)
    sock->Close();
}

static void datasent_callback(ns3::Ptr<ns3::Socket> socket, uint32_t dataSent)
{
  /* The tracing wants to know */
  SgFlow* flow = getFlowFromSocket(socket);
  flow->sentBytes_ += dataSent;
  XBT_DEBUG("datasent_cb of F[%p, %p, %u] %u sent (%u total)", flow, flow->action_, flow->totalBytes_, dataSent,
            flow->sentBytes_);
}

static void normalClose_callback(ns3::Ptr<ns3::Socket> socket)
{
  SgFlow* flow = getFlowFromSocket(socket);
  XBT_DEBUG("normalClose_cb of F[%p, %p, %u]", flow, flow->action_, flow->totalBytes_);
  receive_callback(socket);
}

static void errorClose_callback(ns3::Ptr<ns3::Socket> socket)
{
  SgFlow* flow = getFlowFromSocket(socket);
  XBT_DEBUG("errorClose_cb of F[%p, %p, %u]", flow, flow->action_, flow->totalBytes_);
  xbt_die("NS3: a socket was closed anormally");
}

static void succeededConnect_callback(ns3::Ptr<ns3::Socket> socket)
{
  SgFlow* flow = getFlowFromSocket(socket);
  XBT_DEBUG("succeededConnect_cb of F[%p, %p, %u]", flow, flow->action_, flow->totalBytes_);
}

static void failedConnect_callback(ns3::Ptr<ns3::Socket> socket)
{
  SgFlow* mysocket = getFlowFromSocket(socket);
  XBT_DEBUG("failedConnect_cb of F[%p, %p, %u]", mysocket, mysocket->action_, mysocket->totalBytes_);
  xbt_die("NS3: a socket failed to connect");
}

void StartFlow(ns3::Ptr<ns3::Socket> sock, const char* to, uint16_t port_number)
{
  SgFlow* flow = getFlowFromSocket(sock);
  ns3::InetSocketAddress serverAddr(to, port_number);

  sock->Connect(serverAddr);
  // tell the tcp implementation to call WriteUntilBufferFull again
  // if we blocked and new tx buffer space becomes available
  sock->SetSendCallback(MakeCallback(&WriteUntilBufferFull));
  // Note when the send is over
  sock->SetRecvCallback(MakeCallback(&receive_callback));
  // Keep track of what was used (for the TRACING module)
  sock->SetDataSentCallback(MakeCallback(&datasent_callback));
  XBT_DEBUG("startFlow of F[%p, %p, %u] dest=%s port=%d", flow, flow->action_, flow->totalBytes_, to, port_number);

  // WriteUntilBufferFull (sock, sock->GetTxAvailable ());
  /*
  sock->SetSendCallback(MakeCallback(&send_callback));
  sock->SetConnectCallback(MakeCallback(&succeededConnect_callback), MakeCallback(&failedConnect_callback));
  sock->SetCloseCallbacks(MakeCallback(&normalClose_callback), MakeCallback(&errorClose_callback));
  send_callback(sock, sock->GetTxAvailable ());
   */
}
