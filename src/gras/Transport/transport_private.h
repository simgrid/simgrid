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
   
  int incoming; /* true if incoming (server) sock, false if client sock */
  int accepting; /* true if master incoming sock in tcp */
   
  int  sd;
  int  port; /* port on this side */
  int  peer_port; /* port on the other side */
  char *peer_name; /* hostname of the other side */

  void *specific; /* plugin specific data */
};
	

/* TCP driver */
gras_error_t gras_trp_tcp_init(gras_trp_plugin_t **dst);

/* SG driver */
gras_error_t gras_trp_sg_init (gras_trp_plugin_t **dst);



#endif /* GRAS_TRP_PRIVATE_H */
