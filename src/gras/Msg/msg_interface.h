/* $Id$ */

/* messaging - high level communication (send/receive messages)             */

/* module's public interface exported within GRAS, but not to end user.     */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003, 2004 Martin Quinson.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_MSG_INTERFACE_H
#define GRAS_MSG_INTERFACE_H

/* gras_msg_t is dereferenced to be stored in procdata, living in Virtu */
typedef struct {
  gras_socket_t  *expeditor;
  gras_msgtype_t *type;
  void           *payload;
  int             payload_size;
} gras_msg_t;

gras_error_t gras_msg_send_namev(gras_socket_t *sock, 
				 const char    *namev, 
				 void          *payload);

#define GRAS_PROTOCOL_VERSION '\0';

typedef struct s_gras_cblist gras_cblist_t;
void gras_cbl_free(void *); /* virtu use that to free the memory at the end */
#endif  /* GRAS_MSG_INTERFACE_H */
