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
#include "Transport/transport_interface.h"

extern gras_dynar_t *_gras_trp_sockets; /* all existing sockets */


/**
 * s_gras_socket:
 * 
 * Description of a socket.
 */

struct s_gras_socket  {
  gras_trp_plugin_t *plugin;
    
  int incoming :1; /* true if we can read from this sock */
  int outgoing :1; /* true if we can write on this sock */
  int accepting :1; /* true if master incoming sock in tcp */
   
  int  sd; 
  int  port; /* port on this side */
  int  peer_port; /* port on the other side */
  char *peer_name; /* hostname of the other side */

  void *specific; /* plugin specific data */
};
	
gras_error_t gras_trp_socket_new(int incomming,
				 gras_socket_t **dst);

/* The drivers */
gras_error_t gras_trp_tcp_init(gras_trp_plugin_t **dst);
gras_error_t gras_trp_file_init(gras_trp_plugin_t **dst);
gras_error_t gras_trp_sg_init (gras_trp_plugin_t **dst);



#endif /* GRAS_TRP_PRIVATE_H */
