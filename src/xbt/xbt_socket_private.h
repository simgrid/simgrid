/* transport - low level communication (send/receive bunches of bytes)      */

/* module's private interface masked even to other parts of XBT.           */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_SOCKET_PRIVATE_H
#define XBT_SOCKET_PRIVATE_H

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"
#include "xbt/socket.h"

/**
 * s_xbt_trp_bufdata:
 * 
 * Description of a socket.
 */
typedef struct s_xbt_trp_bufdata xbt_trp_bufdata_t;

typedef struct s_xbt_socket {

  xbt_trp_plugin_t plugin;

  int incoming:1;               /* true if we can read from this sock */
  int outgoing:1;               /* true if we can write on this sock */
  int accepting:1;              /* true if master incoming sock in tcp */
  int meas:1;                   /* true if this is an experiment socket instead of messaging */
  int valid:1;                  /* false if a select returned that the peer has left, forcing us to "close" the socket */
  int moredata:1;               /* TCP socket use a buffer and read operation get as much 
                                   data as possible. It is possible that several messages
                                   are received in one shoot, and select won't catch them 
                                   afterward again. 
                                   This boolean indicates that this is the case, so that we
                                   don't call select in that case.  Note that measurement
                                   sockets are not concerned since they use the TCP
                                   interface directly, with no buffer. */

  int recvd:1;                  /* true if the recvd_val field contains one byte of the stream (that we peek'ed to check the socket validity) */
  char recvd_val;               /* what we peeked from the socket, if any */

  int refcount;                 /* refcounting on shared sockets */

  unsigned long int buf_size;   /* what to say to the OS. 
                                   Field here to remember it when accepting */

  int sd;

  void *data;                   /* userdata */
  xbt_trp_bufdata_t *bufdata;   /* buffer userdata */
} s_xbt_socket_t;

void xbt_trp_tcp_setup(xbt_trp_plugin_t plug);

#endif                          /* XBT_SOCKET_PRIVATE_H */
