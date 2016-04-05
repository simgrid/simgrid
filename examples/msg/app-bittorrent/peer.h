/* Copyright (c) 2012-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef BITTORRENT_PEER_H
#define BITTORRENT_PEER_H
#include <simgrid/msg.h>
#include <xbt/dict.h>
#include <xbt/dynar.h>
#include <xbt/RngStream.h>
#include "connection.h"
#include "bittorrent.h"

/** Peer data */
typedef struct s_peer {
  int id;                       //peer id

  int pieces;                   //number of pieces the peer has.
  char *bitfield;               //list of pieces the peer has.
  char *bitfield_blocks;        //list of blocks the peer has.
  short *pieces_count;          //number of peers that have each piece.

  xbt_dynar_t current_pieces;   //current pieces the peer is downloading

  xbt_dict_t peers;             //peers list
  xbt_dict_t active_peers;      //active peers list
  int round;                    //current round for the chocking algorithm.


  char mailbox[MAILBOX_SIZE];   //peer mailbox.
  char mailbox_tracker[MAILBOX_SIZE];   //pair mailbox while communicating with the tracker.
  const char *hostname;         //peer hostname

  msg_task_t task_received;     //current task being received
  msg_comm_t comm_received;     //current comm


  RngStream stream;             //RngStream for

  double begin_receive_time;    //time when the receiving communication has begun, useful for calculating host speed.

} s_peer_t, *peer_t;

/** Peer main function */
int peer(int argc, char *argv[]);

int get_peers_data(peer_t peer);
void leech_loop(peer_t peer, double deadline);
void seed_loop(peer_t peer, double deadline);

void peer_init(peer_t, int id, int seed);
void peer_free(peer_t peer);

int has_finished(char *bitfield);

void handle_message(peer_t peer, msg_task_t task);

void update_pieces_count_from_bitfield(peer_t peer, char *bitfield);
void update_choked_peers(peer_t peer);

void update_interested_after_receive(peer_t peer);

void update_bitfield_blocks(peer_t peer, int index, int block_index, int block_length);
int piece_complete(peer_t peer, int index);
int get_first_block(peer_t peer, int piece);


int nb_interested_peers(peer_t peer);
int is_interested(peer_t peer, connection_t remote_peer);
int is_interested_and_free(peer_t peer, connection_t remote_peer);
int in_current_pieces(peer_t peer, int piece);
int partially_downloaded_piece(peer_t peer, connection_t remote_peer);

void request_new_piece_to_peer(peer_t peer, connection_t remote_peer);
void send_request_to_peer(peer_t peer, connection_t remote_peer, int piece);
void remove_current_piece(peer_t peer, connection_t remote_peer, int current_piece);

void update_active_peers_set(peer_t peer, connection_t remote_peer);
int select_piece_to_download(peer_t peer, connection_t remote_peer);

void send_handshake_all(peer_t peer);

void send_interested(peer_t peer, const char *mailbox);

void send_notinterested(peer_t peer, const char *mailbox);
void send_handshake(peer_t peer, const char *mailbox);
void send_bitfield(peer_t peer, const char *mailbox);
void send_choked(peer_t peer, const char *mailbox);
void send_unchoked(peer_t peer, const char *mailbox);
void send_have(peer_t peer, int piece);

void send_request(peer_t peer, const char *mailbox, int piece, int block_index, int block_length);
void send_piece(peer_t peer, const char *mailbox, int piece, int block_index, int block_length);

#endif                          /* BITTORRENT_PEER_H */
