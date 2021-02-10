/* Copyright (c) 2012-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef BITTORRENT_BITTORRENT_HPP_
#define BITTORRENT_BITTORRENT_HPP_

#include <simgrid/s4u.hpp>
#include <xbt/random.hpp>

constexpr char TRACKER_MAILBOX[] = "tracker_mailbox";
/** Max number of peers sent by the tracker to clients */
constexpr int MAXIMUM_PEERS = 50;
/** Interval of time where the peer should send a request to the tracker */
constexpr int TRACKER_QUERY_INTERVAL = 1000;
/** Communication size for a task to the tracker */
constexpr unsigned TRACKER_COMM_SIZE = 1;
constexpr double GET_PEERS_TIMEOUT   = 10000.0;
/** Number of peers that can be unchocked at a given time */
constexpr int MAX_UNCHOKED_PEERS = 4;
/** Interval between each update of the choked peers */
constexpr int UPDATE_CHOKED_INTERVAL = 30;

/** Types of messages exchanged between two peers. */
enum class MessageType { HANDSHAKE, CHOKE, UNCHOKE, INTERESTED, NOTINTERESTED, HAVE, BITFIELD, REQUEST, PIECE, CANCEL };

class Message {
public:
  MessageType type;
  int peer_id;
  simgrid::s4u::Mailbox* return_mailbox;
  unsigned int bitfield = 0U;
  int piece             = 0;
  int block_index       = 0;
  int block_length      = 0;
  Message(MessageType type, int peer_id, simgrid::s4u::Mailbox* return_mailbox)
      : type(type), peer_id(peer_id), return_mailbox(return_mailbox){};
  Message(MessageType type, int peer_id, unsigned int bitfield, simgrid::s4u::Mailbox* return_mailbox)
      : type(type), peer_id(peer_id), return_mailbox(return_mailbox), bitfield(bitfield){};
  Message(MessageType type, int peer_id, simgrid::s4u::Mailbox* return_mailbox, int piece, int block_index,
          int block_length)
      : type(type)
      , peer_id(peer_id)
      , return_mailbox(return_mailbox)
      , piece(piece)
      , block_index(block_index)
      , block_length(block_length){};
  Message(MessageType type, int peer_id, simgrid::s4u::Mailbox* return_mailbox, int piece)
      : type(type), peer_id(peer_id), return_mailbox(return_mailbox), piece(piece){};
};

#endif /* BITTORRENT_BITTORRENT_HPP_ */
