/* $Id$ */

/* trp (transport) - send/receive a bunch of bytes                          */

/* This file implements the public interface of this module, exported to the*/
/*  other modules of GRAS, but not to the end user.                         */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 Martin Quinson.                                       */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_TRANSPORT_H
#define GRAS_TRANSPORT_H

typedef struct s_gras_socket gras_socket_t;

/* A plugin type */
typedef struct gras_trp_plugin_ gras_trp_plugin_t;

/* Module, and get plugin by name */
gras_error_t gras_trp_init(void);

void         gras_trp_exit(void);

gras_error_t gras_socket_client(const char *host,
				unsigned short port,
				unsigned int bufSize,
				/* OUT */ gras_socket_t **dst);
gras_error_t gras_socket_server(unsigned short port,
				unsigned int bufSize,
				/* OUT */ gras_socket_t **dst);
void         gras_socket_close(gras_socket_t *sd);
 
   
gras_error_t gras_trp_bloc_send(gras_socket_t *sd,
				void *data,
				size_t size);
gras_error_t gras_trp_bloc_recv(gras_socket_t *sd,
				void *data,
				size_t size);

#endif /* GRAS_TRANSPORT_H */
