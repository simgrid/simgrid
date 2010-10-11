/* transport - low level communication (send/receive bunches of bytes)      */

/* module's private interface masked even to other parts of GRAS.           */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_TRP_PRIVATE_H
#define GRAS_TRP_PRIVATE_H

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"

#include "gras/emul.h"          /* gras_if_RL() */

#include "gras_modinter.h"      /* module init/exit */
#include "gras/transport.h"     /* rest of module interface */

#include "gras/Transport/transport_interface.h" /* semi-public API */
#include "gras/Virtu/virtu_interface.h" /* libdata management */

extern int gras_trp_libdata_id; /* our libdata identifier */

/* The function that select returned the last time we asked. We need this
   because the TCP read are greedy and try to get as much data in their 
   buffer as possible (to avoid subsequent syscalls).
   (measurement sockets are not buffered and thus not concerned).
  
   So, we can get more than one message in one shoot. And when this happens,
   we have to handle the same socket again afterward without select()ing at
   all. 
 
   Then, this data is not a static of the TCP driver because we want to
   zero it when it gets closed by the user. If not, we use an already freed 
   pointer, which is bad.
 
   It gets tricky since gras_socket_close is part of the common API, not 
   only the RL one. */
extern gras_socket_t _gras_lastly_selected_socket;

/**
 * s_gras_socket:
 * 
 * Description of a socket.
 */
typedef struct gras_trp_bufdata_ gras_trp_bufdata_t;

typedef struct s_gras_socket {
  gras_trp_plugin_t plugin;

  int incoming:1;               /* true if we can read from this sock */
  int outgoing:1;               /* true if we can write on this sock */
  int accepting:1;              /* true if master incoming sock in tcp */
  int meas:1;                   /* true if this is an experiment socket instead of messaging */
  int valid:1;                  /* false if a select returned that the peer quitted, forcing us to "close" the socket */
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

  unsigned long int buf_size;   /* what to say to the OS. 
                                   Field here to remember it when accepting */

  int sd;
  int port;                     /* port on this side */
  int peer_port;                /* port on the other side */
  char *peer_name;              /* hostname of the other side */
  char *peer_proc;              /* process on the other side */

  void *data;                   /* plugin specific data */

  /* buffer plugin specific data. Yeah, C is not OO, so I got to trick */
  gras_trp_bufdata_t *bufdata;
} s_gras_socket_t;

void gras_trp_socket_new(int incomming, gras_socket_t * dst);

/* The drivers */
typedef void (*gras_trp_setup_t) (gras_trp_plugin_t dst);

void gras_trp_tcp_setup(gras_trp_plugin_t plug);
void gras_trp_iov_setup(gras_trp_plugin_t plug);
void gras_trp_file_setup(gras_trp_plugin_t plug);
void gras_trp_sg_setup(gras_trp_plugin_t plug);

/* FIXME: this should be solved by SIMIX

  I'm tired of that shit. the select in SG has to create a socket to expeditor
  manually do deal with the weirdness of the hostdata, themselves here to deal
  with the weird channel concept of SG and convert them back to ports.
  
  When introducing buffered transport (which I want to get used in SG to debug
  the buffering itself), we should not make the rest of the code aware of the
  change and not specify code for this. This is bad design.
  
  But there is bad design all over the place, so fuck off for now, when we can
  get rid of MSG and rely directly on SG, this crude hack can go away. But in
  the meanwhile, I want to sleep this night (FIXME).
  
  Hu! You evil problem! Taste my axe!

*/

gras_socket_t gras_trp_buf_init_sock(gras_socket_t sock);

#endif                          /* GRAS_TRP_PRIVATE_H */
