/* Copyright (c) 2012-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef BITTORRENT_CONNECTION_H_
#define BITTORRENT_CONNECTION_H_

/**  Contains the connection data of a peer. */
typedef struct s_connection {
  int id;                // Peer id
  unsigned int bitfield; // Fields
  char* mailbox;
  int messages_count;
  double peer_speed;
  double last_unchoke;
  int current_piece;
  unsigned int am_interested : 1;   // Indicates if we are interested in something the peer has
  unsigned int interested : 1;      // Indicates if the peer is interested in one of our pieces
  unsigned int choked_upload : 1;   // Indicates if the peer is choked for the current peer
  unsigned int choked_download : 1; // Indicates if the peer has choked the current peer
} s_connection_t;

typedef s_connection_t* connection_t;

/** @brief Build a new connection object from the peer id.
 *  @param id id of the peer
 */
connection_t connection_new(int id);
/** @brief Add a new value to the peer speed average
 *  @param connection connection data
 *  @param speed speed to add to the speed average
 */
void connection_add_speed_value(connection_t connection, double speed);
/** Frees a connection object */
void connection_free(void* data);
int connection_has_piece(const s_connection_t* connection, unsigned int piece);
#endif /* BITTORRENT_CONNECTION_H_ */
