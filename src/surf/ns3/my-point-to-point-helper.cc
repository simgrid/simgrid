/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/point-to-point-remote-channel.h"
#include "ns3/queue.h"
#include "ns3/config.h"
#include "ns3/packet.h"
#include "ns3/names.h"
#include "ns3/string.h"
#include "ns3/mpi-interface.h"
#include "ns3/mpi-receiver.h"

#include "ns3/trace-helper.h"
#include "my-point-to-point-helper.h"

NS_LOG_COMPONENT_DEFINE ("MyPointToPointHelper");

///> RED Parameters  see src/node/red-queue.* for details
//.AddAttribute ("Mode",
//                "Whether to use Bytes (see MaxBytes) or Packets (see MaxPackets) as the maximum queue size metric.",
//                EnumValue (BYTES), ///> currently supports BYTES only
//                MakeEnumAccessor (&RedQueue::SetMode),
//                MakeEnumChecker (BYTES, "Bytes",
//                                 PACKETS, "Packets"))
// .AddAttribute ("MaxPackets",
//                "The maximum number of packets accepted by this RedQueue.",
//                UintegerValue (100),
//                MakeUintegerAccessor (&RedQueue::m_maxPackets),
//                MakeUintegerChecker<uint32_t> ())
// .AddAttribute ("MaxBytes",
//                "The maximum number of bytes accepted by this RedQueue.",
//                UintegerValue (100000),
//                MakeUintegerAccessor (&RedQueue::m_maxBytes),
//                MakeUintegerChecker<uint32_t> ())
// .AddAttribute ("m_burst",
//                "maximum number of m_burst packets accepted by this queue",
//                UintegerValue (6), ///> bursts must be > minTh/avpkt
//                MakeUintegerAccessor (&RedQueue::m_burst),
//                MakeUintegerChecker<uint32_t> ())
// .AddAttribute ("m_avPkt",
//                "In bytes, use with m_burst to determine the time constant for average queue size calculations",
//                UintegerValue (1024), ///> average packet size
//                MakeUintegerAccessor (&RedQueue::m_avPkt),
//                MakeUintegerChecker<uint32_t> ())
// .AddAttribute ("m_minTh",
//                "Average queue size at which marking becomes a m_prob",
//                UintegerValue (5120), ///> in bytes  1024x5
//                MakeUintegerAccessor (&RedQueue::m_minTh),
//                MakeUintegerChecker<uint32_t> ())
// .AddAttribute ("m_maxTh",
//                "Maximal marking m_prob, should be at least twice min to prevent synchronous retransmits",
//                UintegerValue (15360), ///> in bytes 1024x15
//                MakeUintegerAccessor (&RedQueue::m_maxTh),
//                MakeUintegerChecker<uint32_t> ())
// .AddAttribute ("m_rate",
//                "this m_rate is used for calculating the average queue size after some idle time.",
//                UintegerValue (1500000), ///> in bps, should be set to bandwidth of interface
//                MakeUintegerAccessor (&RedQueue::m_rate),
//                MakeUintegerChecker<uint64_t> ())
// .AddAttribute ("m_prob",
//                "Probability for marking, suggested values are 0.01 and 0.02",
//                DoubleValue (0.02),
//                MakeDoubleAccessor (&RedQueue::m_prob),
//                MakeDoubleChecker <double> ())
std::string qMode = "Bytes";
std::string qBurst = "6";
std::string qAvPkt = "1024";
std::string qLimit = "25600"; //"100000";
std::string qthMin = "5120";  // 1024 x 5 bytes
std::string qthMax = "15360"; // 1024 x 15 bytes
std::string qIdleRate = "1500000";  //1.5 Mbps
std::string qProb = "0.02";

namespace ns3 {

MyPointToPointHelper::MyPointToPointHelper ()
{
  m_queueFactory.SetTypeId ("ns3::DropTailQueue");
  m_queueFactory_red.SetTypeId ("ns3::RedQueue");
//  m_queueFactory_red.Set ("Mode",    StringValue (qMode));
//  m_queueFactory_red.Set ("MaxBytes",StringValue (qLimit));
//  m_queueFactory_red.Set ("m_burst", StringValue (qBurst));
//  m_queueFactory_red.Set ("m_avPkt", StringValue (qAvPkt));
//  m_queueFactory_red.Set ("m_minTh", StringValue (qthMin));
//  m_queueFactory_red.Set ("m_maxTh", StringValue (qthMax));
//  m_queueFactory_red.Set ("m_rate",  StringValue (qIdleRate));
//  m_queueFactory_red.Set ("m_prob",  StringValue (qProb));
  m_deviceFactory.SetTypeId ("ns3::PointToPointNetDevice");
  m_channelFactory.SetTypeId ("ns3::PointToPointChannel");
  m_remoteChannelFactory.SetTypeId ("ns3::PointToPointRemoteChannel");
}

void 
MyPointToPointHelper::SetQueue (std::string type,
                              std::string n1, const AttributeValue &v1,
                              std::string n2, const AttributeValue &v2,
                              std::string n3, const AttributeValue &v3,
                              std::string n4, const AttributeValue &v4,
                              std::string n5, const AttributeValue &v5,
                              std::string n6, const AttributeValue &v6,
                              std::string n7, const AttributeValue &v7,
                              std::string n8, const AttributeValue &v8)
{
  m_queueFactory.SetTypeId (type);
  m_queueFactory.Set (n1, v1);
  m_queueFactory.Set (n2, v2);
  m_queueFactory.Set (n3, v3);
  m_queueFactory.Set (n4, v4);
  m_queueFactory.Set (n5, v5);
  m_queueFactory.Set (n6, v6);
  m_queueFactory.Set (n7, v7);
  m_queueFactory.Set (n8, v8);
}

void 
MyPointToPointHelper::SetDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  m_deviceFactory.Set (n1, v1);
}

void 
MyPointToPointHelper::SetChannelAttribute (std::string n1, const AttributeValue &v1)
{
  m_channelFactory.Set (n1, v1);
  m_remoteChannelFactory.Set (n1, v1);
}

void 
MyPointToPointHelper::EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, bool promiscuous, bool explicitFilename)
{
  //
  // All of the Pcap enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type PointToPointNetDevice.
  //
  Ptr<PointToPointNetDevice> device = nd->GetObject<PointToPointNetDevice> ();
  if (device == 0)
    {
      NS_LOG_INFO ("MyPointToPointHelper::EnablePcapInternal(): Device " << device << " not of type ns3::PointToPointNetDevice");
      return;
    }

  PcapHelper pcapHelper;

  std::string filename;
  if (explicitFilename)
    {
      filename = prefix;
    }
  else
    {
      filename = pcapHelper.GetFilenameFromDevice (prefix, device);
    }

  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile (filename, std::ios::out, 
                                                     PcapHelper::DLT_PPP);
  pcapHelper.HookDefaultSink<PointToPointNetDevice> (device, "PromiscSniffer", file);
}

void 
MyPointToPointHelper::EnableAsciiInternal (
  Ptr<OutputStreamWrapper> stream, 
  std::string prefix, 
  Ptr<NetDevice> nd,
  bool explicitFilename)
{
  //
  // All of the ascii enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type PointToPointNetDevice.
  //
  Ptr<PointToPointNetDevice> device = nd->GetObject<PointToPointNetDevice> ();
  if (device == 0)
    {
      NS_LOG_INFO ("MyPointToPointHelper::EnableAsciiInternal(): Device " << device <<
                   " not of type ns3::PointToPointNetDevice");
      return;
    }

  //
  // Our default trace sinks are going to use packet printing, so we have to 
  // make sure that is turned on.
  //
  Packet::EnablePrinting ();

  //
  // If we are not provided an OutputStreamWrapper, we are expected to create 
  // one using the usual trace filename conventions and do a Hook*WithoutContext
  // since there will be one file per context and therefore the context would
  // be redundant.
  //
  if (stream == 0)
    {
      //
      // Set up an output stream object to deal with private ofstream copy 
      // constructor and lifetime issues.  Let the helper decide the actual
      // name of the file given the prefix.
      //
      AsciiTraceHelper asciiTraceHelper;

      std::string filename;
      if (explicitFilename)
        {
          filename = prefix;
        }
      else
        {
          filename = asciiTraceHelper.GetFilenameFromDevice (prefix, device);
        }

      Ptr<OutputStreamWrapper> theStream = asciiTraceHelper.CreateFileStream (filename);

      //
      // The MacRx trace source provides our "r" event.
      //
      asciiTraceHelper.HookDefaultReceiveSinkWithoutContext<PointToPointNetDevice> (device, "MacRx", theStream);

      //
      // The "+", '-', and 'd' events are driven by trace sources actually in the
      // transmit queue.
      //
      Ptr<Queue> queue = device->GetQueue ();
      asciiTraceHelper.HookDefaultEnqueueSinkWithoutContext<Queue> (queue, "Enqueue", theStream);
      asciiTraceHelper.HookDefaultDropSinkWithoutContext<Queue> (queue, "Drop", theStream);
      asciiTraceHelper.HookDefaultDequeueSinkWithoutContext<Queue> (queue, "Dequeue", theStream);

      // PhyRxDrop trace source for "d" event
      asciiTraceHelper.HookDefaultDropSinkWithoutContext<PointToPointNetDevice> (device, "PhyRxDrop", theStream);

      return;
    }

  //
  // If we are provided an OutputStreamWrapper, we are expected to use it, and
  // to providd a context.  We are free to come up with our own context if we
  // want, and use the AsciiTraceHelper Hook*WithContext functions, but for 
  // compatibility and simplicity, we just use Config::Connect and let it deal
  // with the context.
  //
  // Note that we are going to use the default trace sinks provided by the 
  // ascii trace helper.  There is actually no AsciiTraceHelper in sight here,
  // but the default trace sinks are actually publicly available static 
  // functions that are always there waiting for just such a case.
  //
  uint32_t nodeid = nd->GetNode ()->GetId ();
  uint32_t deviceid = nd->GetIfIndex ();
  std::ostringstream oss;

  oss << "/NodeList/" << nd->GetNode ()->GetId () << "/DeviceList/" << deviceid << "/$ns3::PointToPointNetDevice/MacRx";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultReceiveSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::PointToPointNetDevice/TxQueue/Enqueue";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultEnqueueSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::PointToPointNetDevice/TxQueue/Dequeue";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDequeueSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::PointToPointNetDevice/TxQueue/Drop";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));

  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::PointToPointNetDevice/PhyRxDrop";
  Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));
}

NetDeviceContainer 
MyPointToPointHelper::Install (NodeContainer c)
{
  NS_ASSERT (c.GetN () == 2);
  return Install (c.Get (0), c.Get (1));
}

NetDeviceContainer 
MyPointToPointHelper::Install (Ptr<Node> a, e_ns3_network_element_type_t type_a, Ptr<Node> b, e_ns3_network_element_type_t type_b)
{
  NetDeviceContainer container;
  Ptr<Queue> queueA;
  Ptr<Queue> queueB;

  Ptr<PointToPointNetDevice> devA = m_deviceFactory.Create<PointToPointNetDevice> ();
  devA->SetAddress (Mac48Address::Allocate ());
  a->AddDevice (devA);

  if(type_a == NS3_NETWORK_ELEMENT_ROUTER){
	queueA = m_queueFactory_red.Create<Queue> ();
  }
  else
	  queueA = m_queueFactory.Create<Queue> ();
  devA->SetQueue (queueA);

  Ptr<PointToPointNetDevice> devB = m_deviceFactory.Create<PointToPointNetDevice> ();
  devB->SetAddress (Mac48Address::Allocate ());
  b->AddDevice (devB);

  if(type_b == NS3_NETWORK_ELEMENT_ROUTER){
	queueB = m_queueFactory_red.Create<Queue> ();
  }
  else
	  queueB = m_queueFactory.Create<Queue> ();
  devB->SetQueue (queueB);

  // If MPI is enabled, we need to see if both nodes have the same system id 
  // (rank), and the rank is the same as this instance.  If both are true, 
  //use a normal p2p channel, otherwise use a remote channel
  bool useNormalChannel = true;
  Ptr<PointToPointChannel> channel = 0;
  if (MpiInterface::IsEnabled ())
    {
      uint32_t n1SystemId = a->GetSystemId ();
      uint32_t n2SystemId = b->GetSystemId ();
      uint32_t currSystemId = MpiInterface::GetSystemId ();
      if (n1SystemId != currSystemId || n2SystemId != currSystemId) 
        {
          useNormalChannel = false;
        }
    }
  if (useNormalChannel)
    {
      channel = m_channelFactory.Create<PointToPointChannel> ();
    }
  else
    {
      channel = m_remoteChannelFactory.Create<PointToPointRemoteChannel> ();
      Ptr<MpiReceiver> mpiRecA = CreateObject<MpiReceiver> ();
      Ptr<MpiReceiver> mpiRecB = CreateObject<MpiReceiver> ();
      mpiRecA->SetReceiveCallback (MakeCallback (&PointToPointNetDevice::Receive, devA));
      mpiRecB->SetReceiveCallback (MakeCallback (&PointToPointNetDevice::Receive, devB));
      devA->AggregateObject (mpiRecA);
      devB->AggregateObject (mpiRecB);
    }

  devA->Attach (channel);
  devB->Attach (channel);
  container.Add (devA);
  container.Add (devB);

  return container;
}

NetDeviceContainer 
MyPointToPointHelper::Install (Ptr<Node> a, Ptr<Node> b)
{
  NetDeviceContainer container;

  Ptr<PointToPointNetDevice> devA = m_deviceFactory.Create<PointToPointNetDevice> ();
  devA->SetAddress (Mac48Address::Allocate ());
  a->AddDevice (devA);
  Ptr<Queue> queueA = m_queueFactory.Create<Queue> ();
  devA->SetQueue (queueA);
  Ptr<PointToPointNetDevice> devB = m_deviceFactory.Create<PointToPointNetDevice> ();
  devB->SetAddress (Mac48Address::Allocate ());
  b->AddDevice (devB);
  Ptr<Queue> queueB = m_queueFactory.Create<Queue> ();
  devB->SetQueue (queueB);
  // If MPI is enabled, we need to see if both nodes have the same system id
  // (rank), and the rank is the same as this instance.  If both are true,
  //use a normal p2p channel, otherwise use a remote channel
  bool useNormalChannel = true;
  Ptr<PointToPointChannel> channel = 0;
  if (MpiInterface::IsEnabled ())
    {
      uint32_t n1SystemId = a->GetSystemId ();
      uint32_t n2SystemId = b->GetSystemId ();
      uint32_t currSystemId = MpiInterface::GetSystemId ();
      if (n1SystemId != currSystemId || n2SystemId != currSystemId)
        {
          useNormalChannel = false;
        }
    }
  if (useNormalChannel)
    {
      channel = m_channelFactory.Create<PointToPointChannel> ();
    }
  else
    {
      channel = m_remoteChannelFactory.Create<PointToPointRemoteChannel> ();
      Ptr<MpiReceiver> mpiRecA = CreateObject<MpiReceiver> ();
      Ptr<MpiReceiver> mpiRecB = CreateObject<MpiReceiver> ();
      mpiRecA->SetReceiveCallback (MakeCallback (&PointToPointNetDevice::Receive, devA));
      mpiRecB->SetReceiveCallback (MakeCallback (&PointToPointNetDevice::Receive, devB));
      devA->AggregateObject (mpiRecA);
      devB->AggregateObject (mpiRecB);
    }

  devA->Attach (channel);
  devB->Attach (channel);
  container.Add (devA);
  container.Add (devB);

  return container;
}

NetDeviceContainer
MyPointToPointHelper::Install (Ptr<Node> a, std::string bName)
{
  Ptr<Node> b = Names::Find<Node> (bName);
  return Install (a, b);
}

NetDeviceContainer 
MyPointToPointHelper::Install (std::string aName, Ptr<Node> b)
{
  Ptr<Node> a = Names::Find<Node> (aName);
  return Install (a, b);
}

NetDeviceContainer 
MyPointToPointHelper::Install (std::string aName, std::string bName)
{
  Ptr<Node> a = Names::Find<Node> (aName);
  Ptr<Node> b = Names::Find<Node> (bName);
  return Install (a, b);
}

} // namespace ns3
