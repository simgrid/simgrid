/* Copyright (c) 2012-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "s4u-tracker.hpp"
#include <algorithm>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_bt_tracker, "Messages specific for the tracker");

Tracker::Tracker(std::vector<std::string> args)
{
  // Checking arguments
  xbt_assert(args.size() == 2, "Wrong number of arguments for the tracker.");
  // Retrieving end time
  try {
    deadline = std::stod(args[1]);
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument("Invalid deadline:" + args[1]);
  }
  xbt_assert(deadline > 0, "Wrong deadline supplied");

  mailbox = simgrid::s4u::Mailbox::by_name(TRACKER_MAILBOX);

  XBT_INFO("Tracker launched.");
}

void Tracker::operator()()
{
  simgrid::s4u::CommPtr comm = nullptr;
  void* received             = nullptr;
  while (simgrid::s4u::Engine::get_clock() < deadline) {
    if (comm == nullptr)
      comm = mailbox->get_async(&received);
    if (comm->test()) {
      // Retrieve the data sent by the peer.
      xbt_assert(received != nullptr);
      TrackerQuery* tq = static_cast<TrackerQuery*>(received);

      // Add the peer to our peer list, if not already known.
      if (known_peers.find(tq->getPeerId()) == known_peers.end()) {
        known_peers.insert(tq->getPeerId());
      }

      // Sending back peers to the requesting peer
      TrackerAnswer* ta = new TrackerAnswer(TRACKER_QUERY_INTERVAL);
      std::set<int>::iterator next_peer;
      int nb_known_peers = static_cast<int>(known_peers.size());
      int max_tries      = std::min(MAXIMUM_PEERS, nb_known_peers);
      int tried          = 0;
      while (tried < max_tries) {
        do {
          next_peer = known_peers.begin();
          std::advance(next_peer, random.uniform_int(0, nb_known_peers - 1));
        } while (ta->getPeers().find(*next_peer) != ta->getPeers().end());
        ta->addPeer(*next_peer);
        tried++;
      }
      tq->getReturnMailbox()->put_init(ta, TRACKER_COMM_SIZE)->detach();

      delete tq;
      comm = nullptr;
    } else {
      simgrid::s4u::this_actor::sleep_for(1);
    }
  }
  XBT_INFO("Tracker is leaving");
}
