/* Copyright (c) 2012-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "bittorrent-peer.h"
#include "bittorrent-messages.h"
#include "connection.h"
#include "tracker.h"
#include <simgrid/msg.h>

#include <limits.h>
#include <stdio.h> /* snprintf */

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_peers, "Messages specific for the peers");

/*
 * User parameters for transferred file data. For the test, the default values are :
 * File size: 10 pieces * 5 blocks/piece * 16384 bytes/block = 819200 bytes
 */
#define FILE_PIECES 10UL
#define PIECES_BLOCKS 5UL
#define BLOCK_SIZE 16384
static const unsigned long int FILE_SIZE = FILE_PIECES * PIECES_BLOCKS * BLOCK_SIZE;

/** Number of blocks asked by each request */
#define BLOCKS_REQUESTED 2

#define SLEEP_DURATION 1

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

int peer_has_not_piece(peer_t peer, unsigned int piece)
{
  return !(peer->bitfield & 1U << piece);
}

/** Check that a piece is not currently being download by the peer. */
int peer_is_not_downloading_piece(peer_t peer, unsigned int piece)
{
  return !(peer->current_pieces & 1U << piece);
}

void get_status(char** status, unsigned int bitfield)
{
  for (int i             = FILE_PIECES - 1; i >= 0; i--)
    (*status)[i]         = (bitfield & (1U << i)) ? '1' : '0';
  (*status)[FILE_PIECES] = '\0';
}

/** Peer main function */
int peer(int argc, char* argv[])
{
  // Check arguments
  xbt_assert(argc == 3 || argc == 4, "Wrong number of arguments");

  // Build peer object
  peer_t peer = peer_init(xbt_str_parse_int(argv[1], "Invalid ID: %s"), argc == 4 ? 1 : 0);

  // Retrieve deadline
  double deadline = xbt_str_parse_double(argv[2], "Invalid deadline: %s");
  xbt_assert(deadline > 0, "Wrong deadline supplied");

  char* status = xbt_malloc0(FILE_PIECES + 1);
  get_status(&status, peer->bitfield);
  XBT_INFO("Hi, I'm joining the network with id %d", peer->id);
  // Getting peer data from the tracker.
  if (get_peers_data(peer)) {
    XBT_DEBUG("Got %d peers from the tracker. Current status is: %s", xbt_dict_length(peer->peers), status);
    peer->begin_receive_time = MSG_get_clock();
    MSG_mailbox_set_async(peer->mailbox);
    if (has_finished(peer->bitfield)) {
      send_handshake_all(peer);
    } else {
      leech_loop(peer, deadline);
    }
    seed_loop(peer, deadline);
  } else {
    XBT_INFO("Couldn't contact the tracker.");
  }

  get_status(&status, peer->bitfield);
  XBT_INFO("Here is my current status: %s", status);
  if (peer->comm_received) {
    MSG_comm_destroy(peer->comm_received);
  }

  xbt_free(status);
  peer_free(peer);
  return 0;
}

/** @brief Peer main loop when it is leeching.
 *  @param peer peer data
 *  @param deadline time at which the peer has to leave
 */
void leech_loop(peer_t peer, double deadline)
{
  double next_choked_update = MSG_get_clock() + UPDATE_CHOKED_INTERVAL;
  XBT_DEBUG("Start downloading.");

  /* Send a "handshake" message to all the peers it got (since it couldn't have gotten more than 50 peers) */
  send_handshake_all(peer);
  XBT_DEBUG("Starting main leech loop");

  while (MSG_get_clock() < deadline && count_pieces(peer->bitfield) < FILE_PIECES) {
    if (peer->comm_received == NULL) {
      peer->task_received = NULL;
      peer->comm_received = MSG_task_irecv(&peer->task_received, peer->mailbox);
    }
    if (MSG_comm_test(peer->comm_received)) {
      msg_error_t status = MSG_comm_get_status(peer->comm_received);
      MSG_comm_destroy(peer->comm_received);
      peer->comm_received = NULL;
      if (status == MSG_OK) {
        handle_message(peer, peer->task_received);
      }
    } else {
      // We don't execute the choke algorithm if we don't already have a piece
      if (MSG_get_clock() >= next_choked_update && count_pieces(peer->bitfield) > 0) {
        update_choked_peers(peer);
        next_choked_update += UPDATE_CHOKED_INTERVAL;
      } else {
        MSG_process_sleep(SLEEP_DURATION);
      }
    }
  }
  if (has_finished(peer->bitfield))
    XBT_DEBUG("%d becomes a seeder", peer->id);
}

/** @brief Peer main loop when it is seeding
 *  @param peer peer data
 *  @param deadline time when the peer will leave
 */
void seed_loop(peer_t peer, double deadline)
{
  double next_choked_update = MSG_get_clock() + UPDATE_CHOKED_INTERVAL;
  XBT_DEBUG("Start seeding.");
  // start the main seed loop
  while (MSG_get_clock() < deadline) {
    if (peer->comm_received == NULL) {
      peer->task_received = NULL;
      peer->comm_received = MSG_task_irecv(&peer->task_received, peer->mailbox);
    }
    if (MSG_comm_test(peer->comm_received)) {
      msg_error_t status = MSG_comm_get_status(peer->comm_received);
      MSG_comm_destroy(peer->comm_received);
      peer->comm_received = NULL;
      if (status == MSG_OK) {
        handle_message(peer, peer->task_received);
      }
    } else {
      if (MSG_get_clock() >= next_choked_update) {
        update_choked_peers(peer);
        // TODO: Change the choked peer algorithm when seeding.
        next_choked_update += UPDATE_CHOKED_INTERVAL;
      } else {
        MSG_process_sleep(SLEEP_DURATION);
      }
    }
  }
}

/** @brief Retrieves the peer list from the tracker
 *  @param peer current peer data
 */
int get_peers_data(peer_t peer)
{
  int success    = 0;
  double timeout = MSG_get_clock() + GET_PEERS_TIMEOUT;

  // Build the task to send to the tracker
  tracker_task_data_t data =
      tracker_task_data_new(MSG_host_get_name(MSG_host_self()), peer->mailbox_tracker, peer->id, 0, 0, FILE_SIZE);
  msg_task_t task_send = MSG_task_create(NULL, 0, TRACKER_COMM_SIZE, data);
  while ((success == 0) && MSG_get_clock() < timeout) {
    XBT_DEBUG("Sending a peer request to the tracker.");
    msg_error_t status = MSG_task_send_with_timeout(task_send, TRACKER_MAILBOX, GET_PEERS_TIMEOUT);
    if (status == MSG_OK) {
      success = 1;
    }
  }

  success                  = 0;
  msg_task_t task_received = NULL;
  while ((success == 0) && MSG_get_clock() < timeout) {
    msg_comm_t comm_received = MSG_task_irecv(&task_received, peer->mailbox_tracker);
    msg_error_t status       = MSG_comm_wait(comm_received, GET_PEERS_TIMEOUT);
    if (status == MSG_OK) {
      tracker_task_data_t data = MSG_task_get_data(task_received);
      unsigned i;
      int peer_id;
      // Add the peers the tracker gave us to our peer list.
      xbt_dynar_foreach (data->peers, i, peer_id) {
        if (peer_id != peer->id)
          xbt_dict_set_ext(peer->peers, (char*)&peer_id, sizeof(int), connection_new(peer_id), NULL);
      }
      success = 1;
      // free the communication and the task
      MSG_comm_destroy(comm_received);
      tracker_task_data_free(data);
      MSG_task_destroy(task_received);
    }
  }

  return success;
}

/** @brief Initialize the peer data.
 *  @param peer peer data
 *  @param id id of the peer to take in the network
 *  @param seed indicates if the peer is a seed.
 */
peer_t peer_init(int id, int seed)
{
  peer_t peer    = xbt_new(s_peer_t, 1);
  peer->id       = id;
  peer->hostname = MSG_host_get_name(MSG_host_self());

  snprintf(peer->mailbox, MAILBOX_SIZE - 1, "%d", id);
  snprintf(peer->mailbox_tracker, MAILBOX_SIZE - 1, "tracker_%d", id);
  peer->peers        = xbt_dict_new_homogeneous(NULL);
  peer->active_peers = xbt_dict_new_homogeneous(NULL);

  if (seed) {
    peer->bitfield        = (1U << FILE_PIECES) - 1U;
    peer->bitfield_blocks = (1ULL << (FILE_PIECES * PIECES_BLOCKS)) - 1ULL;
  } else {
    peer->bitfield        = 0;
    peer->bitfield_blocks = 0;
  }

  peer->current_pieces = 0;

  peer->pieces_count = xbt_new0(short, FILE_PIECES);

  peer->comm_received = NULL;

  peer->round = 0;

  return peer;
}

/** Destroys a poor peer object. */
void peer_free(peer_t peer)
{
  char* key;
  connection_t connection;
  xbt_dict_cursor_t cursor;
  xbt_dict_foreach (peer->peers, cursor, key, connection) {
    connection_free(connection);
  }
  xbt_dict_free(&peer->peers);
  xbt_dict_free(&peer->active_peers);
  xbt_free(peer->pieces_count);
  xbt_free(peer);
}

/** @brief Returns if a peer has finished downloading the file
 *  @param bitfield peer bitfield
 */
int has_finished(unsigned int bitfield)
{
  return bitfield == (1U << FILE_PIECES) - 1U;
}

int nb_interested_peers(peer_t peer)
{
  xbt_dict_cursor_t cursor = NULL;
  char* key;
  connection_t connection;
  int nb = 0;
  xbt_dict_foreach (peer->peers, cursor, key, connection) {
    if (connection->interested)
      nb++;
  }
  return nb;
}

void update_active_peers_set(peer_t peer, connection_t remote_peer)
{
  if ((remote_peer->interested != 0) && (remote_peer->choked_upload == 0)) {
    // add in the active peers set
    xbt_dict_set_ext(peer->active_peers, (char*)&remote_peer->id, sizeof(int), remote_peer, NULL);
  } else if (xbt_dict_get_or_null_ext(peer->active_peers, (char*)&remote_peer->id, sizeof(int))) {
    xbt_dict_remove_ext(peer->active_peers, (char*)&remote_peer->id, sizeof(int));
  }
}

/** @brief Handle a received message sent by another peer
 * @param peer Peer data
 * @param task task received.
 */
void handle_message(peer_t peer, msg_task_t task)
{
  const char* type_names[10] = {"HANDSHAKE", "CHOKE",    "UNCHOKE", "INTERESTED", "NOTINTERESTED",
                                "HAVE",      "BITFIELD", "REQUEST", "PIECE",      "CANCEL"};
  message_t message = MSG_task_get_data(task);
  XBT_DEBUG("Received a %s message from %s (%s)", type_names[message->type], message->mailbox,
            message->issuer_host_name);

  connection_t remote_peer;
  remote_peer = xbt_dict_get_or_null_ext(peer->peers, (char*)&message->peer_id, sizeof(int));

  switch (message->type) {
    case MESSAGE_HANDSHAKE:
      // Check if the peer is in our connection list.
      if (remote_peer == 0) {
        xbt_dict_set_ext(peer->peers, (char*)&message->peer_id, sizeof(int), connection_new(message->peer_id), NULL);
        send_handshake(peer, message->mailbox);
      }
      // Send our bitfield to the peer
      send_bitfield(peer, message->mailbox);
      break;
    case MESSAGE_BITFIELD:
      // Update the pieces list
      update_pieces_count_from_bitfield(peer, message->bitfield);
      // Store the bitfield
      remote_peer->bitfield = message->bitfield;
      xbt_assert(!remote_peer->am_interested, "Should not be interested at first");
      if (is_interested(peer, remote_peer)) {
        remote_peer->am_interested = 1;
        send_interested(peer, message->mailbox);
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
      XBT_DEBUG("\t for piece %d", message->index);
      xbt_assert((message->index >= 0 && message->index < FILE_PIECES), "Wrong HAVE message received");
      remote_peer->bitfield = remote_peer->bitfield | (1U << message->index);
      peer->pieces_count[message->index]++;
      // If the piece is in our pieces, we tell the peer that we are interested.
      if ((remote_peer->am_interested == 0) && peer_has_not_piece(peer, message->index)) {
        remote_peer->am_interested = 1;
        send_interested(peer, message->mailbox);
        if (remote_peer->choked_download == 0)
          request_new_piece_to_peer(peer, remote_peer);
      }
      break;
    case MESSAGE_REQUEST:
      xbt_assert(remote_peer->interested);
      xbt_assert((message->index >= 0 && message->index < FILE_PIECES), "Wrong request received");
      if (remote_peer->choked_upload == 0) {
        XBT_DEBUG("\t for piece %d (%d,%d)", message->index, message->block_index,
                  message->block_index + message->block_length);
        if (!peer_has_not_piece(peer, message->index)) {
          send_piece(peer, message->mailbox, message->index, message->block_index, message->block_length);
        }
      } else {
        XBT_DEBUG("\t for piece %d but he is choked.", message->peer_id);
      }
      break;
    case MESSAGE_PIECE:
      XBT_DEBUG(" \t for piece %d (%d,%d)", message->index, message->block_index,
                message->block_index + message->block_length);
      xbt_assert(!remote_peer->choked_download);
      xbt_assert(remote_peer->choked_download != 1, "Can't received a piece if I'm choked !");
      xbt_assert((message->index >= 0 && message->index < FILE_PIECES), "Wrong piece received");
      // TODO: Execute à computation.
      if (peer_has_not_piece(peer, message->index)) {
        update_bitfield_blocks(peer, message->index, message->block_index, message->block_length);
        if (piece_complete(peer, message->index)) {
          // Removing the piece from our piece list
          remove_current_piece(peer, remote_peer, message->index);
          // Setting the fact that we have the piece
          peer->bitfield = peer->bitfield | (1U << message->index);
          char* status   = xbt_malloc0(FILE_PIECES + 1);
          get_status(&status, peer->bitfield);
          XBT_DEBUG("My status is now %s", status);
          xbt_free(status);
          // Sending the information to all the peers we are connected to
          send_have(peer, message->index);
          // sending UNINTERESTED to peers that do not have what we want.
          update_interested_after_receive(peer);
        } else {                                                   // piece not completed
          send_request_to_peer(peer, remote_peer, message->index); // ask for the next block
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
    connection_add_speed_value(remote_peer, 1.0 / (MSG_get_clock() - peer->begin_receive_time));
  }
  peer->begin_receive_time = MSG_get_clock();

  task_message_free(task);
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

/** @brief Updates the list of who has a piece from a bitfield
 *  @param peer peer we want to update the list
 *  @param bitfield bitfield
 */
void update_pieces_count_from_bitfield(peer_t peer, unsigned int bitfield)
{
  for (int i = 0; i < FILE_PIECES; i++) {
    if (bitfield & (1U << i)) {
      peer->pieces_count[i]++;
    }
  }
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
int select_piece_to_download(peer_t peer, connection_t remote_peer)
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

/** @brief Update the list of current choked and unchoked peers, using the choke algorithm
 *  @param peer the current peer
 */
void update_choked_peers(peer_t peer)
{
  if (nb_interested_peers(peer) == 0)
    return;
  XBT_DEBUG("(%d) update_choked peers %u active peers", peer->id, xbt_dict_size(peer->active_peers));
  // update the current round
  peer->round = (peer->round + 1) % 3;
  char* key;
  char* key_choked         = NULL;
  connection_t peer_chosen = NULL;
  connection_t peer_choked = NULL;
  // remove a peer from the list
  xbt_dict_cursor_t cursor = NULL;
  xbt_dict_cursor_first(peer->active_peers, &cursor);
  if (!xbt_dict_is_empty(peer->active_peers)) {
    key_choked  = xbt_dict_cursor_get_key(cursor);
    peer_choked = xbt_dict_cursor_get_data(cursor);
  }
  xbt_dict_cursor_free(&cursor);

  /**If we are currently seeding, we unchoke the peer which has been unchoked the last time.*/
  if (has_finished(peer->bitfield)) {
    connection_t connection;
    double unchoke_time = MSG_get_clock() + 1;

    xbt_dict_foreach (peer->peers, cursor, key, connection) {
      if (connection->last_unchoke < unchoke_time && (connection->interested != 0) &&
          (connection->choked_upload != 0)) {
        unchoke_time = connection->last_unchoke;
        peer_chosen  = connection;
      }
    }
  } else {
    // Random optimistic unchoking
    if (peer->round == 0) {
      int j = 0;
      do {
        // We choose a random peer to unchoke.
        int id_chosen = 0;
        if (xbt_dict_length(peer->peers) > 0) {
          id_chosen = rand() % xbt_dict_length(peer->peers);
        }
        int i         = 0;
        connection_t connection;
        xbt_dict_foreach (peer->peers, cursor, key, connection) {
          if (i == id_chosen) {
            peer_chosen = connection;
            break;
          }
          i++;
        }
        xbt_dict_cursor_free(&cursor);
        xbt_assert(peer_chosen != NULL, "A peer should have been selected at this point");
        if ((peer_chosen->interested == 0) || (peer_chosen->choked_upload == 0))
          peer_chosen = NULL;
        else
          XBT_DEBUG("Nothing to do, keep going");
        j++;
      } while (peer_chosen == NULL && j < MAXIMUM_PEERS);
    } else {
      // Use the "fastest download" policy.
      connection_t connection;
      double fastest_speed = 0.0;
      xbt_dict_foreach (peer->peers, cursor, key, connection) {
        if (connection->peer_speed > fastest_speed && (connection->choked_upload != 0) &&
            (connection->interested != 0)) {
          peer_chosen   = connection;
          fastest_speed = connection->peer_speed;
        }
      }
    }
  }

  if (peer_chosen != NULL)
    XBT_DEBUG("(%d) update_choked peers unchoked (%d) ; int (%d) ; choked (%d) ", peer->id, peer_chosen->id,
              peer_chosen->interested, peer_chosen->choked_upload);

  if (peer_choked != peer_chosen) {
    if (peer_choked != NULL) {
      xbt_assert((!peer_choked->choked_upload), "Tries to choked a choked peer");
      peer_choked->choked_upload = 1;
      xbt_assert((*((int*)key_choked) == peer_choked->id));
      update_active_peers_set(peer, peer_choked);
      XBT_DEBUG("(%d) Sending a CHOKE to %d", peer->id, peer_choked->id);
      send_choked(peer, peer_choked->mailbox);
    }
    if (peer_chosen != NULL) {
      xbt_assert((peer_chosen->choked_upload), "Tries to unchoked an unchoked peer");
      peer_chosen->choked_upload = 0;
      xbt_dict_set_ext(peer->active_peers, (char*)&peer_chosen->id, sizeof(int), peer_chosen, NULL);
      peer_chosen->last_unchoke = MSG_get_clock();
      XBT_DEBUG("(%d) Sending a UNCHOKE to %d", peer->id, peer_chosen->id);
      update_active_peers_set(peer, peer_chosen);
      send_unchoked(peer, peer_chosen->mailbox);
    }
  }
}

/** @brief Update "interested" state of peers: send "not interested" to peers that don't have any more pieces we want.
 *  @param peer our peer data
 */
void update_interested_after_receive(peer_t peer)
{
  char* key;
  xbt_dict_cursor_t cursor;
  connection_t connection;
  xbt_dict_foreach (peer->peers, cursor, key, connection) {
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
        send_notinterested(peer, connection->mailbox);
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
int piece_complete(peer_t peer, int index)
{
  for (int i = 0; i < PIECES_BLOCKS; i++) {
    if (!(peer->bitfield_blocks & 1ULL << (index * PIECES_BLOCKS + i))) {
      return 0;
    }
  }
  return 1;
}

/** Returns the first block that a peer doesn't have in a piece. If the peer has all blocks of the piece, returns -1. */
int get_first_block(peer_t peer, int piece)
{
  for (int i = 0; i < PIECES_BLOCKS; i++) {
    if (!(peer->bitfield_blocks & 1ULL << (piece * PIECES_BLOCKS + i))) {
      return i;
    }
  }
  return -1;
}

/** Indicates if the remote peer has a piece not stored by the local peer */
int is_interested(peer_t peer, connection_t remote_peer)
{
  return remote_peer->bitfield & (peer->bitfield ^ ((1 << FILE_PIECES) - 1));
}

/** Indicates if the remote peer has a piece not stored by the local peer nor requested by the local peer */
int is_interested_and_free(peer_t peer, connection_t remote_peer)
{
  for (int i = 0; i < FILE_PIECES; i++) {
    if (peer_has_not_piece(peer, i) && connection_has_piece(remote_peer, i) && peer_is_not_downloading_piece(peer, i)) {
      return 1;
    }
  }
  return 0;
}

/** Returns a piece that is partially downloaded and stored by the remote peer if any -1 otherwise. */
int partially_downloaded_piece(peer_t peer, connection_t remote_peer)
{
  for (int i = 0; i < FILE_PIECES; i++) {
    if (peer_has_not_piece(peer, i) && connection_has_piece(remote_peer, i) && peer_is_not_downloading_piece(peer, i) &&
        get_first_block(peer, i) > 0)
      return i;
  }
  return -1;
}

/** @brief Send request messages to a peer that have unchoked us
 *  @param peer peer
 *  @param remote_peer peer data to the peer we want to send the request
 */
void send_request_to_peer(peer_t peer, connection_t remote_peer, int piece)
{
  remote_peer->current_piece = piece;
  xbt_assert(connection_has_piece(remote_peer, piece));
  int block_index = get_first_block(peer, piece);
  if (block_index != -1) {
    int block_length = MIN(BLOCKS_REQUESTED, PIECES_BLOCKS - block_index);
    send_request(peer, remote_peer->mailbox, piece, block_index, block_length);
  }
}

/***********************************************************
 *
 *  Low level message functions
 *
 ***********************************************************/

/** @brief Send a "interested" message to a peer
 *  @param peer peer data
 *  @param mailbox destination mailbox
 */
void send_interested(peer_t peer, const char* mailbox)
{
  msg_task_t task = task_message_new(MESSAGE_INTERESTED, peer->hostname, peer->mailbox, peer->id,
                                     task_message_size(MESSAGE_INTERESTED));
  MSG_task_dsend(task, mailbox, task_message_free);
  XBT_DEBUG("Sending INTERESTED to %s", mailbox);
}

/** @brief Send a "not interested" message to a peer
 *  @param peer peer data
 *  @param mailbox destination mailbox
 */
void send_notinterested(peer_t peer, const char* mailbox)
{
  msg_task_t task = task_message_new(MESSAGE_NOTINTERESTED, peer->hostname, peer->mailbox, peer->id,
                                     task_message_size(MESSAGE_NOTINTERESTED));
  MSG_task_dsend(task, mailbox, task_message_free);
  XBT_DEBUG("Sending NOTINTERESTED to %s", mailbox);
}

/** @brief Send a handshake message to all the peers the peer has.
 *  @param peer peer data
 */
void send_handshake_all(peer_t peer)
{
  connection_t remote_peer;
  xbt_dict_cursor_t cursor = NULL;
  char* key;
  xbt_dict_foreach (peer->peers, cursor, key, remote_peer) {
    msg_task_t task = task_message_new(MESSAGE_HANDSHAKE, peer->hostname, peer->mailbox, peer->id,
                                       task_message_size(MESSAGE_HANDSHAKE));
    MSG_task_dsend(task, remote_peer->mailbox, task_message_free);
    XBT_DEBUG("Sending a HANDSHAKE to %s", remote_peer->mailbox);
  }
}

/** @brief Send a "handshake" message to an user
 *  @param peer peer data
 *  @param mailbox mailbox where to we send the message
 */
void send_handshake(peer_t peer, const char* mailbox)
{
  XBT_DEBUG("Sending a HANDSHAKE to %s", mailbox);
  msg_task_t task = task_message_new(MESSAGE_HANDSHAKE, peer->hostname, peer->mailbox, peer->id,
                                     task_message_size(MESSAGE_HANDSHAKE));
  MSG_task_dsend(task, mailbox, task_message_free);
}

/** Send a "choked" message to a peer. */
void send_choked(peer_t peer, const char* mailbox)
{
  XBT_DEBUG("Sending a CHOKE to %s", mailbox);
  msg_task_t task =
      task_message_new(MESSAGE_CHOKE, peer->hostname, peer->mailbox, peer->id, task_message_size(MESSAGE_CHOKE));
  MSG_task_dsend(task, mailbox, task_message_free);
}

/** Send a "unchoked" message to a peer */
void send_unchoked(peer_t peer, const char* mailbox)
{
  XBT_DEBUG("Sending a UNCHOKE to %s", mailbox);
  msg_task_t task =
      task_message_new(MESSAGE_UNCHOKE, peer->hostname, peer->mailbox, peer->id, task_message_size(MESSAGE_UNCHOKE));
  MSG_task_dsend(task, mailbox, task_message_free);
}

/** Send a "HAVE" message to all peers we are connected to */
void send_have(peer_t peer, int piece)
{
  XBT_DEBUG("Sending HAVE message to all my peers");
  connection_t remote_peer;
  xbt_dict_cursor_t cursor = NULL;
  char* key;
  xbt_dict_foreach (peer->peers, cursor, key, remote_peer) {
    msg_task_t task = task_message_index_new(MESSAGE_HAVE, peer->hostname, peer->mailbox, peer->id, piece,
                                             task_message_size(MESSAGE_HAVE));
    MSG_task_dsend(task, remote_peer->mailbox, task_message_free);
  }
}

/** @brief Send a bitfield message to all the peers the peer has.
 *  @param peer peer data
 */
void send_bitfield(peer_t peer, const char* mailbox)
{
  XBT_DEBUG("Sending a BITFIELD to %s", mailbox);
  msg_task_t task = task_message_bitfield_new(peer->hostname, peer->mailbox, peer->id, peer->bitfield, FILE_PIECES);
  MSG_task_dsend(task, mailbox, task_message_free);
}

/** Send a "request" message to a pair, containing a request for a piece */
void send_request(peer_t peer, const char* mailbox, int piece, int block_index, int block_length)
{
  XBT_DEBUG("Sending a REQUEST to %s for piece %d (%d,%d)", mailbox, piece, block_index, block_length);
  msg_task_t task = task_message_request_new(peer->hostname, peer->mailbox, peer->id, piece, block_index, block_length);
  MSG_task_dsend(task, mailbox, task_message_free);
}

/** Send a "piece" message to a pair, containing a piece of the file */
void send_piece(peer_t peer, const char* mailbox, int piece, int block_index, int block_length)
{
  XBT_DEBUG("Sending the PIECE %d (%d,%d) to %s", piece, block_index, block_length, mailbox);
  xbt_assert(piece >= 0, "Tried to send a piece that doesn't exist.");
  xbt_assert(!peer_has_not_piece(peer, piece), "Tried to send a piece that we doesn't have.");
  msg_task_t task =
      task_message_piece_new(peer->hostname, peer->mailbox, peer->id, piece, block_index, block_length, BLOCK_SIZE);
  MSG_task_dsend(task, mailbox, task_message_free);
}
