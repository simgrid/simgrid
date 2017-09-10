/* Copyright (c) 2012-2014, 2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef BITTORRENT_TRACKER_HPP_
#define BITTORRENT_TRACKER_HPP_

#include "s4u-bittorrent.hpp"
#include <set>

class TrackerQuery {
  int peer_id; // peer id
  simgrid::s4u::MailboxPtr return_mailbox;
public:
  explicit TrackerQuery(int peer_id, simgrid::s4u::MailboxPtr return_mailbox)
      : peer_id(peer_id), return_mailbox(return_mailbox){};
  ~TrackerQuery() = default;
  int getPeerId() { return peer_id; }
  simgrid::s4u::MailboxPtr getReturnMailbox() { return return_mailbox; }
};

class TrackerAnswer {
  // int interval; // how often the peer should contact the tracker (unused for now)
  std::set<int>* peers; // the peer list the peer has asked for.
public:
  explicit TrackerAnswer(int /*interval*/) /*: interval(interval)*/ { peers = new std::set<int>; }
  TrackerAnswer(const TrackerAnswer&)                               = delete;
  ~TrackerAnswer() { delete peers; };
  void addPeer(int peer) { peers->insert(peer); }
  std::set<int>* getPeers() { return peers; }
};

class Tracker {
  double deadline;
  RngStream stream;
  simgrid::s4u::MailboxPtr mailbox;
  std::set<int> known_peers;

public:
  explicit Tracker(std::vector<std::string> args);
  void operator()();
};

#endif /* BITTORRENT_TRACKER_HPP */
