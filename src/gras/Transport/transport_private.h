/* $Id$ */

/* transport - low level communication (send/receive bunches of bytes)      */

/* module's private interface masked even to other parts of GRAS.           */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 Martin Quinson.                                       */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_TRP_PRIVATE_H
#define GRAS_TRP_PRIVATE_H

#include "gras_private.h"
#include "gras/Transport/transport_interface.h"
#include "gras/Virtu/virtu_interface.h" /* socketset_get() */

/**
 * s_gras_socket:
 * 
 * Description of a socket.
 */
typedef struct gras_trp_bufdata_ gras_trp_bufdata_t;

struct s_gras_socket  {
  gras_trp_plugin_t *plugin;
    
  int incoming :1; /* true if we can read from this sock */
  int outgoing :1; /* true if we can write on this sock */
  int accepting :1; /* true if master incoming sock in tcp */
  int raw :1; /* true if this is an experiment socket instead of messaging */
   
  int  sd; 
  int  port; /* port on this side */
  int  peer_port; /* port on the other side */
  char *peer_name; /* hostname of the other side */

  void *data;    /* plugin specific data */

  /* buffer plugin specific data. Yeah, C is not OO, so I got to trick */
  gras_trp_bufdata_t *bufdata; 
};
	
gras_error_t gras_trp_socket_new(int incomming,
				 gras_socket_t **dst);

/* The drivers */
typedef gras_error_t (*gras_trp_setup_t)(gras_trp_plugin_t *dst);

gras_error_t gras_trp_tcp_setup(gras_trp_plugin_t *plug);
gras_error_t gras_trp_file_setup(gras_trp_plugin_t *plug);
gras_error_t gras_trp_sg_setup(gras_trp_plugin_t *plug);
gras_error_t gras_trp_buf_setup(gras_trp_plugin_t *plug);

/*

  I'm tired of that shit. the select in SG has to create a socket to expeditor
  manually do deal with the weirdness of the hostdata, themselves here to deal
  with the weird channel concept of SG and convert them back to ports.
  
  When introducing buffered transport (whith I want to get used in SG to debug
  the buffering itself), we should not make the rest of the code aware of the
  change and not specify code for this. This is bad design.
  
  But there is bad design all over the place, so fuck off for now, when we can
  get rid of MSG and rely directly on SG, this crude hack can go away. But in
  the meanwhile, I want to sleep this night (FIXME).
  
  Hu! You evil problem! Taste my axe!

*/

gras_error_t gras_trp_buf_init_sock(gras_socket_t *sock);


#endif /* GRAS_TRP_PRIVATE_H */
