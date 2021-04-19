/* Copyright (c) 2012-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef BITTORRENT_PEER_H
#define BITTORRENT_PEER_H
#include "app-bittorrent.h"

#include <simgrid/comm.h>
#include <simgrid/mailbox.h>
#include <xbt/dict.h>
#include <xbt/dynar.h>

/**  Contains the connection data of a peer. */
typedef struct s_connection {
  int id; // Peer id
  sg_mailbox_t mailbox;
  unsigned int bitfield; // Fields
  double peer_speed;
  double last_unchoke;
  int current_piece;
  unsigned int am_interested : 1;   // Indicates if we are interested in something the peer has
  unsigned int interested : 1;      // Indicates if the peer is interested in one of our pieces
  unsigned int choked_upload : 1;   // Indicates if the peer is choked for the current peer
  unsigned int choked_download : 1; // Indicates if the peer has choked the current peer
} s_connection_t;

typedef s_connection_t* connection_t;
typedef const s_connection_t* const_connection_t;

connection_t connection_new(int id);
void connection_add_speed_value(connection_t connection, double speed);
int connection_has_piece(const_connection_t connection, unsigned int piece);

/** Peer data */
typedef struct s_peer {
  int id; // peer id
  double deadline;
  sg_mailbox_t mailbox; // peer mailbox.

  xbt_dict_t connected_peers; // peers list
  xbt_dict_t active_peers;    // active peers list

  unsigned int bitfield;              // list of pieces the peer has.
  unsigned long long bitfield_blocks; // list of blocks the peer has.
  short* pieces_count;                // number of peers that have each piece.
  unsigned int current_pieces;        // current pieces the peer is downloading
  double begin_receive_time; // time when the receiving communication has begun, useful for calculating host speed.
  int round;                 // current round for the chocking algorithm.

  sg_comm_t comm_received; // current comm
  message_t message;       // current message being received
} s_peer_t;
typedef s_peer_t* peer_t;
typedef const s_peer_t* const_peer_t;

/** Peer main function */
void peer(int argc, char* argv[]);

int get_peers_from_tracker(const_peer_t peer);
void send_handshake_to_all_peers(const_peer_t peer);
void send_message(const_peer_t peer, sg_mailbox_t mailbox, e_message_type type, uint64_t size);
void send_bitfield(const_peer_t peer, sg_mailbox_t mailbox);
void send_piece(const_peer_t peer, sg_mailbox_t mailbox, int piece, int block_index, int block_length);
void send_have_to_all_peers(const_peer_t peer, int piece);
void send_request_to_peer(const_peer_t peer, connection_t remote_peer, int piece);

void get_status(char* status, unsigned int bitfield);
int has_finished(unsigned int bitfield);
int is_interested(const_peer_t peer, const_connection_t remote_peer);
int is_interested_and_free(const_peer_t peer, const_connection_t remote_peer);
void update_pieces_count_from_bitfield(const_peer_t peer, unsigned int bitfield);

int count_pieces(unsigned int bitfield);
int nb_interested_peers(const_peer_t peer);

void leech(peer_t peer);
void seed(peer_t peer);

void handle_message(peer_t peer, message_t task);

void update_choked_peers(peer_t peer);

void update_interested_after_receive(const_peer_t peer);

void update_bitfield_blocks(peer_t peer, int index, int block_index, int block_length);
int piece_complete(const_peer_t peer, int index);
int get_first_missing_block_from(const_peer_t peer, int piece);

int peer_has_not_piece(const_peer_t peer, unsigned int piece);
int peer_is_not_downloading_piece(const_peer_t peer, unsigned int piece);

int partially_downloaded_piece(const_peer_t peer, const_connection_t remote_peer);

void request_new_piece_to_peer(peer_t peer, connection_t remote_peer);
void remove_current_piece(peer_t peer, connection_t remote_peer, unsigned int current_piece);

void update_active_peers_set(const_peer_t peer, connection_t remote_peer);
int select_piece_to_download(const_peer_t peer, const_connection_t remote_peer);

#endif /* BITTORRENT_PEER_H */
