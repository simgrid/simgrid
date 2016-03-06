/* Copyright (c) 2012-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "peer.h"
#include "tracker.h"
#include "connection.h"
#include "messages.h"
#include <simgrid/msg.h>
#include <xbt/RngStream.h>
#include <limits.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_peers, "Messages specific for the peers");

//TODO: Let users change this
/*
 * File transfered data
 * For the test, default values are :
 * File size: 10 pieces * 5 blocks/piece * 16384 bytes/block = 819200 bytes
 */

#define FILE_PIECES  10
#define PIECES_BLOCKS 5
#define BLOCK_SIZE  16384
#define ENABLE_END_GAME_MODE 1

/** Number of blocks asked by each request */
#define BLOCKS_REQUESTED 2

static const unsigned long int FILE_SIZE = FILE_PIECES * PIECES_BLOCKS * BLOCK_SIZE;

#define SLEEP_DURATION 1

/** Peer main function */
int peer(int argc, char *argv[])
{
  s_peer_t peer;
  //Check arguments
  xbt_assert(argc == 3 || argc == 4, "Wrong number of arguments");
  //Build peer object
  if (argc == 4) {
    peer_init(&peer, xbt_str_parse_int(argv[1],"Invalid ID: %s"), 1);
  } else {
    peer_init(&peer, xbt_str_parse_int(argv[1],"Invalid ID: %s"), 0);
  }
  //Retrieve deadline
  double deadline = xbt_str_parse_double(argv[2],"Invalid deadline: %s");
  xbt_assert(deadline > 0, "Wrong deadline supplied");
  XBT_INFO("Hi, I'm joining the network with id %d", peer.id);
  //Getting peer data from the tracker.
  if (get_peers_data(&peer)) {
    XBT_DEBUG("Got %d peers from the tracker", xbt_dict_length(peer.peers));
    XBT_DEBUG("Here is my current status: %s", peer.bitfield);
    peer.begin_receive_time = MSG_get_clock();
    MSG_mailbox_set_async(peer.mailbox);
    if (has_finished(peer.bitfield)) {
      peer.pieces = FILE_PIECES;
      send_handshake_all(&peer);
      seed_loop(&peer, deadline);
    } else {
      leech_loop(&peer, deadline);
      seed_loop(&peer, deadline);
    }
  } else {
    XBT_INFO("Couldn't contact the tracker.");
  }

  XBT_INFO("Here is my current status: %s", peer.bitfield);
  if (peer.comm_received) {
    MSG_comm_destroy(peer.comm_received);
  }

  peer_free(&peer);
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
  /*
   * Send a "handshake" message to all the peers it got
   * (since it couldn't have gotten more than 50 peers)
   */
  send_handshake_all(peer);
  XBT_DEBUG("Starting main leech loop");

  while (MSG_get_clock() < deadline && peer->pieces < FILE_PIECES) {
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
      //We don't execute the choke algorithm if we don't already have a piece
      if (MSG_get_clock() >= next_choked_update && peer->pieces > 0) {
        update_choked_peers(peer);
        next_choked_update += UPDATE_CHOKED_INTERVAL;
      } else {
        MSG_process_sleep(SLEEP_DURATION);
      }
    }
  }
  if (peer->pieces == FILE_PIECES)
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
  //start the main seed loop
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
        //TODO: Change the choked peer algorithm when seeding.
        next_choked_update += UPDATE_CHOKED_INTERVAL;
      } else {
        MSG_process_sleep(SLEEP_DURATION);
      }
    }
  }
}

/** @brief Retrieves the peer list from the tracker
 *  @param peer current peer data
 */
int get_peers_data(peer_t peer)
{
  int success = 0, send_success = 0;
  double timeout = MSG_get_clock() + GET_PEERS_TIMEOUT;
  //Build the task to send to the tracker
  tracker_task_data_t data = tracker_task_data_new(MSG_host_get_name(MSG_host_self()), peer->mailbox_tracker,
                                                   peer->id, 0, 0, FILE_SIZE);
  //Build the task to send.
  msg_task_t task_send = MSG_task_create(NULL, 0, TRACKER_COMM_SIZE, data);
  msg_task_t task_received = NULL;
  msg_comm_t comm_received;
  while (!send_success && MSG_get_clock() < timeout) {
    XBT_DEBUG("Sending a peer request to the tracker.");
    msg_error_t status = MSG_task_send_with_timeout(task_send, TRACKER_MAILBOX, GET_PEERS_TIMEOUT);
    if (status == MSG_OK) {
      send_success = 1;
    }
  }
  while (!success && MSG_get_clock() < timeout) {
    comm_received = MSG_task_irecv(&task_received, peer->mailbox_tracker);
    msg_error_t status = MSG_comm_wait(comm_received, GET_PEERS_TIMEOUT);
    if (status == MSG_OK) {
      tracker_task_data_t data = MSG_task_get_data(task_received);
      unsigned i;
      int peer_id;
      //Add the peers the tracker gave us to our peer list.
      xbt_dynar_foreach(data->peers, i, peer_id) {
        if (peer_id != peer->id)
          xbt_dict_set_ext(peer->peers, (char *) &peer_id, sizeof(int), connection_new(peer_id), NULL);
      }
      success = 1;
      //free the communication and the task
      MSG_comm_destroy(comm_received);
      tracker_task_data_free(data);
      MSG_task_destroy(task_received);
      comm_received = NULL;
    }
  }

  return success;
}

/** @brief Initialize the peer data.
 *  @param peer peer data
 *  @param id id of the peer to take in the network
 *  @param seed indicates if the peer is a seed.
 */
void peer_init(peer_t peer, int id, int seed)
{
  peer->id = id;
  sprintf(peer->mailbox, "%d", id);
  sprintf(peer->mailbox_tracker, "tracker_%d", id);
  peer->peers = xbt_dict_new();
  peer->active_peers = xbt_dict_new();
  peer->hostname = MSG_host_get_name(MSG_host_self());

  peer->bitfield = xbt_new(char, FILE_PIECES + 1);
  peer->bitfield_blocks = xbt_new(char, (FILE_PIECES) * (PIECES_BLOCKS) + 1);
  if (seed) {
    memset(peer->bitfield, '1', sizeof(char) * (FILE_PIECES + 1));
    memset(peer->bitfield_blocks, '1', sizeof(char) * FILE_PIECES * (PIECES_BLOCKS));
  } else {
    memset(peer->bitfield, '0', sizeof(char) * (FILE_PIECES + 1));
    memset(peer->bitfield_blocks, '0', sizeof(char) * FILE_PIECES * (PIECES_BLOCKS));
  }

  peer->bitfield[FILE_PIECES] = '\0';
  peer->pieces = 0;

  peer->pieces_count = xbt_new0(short, FILE_PIECES);

  peer->current_pieces = xbt_dynar_new(sizeof(int), NULL);

  peer->stream = (RngStream)MSG_host_get_property_value(MSG_host_self(), "stream");
  peer->comm_received = NULL;

  peer->round = 0;
}

/** Destroys a poor peer object. */
void peer_free(peer_t peer)
{
  char *key;
  connection_t connection;
  xbt_dict_cursor_t cursor;
  xbt_dict_foreach(peer->peers, cursor, key, connection) {
    connection_free(connection);
  }
  xbt_dict_free(&peer->peers);
  xbt_dict_free(&peer->active_peers);
  xbt_dynar_free(&peer->current_pieces);
  xbt_free(peer->pieces_count);
  xbt_free(peer->bitfield);
  xbt_free(peer->bitfield_blocks);
}

/** @brief Returns if a peer has finished downloading the file
 *  @param bitfield peer bitfield
 */
int has_finished(char *bitfield)
{
  return ((memchr(bitfield, '0', sizeof(char) * FILE_PIECES) == NULL) ? 1 : 0);
}

int nb_interested_peers(peer_t peer)
{
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  connection_t connection;
  int nb = 0;
  xbt_dict_foreach(peer->peers, cursor, key, connection) {
    if (connection->interested)
      nb++;
  }
  return nb;
}

void update_active_peers_set(peer_t peer, connection_t remote_peer)
{
  if (remote_peer->interested && !remote_peer->choked_upload) {
    //add in the active peers set
    xbt_dict_set_ext(peer->active_peers, (char *) &remote_peer->id, sizeof(int), remote_peer, NULL);
  } else {
    //remove
    xbt_ex_t e;
    TRY {
      xbt_dict_remove_ext(peer->active_peers, (char *) &remote_peer->id, sizeof(int));
    }
    CATCH(e) {
      xbt_ex_free(e);
    }
  }
}

/** @brief Handle a received message sent by another peer
 * @param peer Peer data
 * @param task task received.
 */
void handle_message(peer_t peer, msg_task_t task)
{
  message_t message = MSG_task_get_data(task);
  connection_t remote_peer;
  remote_peer = xbt_dict_get_or_null_ext(peer->peers, (char *) &message->peer_id, sizeof(int));
  switch (message->type) {
  case MESSAGE_HANDSHAKE:
    XBT_DEBUG("Received a HANDSHAKE from %s (%s)", message->mailbox, message->issuer_host_name);
    //Check if the peer is in our connection list.
    if (!remote_peer) {
      xbt_dict_set_ext(peer->peers, (char *) &message->peer_id, sizeof(int), connection_new(message->peer_id), NULL);
      send_handshake(peer, message->mailbox);
    }
    //Send our bitfield to the peer
    send_bitfield(peer, message->mailbox);
    break;
  case MESSAGE_BITFIELD:
    XBT_DEBUG("Recieved a BITFIELD message from %s (%s)", message->mailbox, message->issuer_host_name);
    //Update the pieces list
    update_pieces_count_from_bitfield(peer, message->bitfield);
    //Store the bitfield
    remote_peer->bitfield = xbt_strdup(message->bitfield);
    xbt_assert(!remote_peer->am_interested, "Should not be interested at first");
    if (is_interested(peer, remote_peer)) {
      remote_peer->am_interested = 1;
      send_interested(peer, message->mailbox);
    }
    break;
  case MESSAGE_INTERESTED:
    XBT_DEBUG("Recieved an INTERESTED message from %s (%s)", message->mailbox, message->issuer_host_name);
    xbt_assert((remote_peer != NULL), "A non-in-our-list peer has sent us a message. WTH ?");
    //Update the interested state of the peer.
    remote_peer->interested = 1;
    update_active_peers_set(peer, remote_peer);
    break;
  case MESSAGE_NOTINTERESTED:
    XBT_DEBUG("Received a NOTINTERESTED message from %s (%s)", message->mailbox, message->issuer_host_name);
    xbt_assert((remote_peer != NULL), "A non-in-our-list peer has sent us a message. WTH ?");
    remote_peer->interested = 0;
    update_active_peers_set(peer, remote_peer);
    break;
  case MESSAGE_UNCHOKE:
    xbt_assert((remote_peer != NULL), "A non-in-our-list peer has sent us a message. WTH ?");
    XBT_DEBUG("Received a UNCHOKE message from %s (%s)", message->mailbox, message->issuer_host_name);
    xbt_assert(remote_peer->choked_download, "WTF !!!");
    remote_peer->choked_download = 0;
    //Send requests to the peer, since it has unchoked us
    if (remote_peer->am_interested)
      request_new_piece_to_peer(peer, remote_peer);
    break;
  case MESSAGE_CHOKE:
    xbt_assert((remote_peer != NULL), "A non-in-our-list peer has sent us a message. WTH ?");
    XBT_DEBUG("Received a CHOKE message from %s (%s)", message->mailbox, message->issuer_host_name);
    xbt_assert(!remote_peer->choked_download, "WTF !!!");
    remote_peer->choked_download = 1;
    remove_current_piece(peer, remote_peer, remote_peer->current_piece);
    break;
  case MESSAGE_HAVE:
    XBT_DEBUG("Received a HAVE message from %s (%s) of piece %d", message->mailbox, message->issuer_host_name,
              message->index);
    xbt_assert(remote_peer->bitfield, "bitfield not received");
    xbt_assert((message->index >= 0 && message->index < FILE_PIECES), "Wrong HAVE message received");
    remote_peer->bitfield[message->index] = '1';
    peer->pieces_count[message->index]++;
    //If the piece is in our pieces, we tell the peer that we are interested.
    if (!remote_peer->am_interested && peer->bitfield[message->index] == '0') {
      remote_peer->am_interested = 1;
      send_interested(peer, message->mailbox);
      if (!remote_peer->choked_download)
        request_new_piece_to_peer(peer, remote_peer);
    }
    break;
  case MESSAGE_REQUEST:
    xbt_assert(remote_peer->interested, "WTF !!!");

    xbt_assert((message->index >= 0 && message->index < FILE_PIECES), "Wrong request received");
    if (!remote_peer->choked_upload) {
      XBT_DEBUG("Received a REQUEST from %s (%s) for %d (%d,%d)", message->mailbox, message->issuer_host_name,
                message->index, message->block_index, message->block_index + message->block_length);
      if (peer->bitfield[message->index] == '1') {
        send_piece(peer, message->mailbox, message->index, message->block_index, message->block_length);
      }
    } else {
      XBT_DEBUG("Received a REQUEST from %s (%s) for %d but he is choked.", message->mailbox,
                message->issuer_host_name, message->peer_id);
    }
    break;
  case MESSAGE_PIECE:
    XBT_DEBUG("Received piece %d (%d,%d) from %s (%s)", message->index, message->block_index,
              message->block_index + message->block_length, message->mailbox, message->issuer_host_name);
    xbt_assert(!remote_peer->choked_download, "WTF !!!");
    xbt_assert(remote_peer->am_interested || ENABLE_END_GAME_MODE,
               "Can't received a piece if I'm not interested wihtout end-game mode!"
               "piece (%d) bitfield(%s) remote bitfield(%s)", message->index, peer->bitfield, remote_peer->bitfield);
    xbt_assert(remote_peer->choked_download != 1, "Can't received a piece if I'm choked !");
    xbt_assert((message->index >= 0 && message->index < FILE_PIECES), "Wrong piece received");
    //TODO: Execute à computation.
      if (peer->bitfield[message->index] == '0') {
        update_bitfield_blocks(peer, message->index, message->block_index, message->block_length);
        if (piece_complete(peer, message->index)) {
          //Removing the piece from our piece list
          remove_current_piece(peer, remote_peer, message->index);
          //Setting the fact that we have the piece
          peer->bitfield[message->index] = '1';
          peer->pieces++;
          XBT_DEBUG("My status is now %s", peer->bitfield);
          //Sending the information to all the peers we are connected to
          send_have(peer, message->index);
          //sending UNINTERSTED to peers that doesn't have what we want.
          update_interested_after_receive(peer);
        } else {                // piece not completed
          send_request_to_peer(peer, remote_peer, message->index);      // ask for the next block
        }
      } else {
        XBT_DEBUG("However, we already have it");
        xbt_assert(ENABLE_END_GAME_MODE, "Should not happen because we don't use end game mode !");
        request_new_piece_to_peer(peer, remote_peer);
      }
    break;
  case MESSAGE_CANCEL:
    XBT_DEBUG("The received CANCEL from %s (%s)", message->mailbox, message->issuer_host_name);
    break;
  }
  //Update the peer speed.
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
    xbt_dynar_push_as(peer->current_pieces, int, piece);
    send_request_to_peer(peer, remote_peer, piece);
  }
}

/** remove current_piece from the list of currently downloaded pieces. */
void remove_current_piece(peer_t peer, connection_t remote_peer, int current_piece)
{
  int piece_index = -1, piece;
  unsigned int i;
  xbt_dynar_foreach(peer->current_pieces, i, piece) {
    if (piece == current_piece) {
      piece_index = i;
      break;
    }
  }
  if (piece_index != -1)
    xbt_dynar_remove_at(peer->current_pieces, piece_index, NULL);
  remote_peer->current_piece = -1;
}

/** @brief Updates the list of who has a piece from a bitfield
 *  @param peer peer we want to update the list
 *  @param bitfield bitfield
 */
void update_pieces_count_from_bitfield(peer_t peer, char *bitfield)
{
  int i;
  for (i = 0; i < FILE_PIECES; i++) {
    if (bitfield[i] == '1') {
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
  int piece = -1;

  piece = partially_downloaded_piece(peer, remote_peer);
  // strict priority policy
  if (piece != -1)
    return piece;

  // end game mode
  if (xbt_dynar_length(peer->current_pieces) >= (FILE_PIECES - peer->pieces) && is_interested(peer, remote_peer)) {
    if(!ENABLE_END_GAME_MODE)
      return -1;
    int i;
    int nb_interesting_pieces = 0;
    int random_piece_index, current_index = 0;
    // compute the number of interesting pieces
    for (i = 0; i < FILE_PIECES; i++) {
      if (peer->bitfield[i] == '0' && remote_peer->bitfield[i] == '1') {
        nb_interesting_pieces++;
      }
    }
    xbt_assert(nb_interesting_pieces != 0, "WTF !!!");
    // get a random interesting piece
    random_piece_index = RngStream_RandInt(peer->stream, 0, nb_interesting_pieces - 1);
    for (i = 0; i < FILE_PIECES; i++) {
      if (peer->bitfield[i] == '0' && remote_peer->bitfield[i] == '1') {
        if (random_piece_index == current_index) {
          piece = i;
          break;
        }
        current_index++;
      }
    }
    xbt_assert(piece != -1, "WTF !!!");
    return piece;
  }
  // Random first policy
  if (peer->pieces < 4 && is_interested_and_free(peer, remote_peer)) {
    int i;
    int nb_interesting_pieces = 0;
    int random_piece_index, current_index = 0;
    // compute the number of interesting pieces
    for (i = 0; i < FILE_PIECES; i++) {
      if (peer->bitfield[i] == '0' && remote_peer->bitfield[i] == '1' && !in_current_pieces(peer, i)) {
        nb_interesting_pieces++;
      }
    }
    xbt_assert(nb_interesting_pieces != 0, "WTF !!!");
    // get a random interesting piece
    random_piece_index = RngStream_RandInt(peer->stream, 0, nb_interesting_pieces - 1);
    for (i = 0; i < FILE_PIECES; i++) {
      if (peer->bitfield[i] == '0' && remote_peer->bitfield[i] == '1' && !in_current_pieces(peer, i)) {
        if (random_piece_index == current_index) {
          piece = i;
          break;
        }
        current_index++;
      }
    }
    xbt_assert(piece != -1, "WTF !!!");
    return piece;
  } else {                      // Rarest first policy
    int i;
    short min = SHRT_MAX;
    int nb_min_pieces = 0;
    int random_rarest_index, current_index = 0;
    // compute the smallest number of copies of available pieces
    for (i = 0; i < FILE_PIECES; i++) {
      if (peer->pieces_count[i] < min && peer->bitfield[i] == '0' && remote_peer->bitfield[i] == '1'
          && !in_current_pieces(peer, i))
        min = peer->pieces_count[i];
    }
    xbt_assert(min != SHRT_MAX || !is_interested_and_free(peer, remote_peer), "WTF !!!");
    // compute the number of rarest pieces
    for (i = 0; i < FILE_PIECES; i++) {
      if (peer->pieces_count[i] == min && peer->bitfield[i] == '0' && remote_peer->bitfield[i] == '1'
          && !in_current_pieces(peer, i))
        nb_min_pieces++;
    }
    xbt_assert(nb_min_pieces != 0 || !is_interested_and_free(peer, remote_peer), "WTF !!!");
    // get a random rarest piece
    random_rarest_index = RngStream_RandInt(peer->stream, 0, nb_min_pieces - 1);
    for (i = 0; i < FILE_PIECES; i++) {
      if (peer->pieces_count[i] == min && peer->bitfield[i] == '0' && remote_peer->bitfield[i] == '1'
          && !in_current_pieces(peer, i)) {
        if (random_rarest_index == current_index) {
          piece = i;
          break;
        }
        current_index++;
      }
    }
    xbt_assert(piece != -1 || !is_interested_and_free(peer, remote_peer), "WTF !!!");
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
  XBT_DEBUG("(%d) update_choked peers %d active peers", peer->id, xbt_dict_size(peer->active_peers));
  //update the current round
  peer->round = (peer->round + 1) % 3;
  char *key, *key_choked=NULL;
  connection_t peer_choosed = NULL;
  connection_t peer_choked = NULL;
  //remove a peer from the list
  xbt_dict_cursor_t cursor = NULL;
  xbt_dict_cursor_first(peer->active_peers, &cursor);
  if (xbt_dict_length(peer->active_peers) > 0) {
    key_choked = xbt_dict_cursor_get_key(cursor);
    peer_choked = xbt_dict_cursor_get_data(cursor);
  }
  xbt_dict_cursor_free(&cursor);

  /**If we are currently seeding, we unchoke the peer which has been unchoked the last time.*/
  if (peer->pieces == FILE_PIECES) {
    connection_t connection;
    double unchoke_time = MSG_get_clock() + 1;

    xbt_dict_foreach(peer->peers, cursor, key, connection) {
      if (connection->last_unchoke < unchoke_time && connection->interested && connection->choked_upload) {
        unchoke_time = connection->last_unchoke;
        peer_choosed = connection;
      }
    }
  } else {
    //Random optimistic unchoking
    if (peer->round == 0) {
      int j = 0;
      do {
        //We choose a random peer to unchoke.
        int id_chosen = RngStream_RandInt(peer->stream, 0, xbt_dict_length(peer->peers) - 1);
        int i = 0;
        connection_t connection;
        xbt_dict_foreach(peer->peers, cursor, key, connection) {
          if (i == id_chosen) {
            peer_choosed = connection;
            break;
          }
          i++;
        }
        xbt_dict_cursor_free(&cursor);
        if (!peer_choosed->interested || !peer_choosed->choked_upload) {
          peer_choosed = NULL;
        }
        j++;
      } while (peer_choosed == NULL && j < MAXIMUM_PAIRS);
    } else {
      //Use the "fastest download" policy.
      connection_t connection;
      double fastest_speed = 0.0;
      xbt_dict_foreach(peer->peers, cursor, key, connection) {
        if (connection->peer_speed > fastest_speed && connection->choked_upload && connection->interested) {
          peer_choosed = connection;
          fastest_speed = connection->peer_speed;
        }
      }
    }
  }

  if (peer_choosed != NULL)
    XBT_DEBUG("(%d) update_choked peers unchoked (%d) ; int (%d) ; choked (%d) ",
              peer->id, peer_choosed->id, peer_choosed->interested, peer_choosed->choked_upload);

  if (peer_choked != peer_choosed) {
    if (peer_choked != NULL) {
      xbt_assert((!peer_choked->choked_upload), "Tries to choked a choked peer");
      peer_choked->choked_upload = 1;
      xbt_assert((*((int *) key_choked) == peer_choked->id), "WTF !!!");
      update_active_peers_set(peer, peer_choked);
      XBT_DEBUG("(%d) Sending a CHOKE to %d", peer->id, peer_choked->id);
      send_choked(peer, peer_choked->mailbox);
    }
    if (peer_choosed != NULL) {
      xbt_assert((peer_choosed->choked_upload), "Tries to unchoked an unchoked peer");
      peer_choosed->choked_upload = 0;
      xbt_dict_set_ext(peer->active_peers, (char *) &peer_choosed->id, sizeof(int), peer_choosed, NULL);
      peer_choosed->last_unchoke = MSG_get_clock();
      XBT_DEBUG("(%d) Sending a UNCHOKE to %d", peer->id, peer_choosed->id);
      update_active_peers_set(peer, peer_choosed);
      send_unchoked(peer, peer_choosed->mailbox);
    }
  }
}

/** @brief Update "interested" state of peers: send "not interested" to peers that don't have any more pieces we want.
 *  @param peer our peer data
 */
void update_interested_after_receive(peer_t peer)
{
  char *key;
  xbt_dict_cursor_t cursor;
  connection_t connection;
  int interested;
  xbt_dict_foreach(peer->peers, cursor, key, connection) {
    interested = 0;
    if (connection->am_interested) {
      xbt_assert(connection->bitfield, "Bitfield not received");
      //Check if the peer still has a piece we want.
      int i;
      for (i = 0; i < FILE_PIECES; i++) {
        if (connection->bitfield[i] == '1' && peer->bitfield[i] == '0') {
          interested = 1;
          break;
        }
      }
      if (!interested) {        //no more piece to download from connection
        connection->am_interested = 0;
        send_notinterested(peer, connection->mailbox);
      }
    }
  }
}

void update_bitfield_blocks(peer_t peer, int index, int block_index, int block_length)
{
  int i;
  xbt_assert((index >= 0 && index <= FILE_PIECES), "Wrong piece.");
  xbt_assert((block_index >= 0 && block_index <= PIECES_BLOCKS), "Wrong block : %d.", block_index);
  for (i = block_index; i < (block_index + block_length); i++) {
    peer->bitfield_blocks[index * PIECES_BLOCKS + i] = '1';
  }
}

/** Returns if a peer has completed the download of a piece */
int piece_complete(peer_t peer, int index)
{
  int i;
  for (i = 0; i < PIECES_BLOCKS; i++) {
    if (peer->bitfield_blocks[index * PIECES_BLOCKS + i] == '0') {
      return 0;
    }
  }
  return 1;
}

/** Returns the first block that a peer doesn't have in a piece. If the peer has all blocks of the piece, returns -1. */
int get_first_block(peer_t peer, int piece)
{
  int i;
  for (i = 0; i < PIECES_BLOCKS; i++) {
    if (peer->bitfield_blocks[piece * PIECES_BLOCKS + i] == '0') {
      return i;
    }
  }
  return -1;
}

/** Indicates if the remote peer has a piece not stored by the local peer */
int is_interested(peer_t peer, connection_t remote_peer)
{
  xbt_assert(remote_peer->bitfield, "Bitfield not received");
  int i;
  for (i = 0; i < FILE_PIECES; i++) {
    if (remote_peer->bitfield[i] == '1' && peer->bitfield[i] == '0') {
      return 1;
    }
  }
  return 0;
}

/** Indicates if the remote peer has a piece not stored by the local peer nor requested by the local peer */
int is_interested_and_free(peer_t peer, connection_t remote_peer)
{
  xbt_assert(remote_peer->bitfield, "Bitfield not received");
  int i;
  for (i = 0; i < FILE_PIECES; i++) {
    if (remote_peer->bitfield[i] == '1' && peer->bitfield[i] == '0' && !in_current_pieces(peer, i)) {
      return 1;
    }
  }
  return 0;
}

/** Returns a piece that is partially downloaded and stored by the remote peer if any -1 otherwise. */
int partially_downloaded_piece(peer_t peer, connection_t remote_peer)
{
  xbt_assert(remote_peer->bitfield, "Bitfield not received");
  int i;
  for (i = 0; i < FILE_PIECES; i++) {
    if (remote_peer->bitfield[i] == '1' && peer->bitfield[i] == '0' && !in_current_pieces(peer, i)) {
      if (get_first_block(peer, i) > 0)
        return i;
    }
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
  int block_index, block_length;
  xbt_assert(remote_peer->bitfield, "bitfield not received");
  xbt_assert(remote_peer->bitfield[piece] == '1', "WTF !!!");
  block_index = get_first_block(peer, piece);
  if (block_index != -1) {
    block_length = PIECES_BLOCKS - block_index;
    block_length = MIN(BLOCKS_REQUESTED, block_length);
    send_request(peer, remote_peer->mailbox, piece, block_index, block_length);
  }
}

/** Indicates if a piece is currently being downloaded by the peer. */
int in_current_pieces(peer_t peer, int piece)
{
  unsigned i;
  int peer_piece;
  xbt_dynar_foreach(peer->current_pieces, i, peer_piece) {
    if (peer_piece == piece) {
      return 1;
    }
  }
  return 0;
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
void send_interested(peer_t peer, const char *mailbox)
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
void send_notinterested(peer_t peer, const char *mailbox)
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
  char *key;
  xbt_dict_foreach(peer->peers, cursor, key, remote_peer) {
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
void send_handshake(peer_t peer, const char *mailbox)
{
  msg_task_t task = task_message_new(MESSAGE_HANDSHAKE, peer->hostname, peer->mailbox, peer->id,
                                     task_message_size(MESSAGE_HANDSHAKE));
  MSG_task_dsend(task, mailbox, task_message_free);
  XBT_DEBUG("Sending a HANDSHAKE to %s", mailbox);
}

/** Send a "choked" message to a peer. */
void send_choked(peer_t peer, const char *mailbox)
{
  XBT_DEBUG("Sending a CHOKE to %s", mailbox);
  msg_task_t task = task_message_new(MESSAGE_CHOKE, peer->hostname, peer->mailbox, peer->id,
                    task_message_size(MESSAGE_CHOKE));
  MSG_task_dsend(task, mailbox, task_message_free);
}

/** Send a "unchoked" message to a peer */
void send_unchoked(peer_t peer, const char *mailbox)
{
  XBT_DEBUG("Sending a UNCHOKE to %s", mailbox);
  msg_task_t task = task_message_new(MESSAGE_UNCHOKE, peer->hostname, peer->mailbox, peer->id,
                                     task_message_size(MESSAGE_UNCHOKE));
  MSG_task_dsend(task, mailbox, task_message_free);
}

/** Send a "HAVE" message to all peers we are connected to */
void send_have(peer_t peer, int piece)
{
  XBT_DEBUG("Sending HAVE message to all my peers");
  connection_t remote_peer;
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  xbt_dict_foreach(peer->peers, cursor, key, remote_peer) {
    msg_task_t task = task_message_index_new(MESSAGE_HAVE, peer->hostname, peer->mailbox, peer->id, piece,
                                             task_message_size(MESSAGE_HAVE));
    MSG_task_dsend(task, remote_peer->mailbox, task_message_free);
  }
}

/** @brief Send a bitfield message to all the peers the peer has.
 *  @param peer peer data
 */
void send_bitfield(peer_t peer, const char *mailbox)
{
  XBT_DEBUG("Sending a BITFIELD to %s", mailbox);
  msg_task_t task = task_message_bitfield_new(peer->hostname, peer->mailbox, peer->id, peer->bitfield, FILE_PIECES);
  MSG_task_dsend(task, mailbox, task_message_free);
}

/** Send a "request" message to a pair, containing a request for a piece */
void send_request(peer_t peer, const char *mailbox, int piece, int block_index, int block_length)
{
  XBT_DEBUG("Sending a REQUEST to %s for piece %d (%d,%d)", mailbox, piece, block_index, block_length);
  msg_task_t task = task_message_request_new(peer->hostname, peer->mailbox, peer->id, piece, block_index, block_length);
  MSG_task_dsend(task, mailbox, task_message_free);
}

/** Send a "piece" message to a pair, containing a piece of the file */
void send_piece(peer_t peer, const char *mailbox, int piece, int block_index, int block_length)
{
  XBT_DEBUG("Sending the PIECE %d (%d,%d) to %s", piece, block_index, block_length, mailbox);
  xbt_assert(piece >= 0, "Tried to send a piece that doesn't exist.");
  xbt_assert((peer->bitfield[piece] == '1'), "Tried to send a piece that we doesn't have.");
  msg_task_t task = task_message_piece_new(peer->hostname, peer->mailbox, peer->id, piece, block_index, block_length,
                                           BLOCK_SIZE);
  MSG_task_dsend(task, mailbox, task_message_free);
}
