/* Copyright (c) 2012-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "bittorrent-peer.h"
#include "tracker.h"
#include <simgrid/forward.h>

#include <limits.h>
#include <stdio.h> /* snprintf */

XBT_LOG_NEW_DEFAULT_CATEGORY(bittorrent_peers, "Messages specific for the peers");

/*
 * User parameters for transferred file data. For the test, the default values are :
 * File size: 10 pieces * 5 blocks/piece * 16384 bytes/block = 819200 bytes
 */
#define FILE_PIECES 10UL
#define PIECES_BLOCKS 5UL
#define BLOCK_SIZE 16384

/** Number of blocks asked by each request */
#define BLOCKS_REQUESTED 2UL

#define SLEEP_DURATION 1
#define BITS_TO_BYTES(x) (((x) / 8 + (x) % 8) ? 1 : 0)

const char* const message_type_names[10] = {"HANDSHAKE", "CHOKE",    "UNCHOKE", "INTERESTED", "NOTINTERESTED",
                                            "HAVE",      "BITFIELD", "REQUEST", "PIECE",      "CANCEL"};

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

static peer_t peer_init(int id, int seed)
{
  peer_t peer = xbt_new(s_peer_t, 1);
  peer->id    = id;

  char mailbox_name[MAILBOX_SIZE];
  snprintf(mailbox_name, MAILBOX_SIZE - 1, "%d", id);
  peer->mailbox = sg_mailbox_by_name(mailbox_name);

  peer->connected_peers = xbt_dict_new_homogeneous(NULL);
  peer->active_peers    = xbt_dict_new_homogeneous(NULL);

  if (seed) {
    peer->bitfield        = (1U << FILE_PIECES) - 1U;
    peer->bitfield_blocks = (1ULL << (FILE_PIECES * PIECES_BLOCKS)) - 1ULL;
  } else {
    peer->bitfield        = 0;
    peer->bitfield_blocks = 0;
  }

  peer->current_pieces = 0;
  peer->pieces_count   = xbt_new0(short, FILE_PIECES);
  peer->comm_received  = NULL;
  peer->round          = 0;

  return peer;
}

static void peer_free(peer_t peer)
{
  char* key;
  connection_t connection;
  xbt_dict_cursor_t cursor;
  xbt_dict_foreach (peer->connected_peers, cursor, key, connection)
    xbt_free(connection);

  xbt_dict_free(&peer->connected_peers);
  xbt_dict_free(&peer->active_peers);
  xbt_free(peer->pieces_count);
  xbt_free(peer);
}

/** Peer main function */
void peer(int argc, char* argv[])
{
  // Check arguments
  xbt_assert(argc == 3 || argc == 4, "Wrong number of arguments");

  // Build peer object
  peer_t peer = peer_init((int)xbt_str_parse_int(argv[1], "Invalid ID: %s"), argc == 4 ? 1 : 0);

  // Retrieve deadline
  peer->deadline = xbt_str_parse_double(argv[2], "Invalid deadline: %s");
  xbt_assert(peer->deadline > 0, "Wrong deadline supplied");

  char* status = xbt_malloc0(FILE_PIECES + 1);
  get_status(&status, peer->bitfield);

  XBT_INFO("Hi, I'm joining the network with id %d", peer->id);

  // Getting peer data from the tracker.
  if (get_peers_from_tracker(peer)) {
    XBT_DEBUG("Got %d peers from the tracker. Current status is: %s", xbt_dict_length(peer->connected_peers), status);
    peer->begin_receive_time = simgrid_get_clock();
    sg_mailbox_set_receiver(sg_mailbox_get_name(peer->mailbox));

    if (has_finished(peer->bitfield)) {
      send_handshake_to_all_peers(peer);
    } else {
      leech(peer);
    }
    seed(peer);
  } else {
    XBT_INFO("Couldn't contact the tracker.");
  }

  get_status(&status, peer->bitfield);
  XBT_INFO("Here is my current status: %s", status);
  if (peer->comm_received) {
    sg_comm_unref(peer->comm_received);
  }

  xbt_free(status);
  peer_free(peer);
}

/** @brief Retrieves the peer list from the tracker */
int get_peers_from_tracker(const_peer_t peer)
{
  sg_mailbox_t tracker_mailbox = sg_mailbox_by_name(TRACKER_MAILBOX);

  // Build the task to send to the tracker
  tracker_query_t peer_request = tracker_query_new(peer->id, peer->mailbox);

  XBT_DEBUG("Sending a peer request to the tracker.");
  sg_comm_t request = sg_mailbox_put_async(tracker_mailbox, peer_request, TRACKER_COMM_SIZE);
  sg_error_t res    = sg_comm_wait_for(request, GET_PEERS_TIMEOUT);

  if (res == SG_ERROR_TIMEOUT) {
    XBT_DEBUG("Timeout expired when requesting peers to tracker");
    xbt_free(peer_request);
    return 0;
  }

  void* message           = NULL;
  sg_comm_t comm_received = sg_mailbox_get_async(peer->mailbox, &message);
  res                     = sg_comm_wait_for(comm_received, GET_PEERS_TIMEOUT);
  if (res == SG_OK) {
    const_tracker_answer_t ta = (const_tracker_answer_t)message;
    // Add the peers the tracker gave us to our peer list.
    unsigned i;
    int peer_id;
    // Add the peers the tracker gave us to our peer list.
    xbt_dynar_foreach (ta->peers, i, peer_id) {
      if (peer_id != peer->id)
        xbt_dict_set_ext(peer->connected_peers, (char*)&peer_id, sizeof(int), connection_new(peer_id));
    }
    tracker_answer_free(message);
  } else if (res == SG_ERROR_TIMEOUT) {
    XBT_DEBUG("Timeout expired when requesting peers to tracker");
    tracker_answer_free(message);
    return 0;
  }

  return 1;
}

/** @brief Send a handshake message to all the peers the peer has. */
void send_handshake_to_all_peers(const_peer_t peer)
{
  connection_t remote_peer;
  xbt_dict_cursor_t cursor = NULL;
  char* key;
  xbt_dict_foreach (peer->connected_peers, cursor, key, remote_peer) {
    message_t handshake = message_new(MESSAGE_HANDSHAKE, peer->id, peer->mailbox);
    sg_comm_t comm      = sg_mailbox_put_init(remote_peer->mailbox, handshake, MESSAGE_HANDSHAKE_SIZE);
    sg_comm_detach(comm, NULL);
    XBT_DEBUG("Sending a HANDSHAKE to %s", sg_mailbox_get_name(remote_peer->mailbox));
  }
}

void send_message(const_peer_t peer, sg_mailbox_t mailbox, e_message_type type, uint64_t size)
{
  XBT_DEBUG("Sending %s to %s", message_type_names[type], sg_mailbox_get_name(mailbox));
  message_t message = message_other_new(type, peer->id, peer->mailbox, peer->bitfield);
  sg_comm_t comm    = sg_mailbox_put_init(mailbox, message, size);
  sg_comm_detach(comm, NULL);
}

/** @brief Send a bitfield message to all the peers the peer has */
void send_bitfield(const_peer_t peer, sg_mailbox_t mailbox)
{
  XBT_DEBUG("Sending a BITFIELD to %s", sg_mailbox_get_name(mailbox));
  message_t message = message_other_new(MESSAGE_BITFIELD, peer->id, peer->mailbox, peer->bitfield);
  sg_comm_t comm    = sg_mailbox_put_init(mailbox, message, MESSAGE_BITFIELD_SIZE + BITS_TO_BYTES(FILE_PIECES));
  sg_comm_detach(comm, NULL);
}

/** Send a "piece" message to a pair, containing a piece of the file */
void send_piece(const_peer_t peer, sg_mailbox_t mailbox, int piece, int block_index, int block_length)
{
  XBT_DEBUG("Sending the PIECE %d (%d,%d) to %s", piece, block_index, block_length, sg_mailbox_get_name(mailbox));
  xbt_assert(piece >= 0, "Tried to send a piece that doesn't exist.");
  xbt_assert(!peer_has_not_piece(peer, piece), "Tried to send a piece that we doesn't have.");
  message_t message = message_piece_new(peer->id, peer->mailbox, piece, block_index, block_length);
  sg_comm_t comm    = sg_mailbox_put_init(mailbox, message, BLOCK_SIZE);
  sg_comm_detach(comm, NULL);
}

/** Send a "HAVE" message to all peers we are connected to */
void send_have_to_all_peers(const_peer_t peer, int piece)
{
  XBT_DEBUG("Sending HAVE message to all my peers");
  connection_t remote_peer;
  xbt_dict_cursor_t cursor = NULL;
  char* key;
  xbt_dict_foreach (peer->connected_peers, cursor, key, remote_peer) {
    message_t message = message_index_new(MESSAGE_HAVE, peer->id, peer->mailbox, piece);
    sg_comm_t comm    = sg_mailbox_put_init(remote_peer->mailbox, message, MESSAGE_HAVE_SIZE);
    sg_comm_detach(comm, NULL);
  }
}

/** @brief Send request messages to a peer that have unchoked us */
void send_request_to_peer(const_peer_t peer, connection_t remote_peer, int piece)
{
  remote_peer->current_piece = piece;
  xbt_assert(connection_has_piece(remote_peer, piece));
  int block_index = get_first_missing_block_from(peer, piece);
  if (block_index != -1) {
    int block_length = MIN(BLOCKS_REQUESTED, PIECES_BLOCKS - block_index);
    XBT_DEBUG("Sending a REQUEST to %s for piece %d (%d,%d)", sg_mailbox_get_name(remote_peer->mailbox), piece,
              block_index, block_length);
    message_t message = message_request_new(peer->id, peer->mailbox, piece, block_index, block_length);
    sg_comm_t comm    = sg_mailbox_put_init(remote_peer->mailbox, message, MESSAGE_REQUEST_SIZE);
    sg_comm_detach(comm, NULL);
  }
}

void get_status(char** status, unsigned int bitfield)
{
  for (int i = FILE_PIECES - 1; i >= 0; i--)
    (*status)[i] = (bitfield & (1U << i)) ? '1' : '0';
  (*status)[FILE_PIECES] = '\0';
}

int has_finished(unsigned int bitfield)
{
  return bitfield == (1U << FILE_PIECES) - 1U;
}

/** Indicates if the remote peer has a piece not stored by the local peer */
int is_interested(const_peer_t peer, const_connection_t remote_peer)
{
  return remote_peer->bitfield & (peer->bitfield ^ ((1 << FILE_PIECES) - 1));
}

/** Indicates if the remote peer has a piece not stored by the local peer nor requested by the local peer */
int is_interested_and_free(const_peer_t peer, const_connection_t remote_peer)
{
  for (int i = 0; i < FILE_PIECES; i++)
    if (peer_has_not_piece(peer, i) && connection_has_piece(remote_peer, i) && peer_is_not_downloading_piece(peer, i))
      return 1;
  return 0;
}

/** @brief Updates the list of who has a piece from a bitfield */
void update_pieces_count_from_bitfield(const_peer_t peer, unsigned int bitfield)
{
  for (int i = 0; i < FILE_PIECES; i++)
    if (bitfield & (1U << i))
      peer->pieces_count[i]++;
}

int count_pieces(unsigned int bitfield)
{
  int count      = 0;
  unsigned int n = bitfield;
  while (n) {
    count += n & 1U;
    n >>= 1U;
  }
  return count;
}

int nb_interested_peers(const_peer_t peer)
{
  xbt_dict_cursor_t cursor = NULL;
  char* key;
  connection_t connection;
  int nb = 0;
  xbt_dict_foreach (peer->connected_peers, cursor, key, connection)
    if (connection->interested)
      nb++;

  return nb;
}

/** @brief Peer main loop when it is leeching. */
void leech(peer_t peer)
{
  double next_choked_update = simgrid_get_clock() + UPDATE_CHOKED_INTERVAL;
  XBT_DEBUG("Start downloading.");

  /* Send a "handshake" message to all the peers it got (since it couldn't have gotten more than 50 peers) */
  send_handshake_to_all_peers(peer);
  XBT_DEBUG("Starting main leech loop");

  void* data = NULL;
  while (simgrid_get_clock() < peer->deadline && count_pieces(peer->bitfield) < FILE_PIECES) {
    if (peer->comm_received == NULL)
      peer->comm_received = sg_mailbox_get_async(peer->mailbox, &data);

    if (sg_comm_test(peer->comm_received)) {
      peer->message = (message_t)data;
      handle_message(peer, peer->message);
      xbt_free(peer->message);
      peer->comm_received = NULL;
    } else {
      // We don't execute the choke algorithm if we don't already have a piece
      if (simgrid_get_clock() >= next_choked_update && count_pieces(peer->bitfield) > 0) {
        update_choked_peers(peer);
        next_choked_update += UPDATE_CHOKED_INTERVAL;
      } else {
        sg_actor_sleep_for(SLEEP_DURATION);
      }
    }
  }
  if (has_finished(peer->bitfield))
    XBT_DEBUG("%d becomes a seeder", peer->id);
}

/** @brief Peer main loop when it is seeding */
void seed(peer_t peer)
{
  double next_choked_update = simgrid_get_clock() + UPDATE_CHOKED_INTERVAL;
  XBT_DEBUG("Start seeding.");
  // start the main seed loop
  void* data = NULL;
  while (simgrid_get_clock() < peer->deadline) {
    if (peer->comm_received == NULL)
      peer->comm_received = sg_mailbox_get_async(peer->mailbox, &data);

    if (sg_comm_test(peer->comm_received)) {
      peer->message = (message_t)data;
      handle_message(peer, peer->message);
      xbt_free(peer->message);
      peer->comm_received = NULL;
    } else {
      if (simgrid_get_clock() >= next_choked_update) {
        update_choked_peers(peer);
        // TODO: Change the choked peer algorithm when seeding.
        next_choked_update += UPDATE_CHOKED_INTERVAL;
      } else {
        sg_actor_sleep_for(SLEEP_DURATION);
      }
    }
  }
}

void update_active_peers_set(const s_peer_t* peer, connection_t remote_peer)
{
  if ((remote_peer->interested != 0) && (remote_peer->choked_upload == 0)) {
    // add in the active peers set
    xbt_dict_set_ext(peer->active_peers, (char*)&remote_peer->id, sizeof(int), remote_peer);
  } else if (xbt_dict_get_or_null_ext(peer->active_peers, (char*)&remote_peer->id, sizeof(int))) {
    xbt_dict_remove_ext(peer->active_peers, (char*)&remote_peer->id, sizeof(int));
  }
}

/** @brief Handle a received message sent by another peer */
void handle_message(peer_t peer, message_t message)
{
  XBT_DEBUG("Received a %s message from %s", message_type_names[message->type],
            sg_mailbox_get_name(message->return_mailbox));

  connection_t remote_peer = xbt_dict_get_or_null_ext(peer->connected_peers, (char*)&message->peer_id, sizeof(int));
  xbt_assert(remote_peer != NULL || message->type == MESSAGE_HANDSHAKE,
             "The impossible did happened: A not-in-our-list peer sent us a message.");

  switch (message->type) {
    case MESSAGE_HANDSHAKE:
      // Check if the peer is in our connection list.
      if (remote_peer == 0) {
        xbt_dict_set_ext(peer->connected_peers, (char*)&message->peer_id, sizeof(int),
                         connection_new(message->peer_id));
        send_message(peer, message->return_mailbox, MESSAGE_HANDSHAKE, MESSAGE_HANDSHAKE_SIZE);
      }
      // Send our bitfield to the peer
      send_bitfield(peer, message->return_mailbox);
      break;
    case MESSAGE_BITFIELD:
      // Update the pieces list
      update_pieces_count_from_bitfield(peer, message->bitfield);
      // Store the bitfield
      remote_peer->bitfield = message->bitfield;
      xbt_assert(!remote_peer->am_interested, "Should not be interested at first");
      if (is_interested(peer, remote_peer)) {
        remote_peer->am_interested = 1;
        send_message(peer, message->return_mailbox, MESSAGE_INTERESTED, MESSAGE_INTERESTED_SIZE);
      }
      break;
    case MESSAGE_INTERESTED:
      xbt_assert((remote_peer != NULL), "A non-in-our-list peer has sent us a message. WTH ?");
      // Update the interested state of the peer.
      remote_peer->interested = 1;
      update_active_peers_set(peer, remote_peer);
      break;
    case MESSAGE_NOTINTERESTED:
      xbt_assert((remote_peer != NULL), "A non-in-our-list peer has sent us a message. WTH ?");
      remote_peer->interested = 0;
      update_active_peers_set(peer, remote_peer);
      break;
    case MESSAGE_UNCHOKE:
      xbt_assert((remote_peer != NULL), "A non-in-our-list peer has sent us a message. WTH ?");
      xbt_assert(remote_peer->choked_download);
      remote_peer->choked_download = 0;
      // Send requests to the peer, since it has unchoked us
      if (remote_peer->am_interested)
        request_new_piece_to_peer(peer, remote_peer);
      break;
    case MESSAGE_CHOKE:
      xbt_assert((remote_peer != NULL), "A non-in-our-list peer has sent us a message. WTH ?");
      xbt_assert(!remote_peer->choked_download);
      remote_peer->choked_download = 1;
      if (remote_peer->current_piece != -1)
        remove_current_piece(peer, remote_peer, remote_peer->current_piece);
      break;
    case MESSAGE_HAVE:
      XBT_DEBUG("\t for piece %d", message->piece);
      xbt_assert((message->piece >= 0 && message->piece < FILE_PIECES), "Wrong HAVE message received");
      remote_peer->bitfield = remote_peer->bitfield | (1U << message->piece);
      peer->pieces_count[message->piece]++;
      // If the piece is in our pieces, we tell the peer that we are interested.
      if ((remote_peer->am_interested == 0) && peer_has_not_piece(peer, message->piece)) {
        remote_peer->am_interested = 1;
        send_message(peer, message->return_mailbox, MESSAGE_INTERESTED, MESSAGE_INTERESTED_SIZE);
        if (remote_peer->choked_download == 0)
          request_new_piece_to_peer(peer, remote_peer);
      }
      break;
    case MESSAGE_REQUEST:
      xbt_assert(remote_peer->interested);
      xbt_assert((message->piece >= 0 && message->piece < FILE_PIECES), "Wrong request received");
      if (remote_peer->choked_upload == 0) {
        XBT_DEBUG("\t for piece %d (%d,%d)", message->piece, message->block_index,
                  message->block_index + message->block_length);
        if (!peer_has_not_piece(peer, message->piece)) {
          send_piece(peer, message->return_mailbox, message->piece, message->block_index, message->block_length);
        }
      } else {
        XBT_DEBUG("\t for piece %d but he is choked.", message->peer_id);
      }
      break;
    case MESSAGE_PIECE:
      XBT_DEBUG(" \t for piece %d (%d,%d)", message->piece, message->block_index,
                message->block_index + message->block_length);
      xbt_assert(!remote_peer->choked_download);
      xbt_assert(remote_peer->choked_download != 1, "Can't received a piece if I'm choked !");
      xbt_assert((message->piece >= 0 && message->piece < FILE_PIECES), "Wrong piece received");
      // TODO: Execute à computation.
      if (peer_has_not_piece(peer, message->piece)) {
        update_bitfield_blocks(peer, message->piece, message->block_index, message->block_length);
        if (piece_complete(peer, message->piece)) {
          // Removing the piece from our piece list
          remove_current_piece(peer, remote_peer, message->piece);
          // Setting the fact that we have the piece
          peer->bitfield = peer->bitfield | (1U << message->piece);
          char* status   = xbt_malloc0(FILE_PIECES + 1);
          get_status(&status, peer->bitfield);
          XBT_DEBUG("My status is now %s", status);
          xbt_free(status);
          // Sending the information to all the peers we are connected to
          send_have_to_all_peers(peer, message->piece);
          // sending UNINTERESTED to peers that do not have what we want.
          update_interested_after_receive(peer);
        } else {                                                   // piece not completed
          send_request_to_peer(peer, remote_peer, message->piece); // ask for the next block
        }
      } else {
        XBT_DEBUG("However, we already have it");
        request_new_piece_to_peer(peer, remote_peer);
      }
      break;
    case MESSAGE_CANCEL:
      break;
    default:
      THROW_IMPOSSIBLE;
  }
  // Update the peer speed.
  if (remote_peer) {
    connection_add_speed_value(remote_peer, 1.0 / (simgrid_get_clock() - peer->begin_receive_time));
  }
  peer->begin_receive_time = simgrid_get_clock();
}

/** Selects the appropriate piece to download and requests it to the remote_peer */
void request_new_piece_to_peer(peer_t peer, connection_t remote_peer)
{
  int piece = select_piece_to_download(peer, remote_peer);
  if (piece != -1) {
    peer->current_pieces |= (1U << (unsigned int)piece);
    send_request_to_peer(peer, remote_peer, piece);
  }
}

/** remove current_piece from the list of currently downloaded pieces. */
void remove_current_piece(peer_t peer, connection_t remote_peer, unsigned int current_piece)
{
  peer->current_pieces &= ~(1U << current_piece);
  remote_peer->current_piece = -1;
}

/** @brief Return the piece to be downloaded
 * There are two cases (as described in "Bittorrent Architecture Protocol", Ryan Toole :
 * If a piece is partially downloaded, this piece will be selected prioritarily
 * If the peer has strictly less than 4 pieces, he chooses a piece at random.
 * If the peer has more than pieces, he downloads the pieces that are the less replicated (rarest policy).
 * If all pieces have been downloaded or requested, we select a random requested piece (endgame mode).
 * @param peer: local peer
 * @param remote_peer: information about the connection
 * @return the piece to download if possible. -1 otherwise
 */
int select_piece_to_download(const_peer_t peer, const_connection_t remote_peer)
{
  int piece = partially_downloaded_piece(peer, remote_peer);
  // strict priority policy
  if (piece != -1)
    return piece;

  // end game mode
  if (count_pieces(peer->current_pieces) >= (FILE_PIECES - count_pieces(peer->bitfield)) &&
      (is_interested(peer, remote_peer) != 0)) {
    int nb_interesting_pieces = 0;
    // compute the number of interesting pieces
    for (int i = 0; i < FILE_PIECES; i++) {
      if (peer_has_not_piece(peer, i) && connection_has_piece(remote_peer, i)) {
        nb_interesting_pieces++;
      }
    }
    xbt_assert(nb_interesting_pieces != 0);
    // get a random interesting piece
    int random_piece_index = rand() % nb_interesting_pieces;
    int current_index      = 0;
    for (int i = 0; i < FILE_PIECES; i++) {
      if (peer_has_not_piece(peer, i) && connection_has_piece(remote_peer, i)) {
        if (random_piece_index == current_index) {
          piece = i;
          break;
        }
        current_index++;
      }
    }
    xbt_assert(piece != -1);
    return piece;
  }
  // Random first policy
  if (count_pieces(peer->bitfield) < 4 && (is_interested_and_free(peer, remote_peer) != 0)) {
    int nb_interesting_pieces = 0;
    // compute the number of interesting pieces
    for (int i = 0; i < FILE_PIECES; i++) {
      if (peer_has_not_piece(peer, i) && connection_has_piece(remote_peer, i) &&
          peer_is_not_downloading_piece(peer, i)) {
        nb_interesting_pieces++;
      }
    }
    xbt_assert(nb_interesting_pieces != 0);
    // get a random interesting piece
    int random_piece_index = rand() % nb_interesting_pieces;
    int current_index      = 0;
    for (int i = 0; i < FILE_PIECES; i++) {
      if (peer_has_not_piece(peer, i) && connection_has_piece(remote_peer, i) &&
          peer_is_not_downloading_piece(peer, i)) {
        if (random_piece_index == current_index) {
          piece = i;
          break;
        }
        current_index++;
      }
    }
    xbt_assert(piece != -1);
    return piece;
  } else { // Rarest first policy
    short min         = SHRT_MAX;
    int nb_min_pieces = 0;
    int current_index = 0;
    // compute the smallest number of copies of available pieces
    for (int i = 0; i < FILE_PIECES; i++) {
      if (peer->pieces_count[i] < min && peer_has_not_piece(peer, i) && connection_has_piece(remote_peer, i) &&
          peer_is_not_downloading_piece(peer, i))
        min = peer->pieces_count[i];
    }
    xbt_assert(min != SHRT_MAX || (is_interested_and_free(peer, remote_peer) == 0));
    // compute the number of rarest pieces
    for (int i = 0; i < FILE_PIECES; i++) {
      if (peer->pieces_count[i] == min && peer_has_not_piece(peer, i) && connection_has_piece(remote_peer, i) &&
          peer_is_not_downloading_piece(peer, i))
        nb_min_pieces++;
    }
    xbt_assert(nb_min_pieces != 0 || (is_interested_and_free(peer, remote_peer) == 0));
    // get a random rarest piece
    int random_rarest_index = 0;
    if (nb_min_pieces > 0) {
      random_rarest_index = rand() % nb_min_pieces;
    }
    for (int i = 0; i < FILE_PIECES; i++) {
      if (peer->pieces_count[i] == min && peer_has_not_piece(peer, i) && connection_has_piece(remote_peer, i) &&
          peer_is_not_downloading_piece(peer, i)) {
        if (random_rarest_index == current_index) {
          piece = i;
          break;
        }
        current_index++;
      }
    }
    xbt_assert(piece != -1 || (is_interested_and_free(peer, remote_peer) == 0));
    return piece;
  }
}

/** Update the list of current choked and unchoked peers, using the choke algorithm */
void update_choked_peers(peer_t peer)
{
  if (nb_interested_peers(peer) == 0)
    return;
  XBT_DEBUG("(%d) update_choked peers %u active peers", peer->id, xbt_dict_size(peer->active_peers));
  // update the current round
  peer->round = (peer->round + 1) % 3;
  char* key;
  char* choked_key         = NULL;
  connection_t chosen_peer = NULL;
  connection_t choked_peer = NULL;
  // remove a peer from the list
  xbt_dict_cursor_t cursor = NULL;
  xbt_dict_cursor_first(peer->active_peers, &cursor);
  if (!xbt_dict_is_empty(peer->active_peers)) {
    choked_key  = xbt_dict_cursor_get_key(cursor);
    choked_peer = xbt_dict_cursor_get_data(cursor);
  }
  xbt_dict_cursor_free(&cursor);

  /**If we are currently seeding, we unchoke the peer which has been unchoked the last time.*/
  if (has_finished(peer->bitfield)) {
    connection_t connection;
    double unchoke_time = simgrid_get_clock() + 1;

    xbt_dict_foreach (peer->connected_peers, cursor, key, connection) {
      if (connection->last_unchoke < unchoke_time && (connection->interested != 0) &&
          (connection->choked_upload != 0)) {
        unchoke_time = connection->last_unchoke;
        chosen_peer  = connection;
      }
    }
  } else {
    // Random optimistic unchoking
    if (peer->round == 0) {
      int j = 0;
      do {
        // We choose a random peer to unchoke.
        int id_chosen = 0;
        if (xbt_dict_length(peer->connected_peers) > 0) {
          id_chosen = rand() % xbt_dict_length(peer->connected_peers);
        }
        int i = 0;
        connection_t connection;
        xbt_dict_foreach (peer->connected_peers, cursor, key, connection) {
          if (i == id_chosen) {
            chosen_peer = connection;
            break;
          }
          i++;
        }
        xbt_dict_cursor_free(&cursor);
        xbt_assert(chosen_peer != NULL, "A peer should have been selected at this point");
        if ((chosen_peer->interested == 0) || (chosen_peer->choked_upload == 0))
          chosen_peer = NULL;
        else
          XBT_DEBUG("Nothing to do, keep going");
        j++;
      } while (chosen_peer == NULL && j < MAXIMUM_PEERS);
    } else {
      // Use the "fastest download" policy.
      connection_t connection;
      double fastest_speed = 0.0;
      xbt_dict_foreach (peer->connected_peers, cursor, key, connection) {
        if (connection->peer_speed > fastest_speed && (connection->choked_upload != 0) &&
            (connection->interested != 0)) {
          chosen_peer   = connection;
          fastest_speed = connection->peer_speed;
        }
      }
    }
  }

  if (chosen_peer != NULL)
    XBT_DEBUG("(%d) update_choked peers unchoked (%d) ; int (%d) ; choked (%d) ", peer->id, chosen_peer->id,
              chosen_peer->interested, chosen_peer->choked_upload);

  if (choked_peer != chosen_peer) {
    if (choked_peer != NULL) {
      xbt_assert((!choked_peer->choked_upload), "Tries to choked a choked peer");
      choked_peer->choked_upload = 1;
      xbt_assert((*((int*)choked_key) == choked_peer->id));
      update_active_peers_set(peer, choked_peer);
      XBT_DEBUG("(%d) Sending a CHOKE to %d", peer->id, choked_peer->id);
      send_message(peer, choked_peer->mailbox, MESSAGE_CHOKE, MESSAGE_CHOKE_SIZE);
    }
    if (chosen_peer != NULL) {
      xbt_assert((chosen_peer->choked_upload), "Tries to unchoked an unchoked peer");
      chosen_peer->choked_upload = 0;
      xbt_dict_set_ext(peer->active_peers, (char*)&chosen_peer->id, sizeof(int), chosen_peer);
      chosen_peer->last_unchoke = simgrid_get_clock();
      XBT_DEBUG("(%d) Sending a UNCHOKE to %d", peer->id, chosen_peer->id);
      update_active_peers_set(peer, chosen_peer);
      send_message(peer, chosen_peer->mailbox, MESSAGE_UNCHOKE, MESSAGE_UNCHOKE_SIZE);
    }
  }
}

/** Update "interested" state of peers: send "not interested" to peers that don't have any more pieces we want. */
void update_interested_after_receive(const_peer_t peer)
{
  char* key;
  xbt_dict_cursor_t cursor;
  connection_t connection;
  xbt_dict_foreach (peer->connected_peers, cursor, key, connection) {
    if (connection->am_interested != 0) {
      int interested = 0;
      // Check if the peer still has a piece we want.
      for (int i = 0; i < FILE_PIECES; i++) {
        if (peer_has_not_piece(peer, i) && connection_has_piece(connection, i)) {
          interested = 1;
          break;
        }
      }
      if (!interested) { // no more piece to download from connection
        connection->am_interested = 0;
        send_message(peer, connection->mailbox, MESSAGE_NOTINTERESTED, MESSAGE_NOTINTERESTED_SIZE);
      }
    }
  }
}

void update_bitfield_blocks(peer_t peer, int index, int block_index, int block_length)
{
  xbt_assert((index >= 0 && index <= FILE_PIECES), "Wrong piece.");
  xbt_assert((block_index >= 0 && block_index <= PIECES_BLOCKS), "Wrong block : %d.", block_index);
  for (int i = block_index; i < (block_index + block_length); i++) {
    peer->bitfield_blocks |= (1ULL << (unsigned int)(index * PIECES_BLOCKS + i));
  }
}

/** Returns if a peer has completed the download of a piece */
int piece_complete(const_peer_t peer, int index)
{
  for (int i = 0; i < PIECES_BLOCKS; i++) {
    if (!(peer->bitfield_blocks & 1ULL << (index * PIECES_BLOCKS + i))) {
      return 0;
    }
  }
  return 1;
}

/** Returns the first block that a peer doesn't have in a piece. If the peer has all blocks of the piece, returns -1. */
int get_first_missing_block_from(const_peer_t peer, int piece)
{
  for (int i = 0; i < PIECES_BLOCKS; i++) {
    if (!(peer->bitfield_blocks & 1ULL << (piece * PIECES_BLOCKS + i))) {
      return i;
    }
  }
  return -1;
}

/** Returns a piece that is partially downloaded and stored by the remote peer if any -1 otherwise. */
int partially_downloaded_piece(const_peer_t peer, const_connection_t remote_peer)
{
  for (int i = 0; i < FILE_PIECES; i++) {
    if (peer_has_not_piece(peer, i) && connection_has_piece(remote_peer, i) && peer_is_not_downloading_piece(peer, i) &&
        get_first_missing_block_from(peer, i) > 0)
      return i;
  }
  return -1;
}

int peer_has_not_piece(const_peer_t peer, unsigned int piece)
{
  return !(peer->bitfield & 1U << piece);
}

/** Check that a piece is not currently being download by the peer. */
int peer_is_not_downloading_piece(const_peer_t peer, unsigned int piece)
{
  return !(peer->current_pieces & 1U << piece);
}

/***************** Connection internal functions ***********************/
connection_t connection_new(int id)
{
  connection_t connection = xbt_new(s_connection_t, 1);
  char mailbox_name[MAILBOX_SIZE];
  snprintf(mailbox_name, MAILBOX_SIZE - 1, "%d", id);
  connection->id              = id;
  connection->mailbox         = sg_mailbox_by_name(mailbox_name);
  connection->bitfield        = 0;
  connection->peer_speed      = 0;
  connection->last_unchoke    = 0;
  connection->current_piece   = -1;
  connection->am_interested   = 0;
  connection->interested      = 0;
  connection->choked_upload   = 1;
  connection->choked_download = 1;

  return connection;
}

void connection_add_speed_value(connection_t connection, double speed)
{
  connection->peer_speed = connection->peer_speed * 0.6 + speed * 0.4;
}

int connection_has_piece(const_connection_t connection, unsigned int piece)
{
  return (connection->bitfield & 1U << piece);
}

/***************** Messages creation functions ***********************/
/** @brief Build a new empty message */
message_t message_new(e_message_type type, int peer_id, sg_mailbox_t return_mailbox)
{
  message_t message       = xbt_new(s_message_t, 1);
  message->peer_id        = peer_id;
  message->return_mailbox = return_mailbox;
  message->type           = type;
  return message;
}

/** Builds a message containing an index. */
message_t message_index_new(e_message_type type, int peer_id, sg_mailbox_t return_mailbox, int index)
{
  message_t message = message_new(type, peer_id, return_mailbox);
  message->piece    = index;
  return message;
}

message_t message_other_new(e_message_type type, int peer_id, sg_mailbox_t return_mailbox, unsigned int bitfield)
{
  message_t message = message_new(type, peer_id, return_mailbox);
  message->bitfield = bitfield;
  return message;
}

message_t message_request_new(int peer_id, sg_mailbox_t return_mailbox, int piece, int block_index, int block_length)
{
  message_t message     = message_index_new(MESSAGE_REQUEST, peer_id, return_mailbox, piece);
  message->block_index  = block_index;
  message->block_length = block_length;
  return message;
}

message_t message_piece_new(int peer_id, sg_mailbox_t return_mailbox, int piece, int block_index, int block_length)
{
  message_t message     = message_index_new(MESSAGE_PIECE, peer_id, return_mailbox, piece);
  message->block_index  = block_index;
  message->block_length = block_length;
  return message;
}
