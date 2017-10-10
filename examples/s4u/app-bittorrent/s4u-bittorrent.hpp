/* Copyright (c) 2012-2014, 2016-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef BITTORRENT_BITTORRENT_HPP_
#define BITTORRENT_BITTORRENT_HPP_

#include <simgrid/s4u.hpp>
#include <xbt/RngStream.h>

#define MAILBOX_SIZE 40
#define TRACKER_MAILBOX "tracker_mailbox"
/** Max number of peers sent by the tracker to clients */
#define MAXIMUM_PEERS 50
/** Interval of time where the peer should send a request to the tracker */
#define TRACKER_QUERY_INTERVAL 1000
/** Communication size for a task to the tracker */
#define TRACKER_COMM_SIZE 1
#define GET_PEERS_TIMEOUT 10000
#define TIMEOUT_MESSAGE 10
#define TRACKER_RECEIVE_TIMEOUT 10
/** Number of peers that can be unchocked at a given time */
#define MAX_UNCHOKED_PEERS 4
/** Interval between each update of the choked peers */
#define UPDATE_CHOKED_INTERVAL 30
/** Number of pieces the peer asks for simultaneously */
#define MAX_PIECES 1

/** Message sizes
 * Sizes based on report by A. Legout et al, Understanding BitTorrent: An Experimental Perspective
 * http://hal.inria.fr/inria-00000156/en
 */
#define MESSAGE_HANDSHAKE_SIZE 68
#define MESSAGE_CHOKE_SIZE 5
#define MESSAGE_UNCHOKE_SIZE 5
#define MESSAGE_INTERESTED_SIZE 5
#define MESSAGE_NOTINTERESTED_SIZE 5
#define MESSAGE_HAVE_SIZE 9
#define MESSAGE_BITFIELD_SIZE 5
#define MESSAGE_REQUEST_SIZE 17
#define MESSAGE_PIECE_SIZE 13
#define MESSAGE_CANCEL_SIZE 17

/** Types of messages exchanged between two peers. */
enum e_message_type {
  MESSAGE_HANDSHAKE,
  MESSAGE_CHOKE,
  MESSAGE_UNCHOKE,
  MESSAGE_INTERESTED,
  MESSAGE_NOTINTERESTED,
  MESSAGE_HAVE,
  MESSAGE_BITFIELD,
  MESSAGE_REQUEST,
  MESSAGE_PIECE,
  MESSAGE_CANCEL
};

class Message {
public:
  e_message_type type;
  int peer_id;
  simgrid::s4u::MailboxPtr return_mailbox;
  unsigned int bitfield = 0U;
  int piece             = 0;
  int block_index       = 0;
  int block_length      = 0;
  Message(e_message_type type, int peer_id, simgrid::s4u::MailboxPtr return_mailbox)
      : type(type), peer_id(peer_id), return_mailbox(return_mailbox){};
  Message(e_message_type type, int peer_id, unsigned int bitfield, simgrid::s4u::MailboxPtr return_mailbox)
      : type(type), peer_id(peer_id), return_mailbox(return_mailbox), bitfield(bitfield){};
  Message(e_message_type type, int peer_id, simgrid::s4u::MailboxPtr return_mailbox, int piece, int block_index,
          int block_length)
      : type(type)
      , peer_id(peer_id)
      , return_mailbox(return_mailbox)
      , piece(piece)
      , block_index(block_index)
      , block_length(block_length){};
  Message(e_message_type type, int peer_id, simgrid::s4u::MailboxPtr return_mailbox, int piece)
      : type(type), peer_id(peer_id), return_mailbox(return_mailbox), piece(piece){};
  ~Message() = default;
};

class HostBittorrent {
  RngStream stream_;
  simgrid::s4u::Host* host = nullptr;

public:
  static simgrid::xbt::Extension<simgrid::s4u::Host, HostBittorrent> EXTENSION_ID;

  explicit HostBittorrent(simgrid::s4u::Host* ptr) : host(ptr)
  {
    std::string descr = std::string("RngSream<") + host->getCname() + ">";
    stream_           = RngStream_CreateStream(descr.c_str());
  }

  ~HostBittorrent() { RngStream_DeleteStream(&stream_); };

  RngStream getStream() { return stream_; };
};

#endif /* BITTORRENT_BITTORRENT_HPP_ */
