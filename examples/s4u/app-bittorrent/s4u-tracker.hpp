/* Copyright (c) 2012-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef BITTORRENT_TRACKER_HPP
#define BITTORRENT_TRACKER_HPP

#include "s4u-bittorrent.hpp"
#include <set>

class TrackerQuery {
  int peer_id; // peer id
  simgrid::s4u::Mailbox* return_mailbox;

public:
  explicit TrackerQuery(int peer_id, simgrid::s4u::Mailbox* return_mailbox)
      : peer_id(peer_id), return_mailbox(return_mailbox){};
  int getPeerId() const { return peer_id; }
  simgrid::s4u::Mailbox* getReturnMailbox() const { return return_mailbox; }
};

class TrackerAnswer {
  // int interval; // how often the peer should contact the tracker (unused for now)
  std::set<int> peers; // the peer list the peer has asked for.
public:
  explicit TrackerAnswer(int /*interval*/) /*: interval(interval)*/ {}
  void addPeer(int peer) { peers.insert(peer); }
  const std::set<int>& getPeers() const { return peers; }
};

class Tracker {
  double deadline;
  simgrid::xbt::random::XbtRandom random{42};
  simgrid::s4u::Mailbox* mailbox;
  std::set<int> known_peers;

public:
  explicit Tracker(std::vector<std::string> args);
  void operator()();
};

#endif /* BITTORRENT_TRACKER_HPP */
