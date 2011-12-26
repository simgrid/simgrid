/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Regents of the University of California
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
 * Author: Duy Nguyen<duy@soe.ucsc.edu>
 *
 */

#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "red-queue.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/random-variable.h"

#include <cstdlib>

NS_LOG_COMPONENT_DEFINE ("red");

#define RED_STATS_TABLE_SIZE 256
#define RED_STATS_MASK (RED_STATS_TABLE_SIZE - 1)

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RedQueue);

TypeId RedQueue::GetTypeId (void)
{
  ///< Note: these paramemters must be worked out beforehand for RED to work correctly
  ///< How these parameters are set up can affect RED performance greatly
  static TypeId tid = TypeId ("ns3::RedQueue")
                      .SetParent<Queue> ()
                      .AddConstructor<RedQueue> ()
                      .AddAttribute ("Mode",
                                     "Whether to use Bytes (see MaxBytes) or Packets (see MaxPackets) as the maximum queue size metric.",
                                     EnumValue (BYTES), ///> currently supports BYTES only
                                     MakeEnumAccessor (&RedQueue::SetMode),
                                     MakeEnumChecker (BYTES, "Bytes",
                                                      PACKETS, "Packets"))
                      .AddAttribute ("MaxPackets",
                                     "The maximum number of packets accepted by this RedQueue.",
                                     UintegerValue (100),
                                     MakeUintegerAccessor (&RedQueue::m_maxPackets),
                                     MakeUintegerChecker<uint32_t> ())
                      .AddAttribute ("MaxBytes",
                                     "The maximum number of bytes accepted by this RedQueue.",
                                     UintegerValue (100000),
                                     MakeUintegerAccessor (&RedQueue::m_maxBytes),
                                     MakeUintegerChecker<uint32_t> ())
                      .AddAttribute ("m_burst",
                                     "maximum number of m_burst packets accepted by this queue",
                                     UintegerValue (6), ///> bursts must be > minTh/avpkt
                                     MakeUintegerAccessor (&RedQueue::m_burst),
                                     MakeUintegerChecker<uint32_t> ())
                      .AddAttribute ("m_avPkt",
                                     "In bytes, use with m_burst to determine the time constant for average queue size calculations",
                                     UintegerValue (1024), ///> average packet size
                                     MakeUintegerAccessor (&RedQueue::m_avPkt),
                                     MakeUintegerChecker<uint32_t> ())
                      .AddAttribute ("m_minTh",
                                     "Average queue size at which marking becomes a m_prob",
                                     UintegerValue (5120), ///> in bytes  1024x5
                                     MakeUintegerAccessor (&RedQueue::m_minTh),
                                     MakeUintegerChecker<uint32_t> ())
                      .AddAttribute ("m_maxTh",
                                     "Maximal marking m_prob, should be at least twice min to prevent synchronous retransmits",
                                     UintegerValue (15360), ///> in bytes 1024x15
                                     MakeUintegerAccessor (&RedQueue::m_maxTh),
                                     MakeUintegerChecker<uint32_t> ())
                      .AddAttribute ("m_rate",
                                     "this m_rate is used for calculating the average queue size after some idle time.",
                                     UintegerValue (1500000), ///> in bps, should be set to bandwidth of interface
                                     MakeUintegerAccessor (&RedQueue::m_rate),
                                     MakeUintegerChecker<uint64_t> ())
                      .AddAttribute ("m_prob",
                                     "Probability for marking, suggested values are 0.01 and 0.02",
                                     DoubleValue (0.02),
                                     MakeDoubleAccessor (&RedQueue::m_prob),
                                     MakeDoubleChecker <double> ())
  ;

  return tid;
}

RedQueue::RedQueue ()
  : Queue (),
    m_packets (),
    m_bytesInQueue (0),
    m_wLog (0),
    m_pLog (0),
    m_rmask (0),
    m_scellLog (0),
    m_scellMax (0),
    m_count (-1),
    m_randNum (0),
    m_qavg (0),
    m_initialized (false)
{

  m_sTable = Uint32tVector (RED_STATS_TABLE_SIZE);

}

RedQueue::~RedQueue ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
RedQueue::SetMode (enum Mode mode)
{
  NS_LOG_FUNCTION (mode);
  m_mode = mode;
}

RedQueue::Mode
RedQueue::GetMode (void)
{
  return m_mode;
}

uint64_t
RedQueue::GetAverageQueueSize (void)
{
  return m_qavg;
}


/**
 * The paper says:
 * Given minimum threshold min_th and that we wish to allow bursts of L packets
 * Then Wq should be chosen to satisfy avg_L < min_th
 * L + 1 + [(1-Wq)^(L+1) - 1]/ Wq    <  min_th
 * L + 1 - min_th < [1 - (1-Wq)^(L+1)]/Wq
 * i.e. given min_th 5, L=50, necessary that Wq <= 0.0042
 *
 * Hence
 * burst + 1 - minTh/avPkt < (1-(1-W)^burst)/W
 * this low-pass filter is used to calculate the avg queue size
 *
 */
uint32_t
RedQueue::evalEwma (uint32_t minTh, uint32_t burst, uint32_t avpkt)
{
  NS_LOG_FUNCTION (this);
  uint32_t wlog = 1;


  ///< Just a random W
  double W = 0.5;

  double temp = 0;

  ///< Note: bursts must be larger than minTh/avpkt for it to work
  temp = (double)burst + 1 - (double)minTh / avpkt;

  NS_LOG_DEBUG ( "\t temp =" << temp);

  if (temp < 1.0)
    {
      NS_LOG_DEBUG ("\tFailed to calculate EWMA constant");
      return -1;
    }

  /**
   * wlog =1 , W = .5
   * wlog =2 , W = .25
   * wlog =3 , W = .125
   * wlog =4 , W = .0625
   * wlog =5 , W = .03125
   * wlog =6 , W = .015625
   * wlog =7 , W = .0078125
   * wlog =8 , W = .00390625
   * wlog =9 , W = .001953125
   * wlog =10, W = .0009765625
   */
  for (wlog = 1; wlog < 32; wlog++, W /= 2)
    {
      if (temp <= (1 - pow (1 - W, burst)) / W )
        {
          NS_LOG_DEBUG ("\t wlog=" << wlog);
          return wlog;
        }
    }

  NS_LOG_DEBUG ("\tFailed to calculate EWMA constant");
  return -1;
}

/**
 *
 * Plog = log (prob / (maxTh -minTh) );
 *
 * Paper says: When a packet arrives at the gateway and the average queue size
 * is between min_th and max_th, the initial packet marking probability is:
 * Pb = C1*avg - C2
 * where,
 * C1 = maxP/(max_th - mint_th)
 * C2 = maxP*min_th/(max_th - mint_th)
 * maxP could be chosen so that C1 a power of two
 */
uint32_t
RedQueue::evalP (uint32_t minTh, uint32_t maxTh, double prob)
{
  NS_LOG_FUNCTION (this);

  uint32_t i = maxTh - minTh ;

  if (i <= 0)
    {
      NS_LOG_DEBUG ("maxTh - minTh = 0");
      return -1;
    }

  prob /= i;

  ///< It returns the index that makes C1 a power of two
  for (i = 0; i < 32; i++)
    {
      if (prob > 1.0)
        {
          break;
        }
      prob *= 2;
    }

  ///< Error checking
  if (i >= 32 )
    {
      NS_LOG_DEBUG ("i >= 32, this shouldn't happen");
      return -1;
    }

  NS_LOG_DEBUG ("\t i(makes C1 power of two)=" << i);
  return i;
}


/**
 * avg = avg*(1-W)^m  where m = t/xmitTime
 *
 * m_sTable[ t/2^cellLog] = -log(1-W) * t/xmitTime
 * m_sTable[ t >> cellLog]= -log(1-W) * t/xmitTime
 *
 * t is converted to t/2^cellLog for storage in the table
 * find out what is cellLog and return it
 *
 */
uint32_t
RedQueue::evalIdleDamping (uint32_t wLog, uint32_t avpkt, uint32_t bps)
{
  NS_LOG_FUNCTION (this);

  ///> in microsecond ticks: 1 sec = 1000000 microsecond ticks
  double xmitTime =  ((double) avpkt / bps) * 1000000;

  ///> -log(1 - 1/2^wLog) / xmitTime
  ///> note W = 1/2^wLog
  double wLogTemp = -log (1.0 - 1.0 / (1 << wLog)) / xmitTime;


  ///> the maximum allow idle time
  double maxTime = 31 / wLogTemp;

  NS_LOG_DEBUG ("\t xmitTime=" << xmitTime << " wLogTemp=" << wLogTemp
                               << " maxTime=" << maxTime);


  uint32_t cLog, i;

  for (cLog = 0; cLog < 32; cLog++)
    {

      ///> maxTime < 512* 2^cLog
      ///>  finds the cLog that's able to cover this maxTime
      if (maxTime / (1 << cLog) < 512)
        {
          break;
        }

    }
  if (cLog >= 32)
    {
      return -1;
    }

  m_sTable[0] = 0;

  for (i = 1; i < 255; i++)
    {
      ///> wLogTemp * i * 2^cLog
      m_sTable[i] = (i << cLog) * wLogTemp;


      if (m_sTable[i] > 31)
        {
          m_sTable[i] = 31;
        }
    }

  m_sTable[255] = 31;

  NS_LOG_DEBUG ("\t cLog=" << cLog);
  return cLog;
}


///> red random mask
uint32_t
RedQueue::Rmask (uint32_t pLog)
{
  ///> ~OUL creates a 32 bit mask
  ///> 2^Plog - 1
  return pLog < 32 ? ((1 << pLog) - 1) : ~0UL;

}


void
RedQueue::SetParams (uint32_t minTh, uint32_t maxTh,
                     uint32_t wLog, uint32_t pLog, uint64_t scellLog)
{
  NS_LOG_FUNCTION (this);

  m_qavg = 0;
  m_count = -1;
  m_minTh = minTh;
  m_maxTh = maxTh;
  m_wLog = wLog;
  m_pLog = pLog;
  m_rmask = Rmask (pLog);
  m_scellLog = scellLog;
  m_scellMax = (255 << m_scellLog);

  NS_LOG_DEBUG ("\t m_wLog" << m_wLog << " m_pLog" << m_pLog << " m_scellLog" << m_scellLog
                            << " m_minTh" << m_minTh << " m_maxTh" << m_maxTh
                            << " rmask=" << m_rmask << " m_scellMax=" << m_scellMax);
}

int
RedQueue::IsIdling ()
{
  NS_LOG_FUNCTION_NOARGS ();

  //use IsZero instead
  if ( m_idleStart.GetNanoSeconds () != 0)
    {
      NS_LOG_DEBUG ("\t IsIdling");
    }

  return m_idleStart.GetNanoSeconds () != 0;
}
void
RedQueue::StartIdlePeriod ()
{
  NS_LOG_FUNCTION_NOARGS ();

  m_idleStart = Simulator::Now ();
}
void
RedQueue::EndIdlePeriod ()
{
  NS_LOG_FUNCTION_NOARGS ();

  m_idleStart = NanoSeconds (0);
}
void
RedQueue::Restart ()
{

  NS_LOG_FUNCTION_NOARGS ();

  EndIdlePeriod ();
  m_qavg = 0;
  m_count = -1;

}

/**
 * m = idletime / s
 *
 * m  is the number of pkts that might have been transmitted by the gateway
 * during the time that the queue was free
 * s is a typical transmission for a packet
 *
 * m = idletime / (average pkt size / bandwidth)
 *
 * avg = avg *(1-W)^m
 *
 * We need to precompute a table for this calculation because of the exp power
 *
 */
uint64_t
RedQueue::AvgFromIdleTime ()
{
  NS_LOG_FUNCTION_NOARGS ();

  uint64_t idleTime;
  int shift;

  idleTime = ns3::Time(Simulator::Now() - m_idleStart).GetMicroSeconds();
  //idleTime = RedTimeToInteger (Simulator::Now() - m_idleStart, Time::US);

  if (idleTime > m_scellMax)
    {
      idleTime = m_scellMax; 
    }

  NS_LOG_DEBUG ("\t idleTime=" << idleTime);
  //PrintTable ();

  shift = m_sTable [(idleTime >>  m_scellLog) & RED_STATS_MASK];

  if (shift)
    {
      //std::cout << "shift " << m_qavg << "=>" << (m_qavg >> shift) << std::endl;
      return m_qavg >> shift;
    }
  else
    {
      idleTime = (m_qavg * idleTime) >> m_scellLog;


      NS_LOG_DEBUG ("\t idleus=" << idleTime);

      if (idleTime < (m_qavg / 2))
        {
	  //std::cout <<"idleus " <<  m_qavg << " - " << idleus << " = " << (m_qavg-idleus) << std::endl;
          return m_qavg - idleTime;
        }
      else
        {
	  //std:: cout <<"half " << m_qavg << "=>" <<  (m_qavg/2) << std::endl;
          return (m_qavg / 2) ;
        }
    }
}

uint64_t
RedQueue::AvgFromNonIdleTime (uint32_t backlog)
{
  NS_LOG_FUNCTION (this << backlog);

  NS_LOG_DEBUG ("qavg " << m_qavg);
  NS_LOG_DEBUG ("backlog" << backlog);

 /**
  * This is basically EWMA
  * m_qavg = q_avg*(1-W) + backlog*W
  * m_qavg = q_avg + W(backlog - q_avg)
  *
  */
  return m_qavg + (backlog - (m_qavg >> m_wLog));
}

uint64_t
RedQueue::AvgCalc (uint32_t backlog)
{
  NS_LOG_FUNCTION (this << backlog);

  uint64_t qtemp;

  if ( !IsIdling ())
    {
      qtemp = AvgFromNonIdleTime (backlog);
      NS_LOG_DEBUG ("NonIdle Avg " << qtemp);
      //std::cout <<"n "<< qtemp << std::endl;
      return qtemp;
    }
  else
    {
      qtemp = AvgFromIdleTime ();
      NS_LOG_DEBUG ("Idle Avg" << qtemp);
      //std::cout <<"i "<< qtemp << std::endl;
      return qtemp;
    }
}

int
RedQueue::CheckThresh (uint64_t avg)
{

  NS_LOG_FUNCTION (this << avg);
  NS_LOG_DEBUG ("\t check threshold: min " << m_minTh << " max" << m_maxTh);

  if (avg < m_minTh)
    {
      return BELOW_MIN_THRESH;
    }
  else if (avg >= m_maxTh)
    {
      return ABOVE_MAX_THRESH;
    }
  else
    {
      return BETWEEN_THRESH;
    }
}
uint32_t
RedQueue::RedRandom ()
{
  NS_LOG_FUNCTION_NOARGS ();

  ///> obtain a random u32 number
  ///> return m_rmask & ran.GetInteger ();
  //checkme
  return m_rmask & rand ();
}
int
RedQueue::MarkProbability (uint64_t avg)
{
  NS_LOG_FUNCTION (this << avg);
  NS_LOG_DEBUG ("\t m_randNum " << m_randNum);
  NS_LOG_DEBUG ("\t right\t" << m_randNum);
  NS_LOG_DEBUG ("\t left\t" << ((avg - m_minTh)*m_count));

  ///> max_P* (qavg - qth_min)/(qth_max-qth_min) < rnd/qcount
  //return !((avg - m_minTh ) * m_count < m_randNum);
  //checkme
  return !((avg - m_minTh )* m_count < m_randNum);

}
int
RedQueue::Processing (uint64_t qavg)
{

  NS_LOG_FUNCTION (this << "qavg" << qavg << " m_minTh" << m_minTh << " m_maxTh" << m_maxTh);

  switch (CheckThresh (qavg))
    {
    case BELOW_MIN_THRESH:
      NS_LOG_DEBUG ("\t below threshold ");

      m_count = -1;
      return DONT_MARK;

    case BETWEEN_THRESH:
      NS_LOG_DEBUG ("\t between threshold ");

      if (++m_count)
        {
          NS_LOG_DEBUG ("\t check Mark Prob");
          if (MarkProbability (qavg))
            {
              m_count = 0;
              m_randNum = RedRandom ();

              NS_LOG_DEBUG ("\t Marked Will Drop " << m_qavg);

              return PROB_MARK;
            }
          NS_LOG_DEBUG ("\t Marked Will Save " << m_qavg);
        }
      else
        {
          m_randNum = RedRandom ();
        }
      return DONT_MARK;

    case ABOVE_MAX_THRESH:

      NS_LOG_DEBUG ("\t above threshold ");

      m_count = -1;
      return HARD_MARK;
    }

  NS_LOG_DEBUG ("BUG HERE\n");
  return DONT_MARK;
}


bool
RedQueue::DoEnqueue (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);

  if (m_mode == PACKETS && (m_packets.size () >= m_maxPackets))
    {
      NS_LOG_LOGIC ("Queue full (at max packets) -- droppping pkt");
      Drop (p);
      return false;
    }

  if (m_mode == BYTES && (m_bytesInQueue + p->GetSize () >= m_maxBytes))
    {
      NS_LOG_LOGIC ("Queue full (packet would exceed max bytes) -- droppping pkt");
      Drop (p);
      return false;
    }

  if (!m_initialized)
    {
      // making sure all the variables are initialized ok
      NS_LOG_DEBUG ("\t m_maxPackets" << m_maxPackets
                                      << " m_maxBytes" << m_maxBytes
                                      << " m_burst" << m_burst << " m_avPkt" << m_avPkt
                                      << " m_minTh" << m_minTh << " m_maxTh" << m_maxTh
                                      << " m_rate" << m_rate <<  " m_prob" << m_prob);

      m_wLog = evalEwma (m_minTh, m_burst, m_avPkt);
      m_pLog = evalP (m_minTh, m_maxTh, m_prob);
      m_scellLog = evalIdleDamping (m_wLog, m_avPkt, m_rate);

      SetParams (m_minTh, m_maxTh, m_wLog, m_pLog, m_scellLog);
      EndIdlePeriod ();
//      srand((unsigned)time(0));
      m_initialized = true;
    }

//  PrintTable();

  if (GetMode () == BYTES)
    {
      m_qavg = AvgCalc (m_bytesInQueue);
    }
  else if (GetMode () == PACKETS)
    {
      //not yet supported
//      m_qavg = AvgCalc (m_packets.size ());
    }

  NS_LOG_DEBUG ("\t bytesInQueue  " << m_bytesInQueue << "\tQavg " << m_qavg);
  NS_LOG_DEBUG ("\t packetsInQueue  " << m_packets.size () << "\tQavg " << m_qavg);


  if (IsIdling ())
    {
      EndIdlePeriod ();
    }

  switch (Processing (m_qavg) )
    {
    case DONT_MARK:
      break;

    case PROB_MARK:
      NS_LOG_DEBUG ("\t Dropping due to Prob Mark " << m_qavg);
      m_stats.probDrop++;
      m_stats.probMark++;
      Drop (p);
      return false;

    case HARD_MARK:
      NS_LOG_DEBUG ("\t Dropping due to Hard Mark " << m_qavg);
      m_stats.forcedMark++;
      m_stats.probDrop++;
      Drop (p);
      return false;
    }


  m_bytesInQueue += p->GetSize ();
  m_packets.push_back (p);

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return true;
}

Ptr<Packet>
RedQueue::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  if (m_packets.empty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  Ptr<Packet> p = m_packets.front ();
  m_packets.pop_front ();
  m_bytesInQueue -= p->GetSize ();

  NS_LOG_LOGIC ("Popped " << p);

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  if (m_bytesInQueue <= 0 && !IsIdling ())
    {
      StartIdlePeriod ();
    }

  return p;
}

///> just for completeness
/// m_packets.remove (p) also works
int
RedQueue::DropPacket (Ptr<Packet> p)
{

  NS_LOG_FUNCTION (this << p);

  NS_LOG_DEBUG ("\t Dropping Packet p");

  std::list<Ptr<Packet> >::iterator iter;
  uint32_t packetSize;

  for (iter = m_packets.begin(); iter != m_packets.end(); ++iter)
    {
      if (*iter == p)
        {
	  packetSize= p->GetSize ();
          m_packets.erase(iter);
          m_bytesInQueue -= packetSize; 
          return 1;
        }
    }

  if (!IsIdling ())
    {
      StartIdlePeriod ();
    }

  return 0;
}

Ptr<const Packet>
RedQueue::DoPeek (void) const
{
  NS_LOG_FUNCTION (this);

  if (m_packets.empty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return NULL;
    }

  Ptr<Packet> p = m_packets.front ();

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return p;
}

void
RedQueue::PrintTable ()
{
  NS_LOG_FUNCTION_NOARGS ();

  for (uint32_t i = 0; i < RED_STATS_TABLE_SIZE; i++)
    {
      std::cout << m_sTable[i] << " ";
    }
  std::cout << std::endl;
}


} // namespace ns3
