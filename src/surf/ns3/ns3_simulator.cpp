/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/ns3/ns3_simulator.hpp"
#include "xbt/log.h"
#include "xbt/sysdep.h"

#include <ns3/ipv4-address-helper.h>
#include <ns3/point-to-point-helper.h>
#include <ns3/application-container.h>
#include <ns3/ptr.h>
#include <ns3/callback.h>
#include <ns3/packet-sink.h>

#include <algorithm>

std::map<std::string, SgFlow*> flow_from_sock; // ns3::sock -> SgFlow
std::map<std::string, ns3::ApplicationContainer> sink_from_sock; // ns3::sock -> ns3::PacketSink

static void receive_callback(ns3::Ptr<ns3::Socket> socket);
static void datasent_cb(ns3::Ptr<ns3::Socket> socket, uint32_t dataSent);

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ns3);

SgFlow::SgFlow(uint32_t totalBytes, simgrid::kernel::resource::NetworkNS3Action* action)
{
  total_bytes_ = totalBytes;
  remaining_  = totalBytes;
  action_     = action;
}

static SgFlow* getFlowFromSocket(ns3::Ptr<ns3::Socket> socket)
{
  auto it = flow_from_sock.find(transform_socket_ptr(socket));
  return (it == flow_from_sock.end()) ? nullptr : it->second;
}

static ns3::ApplicationContainer* getSinkFromSocket(ns3::Ptr<ns3::Socket> socket)
{
  auto it = sink_from_sock.find(transform_socket_ptr(socket));
  return (it == sink_from_sock.end()) ? nullptr : &(it->second);
}

static void receive_callback(ns3::Ptr<ns3::Socket> socket)
{
  SgFlow* flow = getFlowFromSocket(socket);
  XBT_DEBUG("received on F[%p, total: %u, remain: %u]", flow, flow->total_bytes_, flow->remaining_);

  if (flow->finished_ == false) {
    flow->finished_ = true;
    XBT_DEBUG("recv_cb of F[%p, %p, %u]", flow, flow->action_, flow->total_bytes_);
    XBT_DEBUG("Stop simulator at %f seconds", ns3::Simulator::Now().GetSeconds());
    ns3::Simulator::Stop();
  }
}

static void send_cb(ns3::Ptr<ns3::Socket> sock, uint32_t txSpace)
{
  SgFlow* flow = getFlowFromSocket(sock);
  ns3::ApplicationContainer* sink = getSinkFromSocket(sock);
  XBT_DEBUG("Asked to write on F[%p, total: %u, remain: %u]", flow, flow->total_bytes_, flow->remaining_);

  if (flow->remaining_ == 0) // all data was already buffered (and socket was already closed)
    return;

  /* While not all is buffered and there remain space in the buffers */
  while (flow->buffered_bytes_ < flow->total_bytes_ && sock->GetTxAvailable() > 0) {
    // Send at most 1040 bytes (data size in a TCP packet), as ns-3 seems to not split correctly by itself
    uint32_t toWrite = std::min({flow->remaining_, sock->GetTxAvailable(), std::uint32_t(1040)});

    if (toWrite == 0) { // buffer full
      XBT_DEBUG("%f: buffer full on flow %p (still %u to go)", ns3::Simulator::Now().GetSeconds(), flow,
                flow->remaining_);
      return;
    }
    int amountSent = sock->Send(0, toWrite, 0);

    xbt_assert(amountSent > 0, "Since TxAvailable>0, amountSent should also >0");
    flow->buffered_bytes_ += amountSent;
    flow->remaining_ -= amountSent;

    XBT_DEBUG("%f: sent %d bytes over flow %p (still %u to go)", ns3::Simulator::Now().GetSeconds(), amountSent, flow,
              flow->remaining_);
  }

  if (flow->buffered_bytes_ >= flow->total_bytes_){
    XBT_DEBUG("Closing Sockets of flow %p", flow);
    // Closing the sockets of the receiving application
    ns3::Ptr<ns3::PacketSink> app = ns3::DynamicCast<ns3::PacketSink, ns3::Application>(sink->Get(0));
    ns3::Ptr<ns3::Socket> listening_sock = app->GetListeningSocket();
    listening_sock->Close();
    listening_sock->SetRecvCallback(ns3::MakeNullCallback<void, ns3::Ptr<ns3::Socket>>());
    for(ns3::Ptr<ns3::Socket> accepted_sock : app->GetAcceptedSockets())
      accepted_sock->Close();
    // Closing the socket of the sender
    sock->Close();
  }
}

static void datasent_cb(ns3::Ptr<ns3::Socket> socket, uint32_t dataSent)
{
  /* The tracing wants to know */
  SgFlow* flow = getFlowFromSocket(socket);
  flow->sent_bytes_ += dataSent;
  XBT_DEBUG("datasent_cb of F[%p, %p, %u] %u sent (%u total)", flow, flow->action_, flow->total_bytes_, dataSent,
            flow->sent_bytes_);
}

static void normalClose_callback(ns3::Ptr<ns3::Socket> socket)
{
  SgFlow* flow = getFlowFromSocket(socket);
  XBT_DEBUG("normalClose_cb of F[%p, %p] total: %u; sent: %u", flow, flow->action_, flow->total_bytes_,
            flow->sent_bytes_);
  // xbt_assert(flow->total_bytes_ == flow->sent_bytes_,
  //    "total_bytes (=%u) is not sent_bytes(=%u)", flow->total_bytes_ , flow->sent_bytes_);
  // xbt_assert(flow->remaining_ == 0, "Remaining is not 0 but %u", flow->remaining_);
  receive_callback(socket);
}

static void errorClose_callback(ns3::Ptr<ns3::Socket> socket)
{
  SgFlow* flow = getFlowFromSocket(socket);
  XBT_DEBUG("errorClose_cb of F[%p, %p, %u]", flow, flow->action_, flow->total_bytes_);
  xbt_die("ns-3: a socket was closed anormally");
}

static void succeededConnect_callback(ns3::Ptr<ns3::Socket> socket)
{
  SgFlow* flow = getFlowFromSocket(socket);
  XBT_DEBUG("succeededConnect_cb of F[%p, %p, %u]", flow, flow->action_, flow->total_bytes_);
}

static void failedConnect_callback(ns3::Ptr<ns3::Socket> socket)
{
  SgFlow* mysocket = getFlowFromSocket(socket);
  XBT_DEBUG("failedConnect_cb of F[%p, %p, %u]", mysocket, mysocket->action_, mysocket->total_bytes_);
  xbt_die("ns-3: a socket failed to connect");
}

void start_flow(ns3::Ptr<ns3::Socket> sock, const char* to, uint16_t port_number)
{
  SgFlow* flow = getFlowFromSocket(sock);
  ns3::InetSocketAddress serverAddr(to, port_number);

  sock->Connect(serverAddr);
  // tell the tcp implementation to call send_cb again
  // if we blocked and new tx buffer space becomes available
  sock->SetSendCallback(MakeCallback(&send_cb));
  // Notice when we actually sent some data (mostly for the TRACING module)
  sock->SetDataSentCallback(MakeCallback(&datasent_cb));

  XBT_DEBUG("startFlow of F[%p, %p, %u] dest=%s port=%d", flow, flow->action_, flow->total_bytes_, to, port_number);

  sock->SetConnectCallback(MakeCallback(&succeededConnect_callback), MakeCallback(&failedConnect_callback));
  sock->SetCloseCallbacks(MakeCallback(&normalClose_callback), MakeCallback(&errorClose_callback));
}
