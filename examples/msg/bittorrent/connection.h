/* Copyright (c) 2012-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef BITTORRENT_CONNECTION_H_
#define BITTORRENT_CONNECTION_H_

/**  Contains the connection data of a peer. */
typedef struct s_connection {
  int id;                       //Peer id
  char *bitfield;               //Fields
  char *mailbox;
  int messages_count;
  double peer_speed;
  double last_unchoke;
  int current_piece;
  int am_interested:1;          //Indicates if we are interested in something the peer has
  int interested:1;             //Indicates if the peer is interested in one of our pieces
  int choked_upload:1;          //Indicates if the peer is choked for the current peer
  int choked_download:1;        //Indicates if the peer has choked the current peer
} s_connection_t, *connection_t;

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
void connection_free(void *data);
#endif                          /* BITTORRENT_CONNECTION_H_ */
