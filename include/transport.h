/* $Id$ */

/* transport - low level communication (send/receive bunches of bytes)      */

/* module's public interface exported to end user.                          */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 Martin Quinson.                                       */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_TRANSPORT_H
#define GRAS_TRANSPORT_H

typedef struct s_gras_socket gras_socket_t;

gras_error_t gras_socket_client(const char *host,
				unsigned short port,
				/* OUT */ gras_socket_t **dst);
gras_error_t gras_socket_server(unsigned short port,
				/* OUT */ gras_socket_t **dst);
void         gras_socket_close(gras_socket_t **sd);


/* debuging functions */
gras_error_t gras_socket_client_from_file(const char*path,
					  /* OUT */ gras_socket_t **dst);
gras_error_t gras_socket_server_from_file(const char*path,
					  /* OUT */ gras_socket_t **dst);
					  
   
#endif /* GRAS_TRANSPORT_H */
