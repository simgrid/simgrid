/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "connection.h"
#include "bittorrent.h"
#include <xbt/sysdep.h>
XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(msg_peers);

connection_t connection_new(int id)
{
  connection_t connection = xbt_new(s_connection_t, 1);

  connection->id = id;
  connection->mailbox = bprintf("%d", id);
  connection->bitfield = NULL;
  connection->current_piece = -1;
  connection->interested = 0;
  connection->am_interested = 0;
  connection->choked_upload = 1;
  connection->choked_download = 1;
  connection->peer_speed = 0;
  connection->last_unchoke = 0;

  return connection;
}

void connection_add_speed_value(connection_t connection, double speed)
{
  connection->peer_speed = connection->peer_speed * 0.6 + speed * 0.4;
}

void connection_free(void *data)
{
  connection_t co = (connection_t) data;
  xbt_free(co->bitfield);
  xbt_free(co->mailbox);
  xbt_free(co);
}
