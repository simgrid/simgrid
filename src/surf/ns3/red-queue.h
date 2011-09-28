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
 * Random Early Detection (RED) algorithm.
 *
 * This implementation uses Bytes as queue size metric by default
 * based on the Kuznetsov's implementation of Red in Linux. 
 * Therefore, only bytes as queue size metric is supported at the moment.
 *
 * Original RED is from
 * Sally Floyd and Van Jacobson, "Random Early Detection Gateways for
 * Congestion Avoidance", 1993, IEEE/ACM Transactions on Networking
 *
 * Description:
 *
 * Packet arrival:
 * avg = (1-W)*avg + W*currentQueueLen
 * W is the queue weight( chosen as 2^(-Wlog)).  Decrease W for larger bursts
 *
 * if ( avg > maxTh) -> mark/drop packets
 * if ( avg < minTh) -> allow packets
 * if ( minTh < avg < maxTh) -> calculate probability for marking/dropping
 *
 *
 * Pb = maxP*(avg - minTh)/(maxTh - minTh)
 * Note: As avg varies from minTh to maxTh, Pb varies 0 to maxP
 *
 * Pa = Pb /(1 - count*Pb)
 * The final marking probability Pa increases slowly as the count increases
 * since the last marked packet
 *
 * maxP can be chosen s/t:
 * maxP = (maxTh - minTh )/2^Plog
 *
 * To allow large bursts of L packets of size S, W can be chosen as:
 * (see paper for details)
 *
 * L + 1 - minTh/S < (1-(1-W)^L)/W
 *
 *
 * Some parameters:
 * W = 0.002
 * maxP = .02
 *
 *
 */

#ifndef RED_QUEUE_H
#define RED_QUEUE_H

#include <queue>
#include "ns3/packet.h"
#include "ns3/queue.h"
#include "ns3/nstime.h"

namespace ns3 {

class TraceContainer;

/**
 * \ingroup queue
 *
 * \brief A RED packet queue
 */
class RedQueue : public Queue
{
public:
  typedef std::vector< uint32_t> Uint32tVector;

  struct Stats
  {
    uint32_t probDrop;  ///< Early probability drops
    uint32_t probMark;  ///< Early probability marks
    uint32_t forcedDrop;  ///< Forced drops, qavg > max threshold
    uint32_t forcedMark;  ///< Forced marks, qavg > max threshold
    uint32_t pdrop;  ///< Drops due to queue limits
    uint32_t other;  ///< Drops due to drop calls
    uint32_t backlog;
  };

  static TypeId GetTypeId (void);
  /**
   * \brief RedQueue Constructor
   *
   */
  RedQueue ();

  virtual ~RedQueue ();

  /**
   * Enumeration of the modes supported in the class.
   *
   */
  enum Mode
  {
    ILLEGAL,     /**< Mode not set */
    PACKETS,     /**< Use number of packets for maximum queue size */
    BYTES,       /**< Use number of bytes for maximum queue size */
  };

  /**
   * Enumeration of RED thresholds
   *
   */
  enum
  {
    BELOW_MIN_THRESH,
    ABOVE_MAX_THRESH,
    BETWEEN_THRESH,
  };

  /**
   * Enumeration Marking
   *
   */
  enum
  {
    DONT_MARK,
    PROB_MARK,
    HARD_MARK,
  };

  /**
   * Set the operating mode of this device.
   *
   * \param mode The operating mode of this device.
   *
   */
  void SetMode (RedQueue::Mode mode);

  /**
   * Get the encapsulation mode of this device.
   *
   * \returns The encapsulation mode of this device.
   */
  RedQueue::Mode  GetMode (void);

 /**
   * Get the average queue size
   *
   * \returns The average queue size in bytes
   */
  uint64_t GetAverageQueueSize (void);


private:
  void SetParams (uint32_t minTh, uint32_t maxTh,
                  uint32_t wLog, uint32_t pLog, uint64_t scellLog );
  void StartIdlePeriod ();
  void EndIdlePeriod ();
  void Restart ();
  void printRedOpt ();
  void printStats ();
  void PrintTable ();

  int IsIdling ();
  int CheckThresh (uint64_t avg);
  int Processing (uint64_t avg);
  int MarkProbability (uint64_t avg);
  int DropPacket (Ptr<Packet> p);

  uint64_t AvgFromIdleTime ();
  uint64_t AvgCalc (uint32_t backlog);

  ///< Avg = Avg*(1-W) + backlog*W
  uint64_t AvgFromNonIdleTime (uint32_t backlog);

  ///< burst + 1 - qmin/avpkt < (1-(1-W)^burst)/W
  ///< this low-pass filter is used to calculate the avg queue size
  uint32_t evalEwma (uint32_t minTh, uint32_t burst, uint32_t avpkt);

  ///< Plog = log (prob / (qMax - qMin))
  uint32_t evalP (uint32_t minTh, uint32_t maxTh, double prob);

  ///< -log(1-W) * t/TxTime
  uint32_t evalIdleDamping (uint32_t wLog, uint32_t avpkt, uint32_t rate);

  uint32_t Rmask (uint32_t pLog);
  uint32_t RedRandom ();

  virtual bool DoEnqueue (Ptr<Packet> p);
  virtual Ptr<Packet> DoDequeue (void);
  virtual Ptr<const Packet> DoPeek (void) const;


  std::list<Ptr<Packet> > m_packets;

  Mode     m_mode;

  uint32_t m_bytesInQueue;

  ///< users' configurable options
  uint32_t m_maxPackets;
  uint32_t m_maxBytes;
  uint32_t m_burst;

  /**
   * in bytes, use with burst to determine the time constant
   * for average queue size calculations, for ewma
   */
  uint32_t m_avPkt;
  uint32_t m_minTh; ///> Min avg length threshold (bytes), should be < maxTh/2
  uint32_t m_maxTh; ///> Max avg length threshold (bytes), should be >= 2*minTh
  uint32_t m_rate; ///> bandwidth in bps
  uint32_t m_wLog; ///> log(W) bits
  uint32_t m_pLog; ///> random number bits log (P_max/(maxTh - minTh))
  uint32_t m_rmask;
  uint32_t m_scellLog; ///> cell size for idle damping
  uint64_t m_scellMax;
  uint32_t m_count;  ///> number of packets since last random number generation
  uint32_t m_randNum; ///> a random number 0 ...2^Plog

  uint64_t m_qavg; ///> average q length
  double m_prob;
  bool m_initialized;

  Stats m_stats;
  Time m_idleStart; ///> start of current idle period
  Uint32tVector m_sTable;
};

}; // namespace ns3

#endif /* RED_QUEUE_H */
