/* $Id$ */

/* messaging - high level communication (send/receive messages)             */

/* module's private interface masked even to other parts of GRAS.           */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003, 2004 Martin Quinson.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_MESSAGE_PRIVATE_H
#define GRAS_MESSAGE_PRIVATE_H

#include "gras_private.h"
#include "Msg/msg_interface.h"

/**
 * gras_msgtype_t:
 *
 * Message type descriptor. There one of these for each registered version.
 */
struct s_gras_msgtype {
  /* headers for the data set */
  unsigned int   code;
  char          *name;
  unsigned int   name_len;
        
  /* payload */
  short int version;
  gras_datadesc_type_t *ctn_type;
};

extern gras_set_t *_gras_msgtype_set; /* of gras_msgtype_t */
void gras_msgtype_free(void *msgtype);


gras_error_t gras_msg_recv(gras_socket_t   *sock,
			   gras_msgtype_t **msgtype,
			   void           **payload);
gras_error_t
gras_msg_recv_no_malloc(gras_socket_t   *sock,
			gras_msgtype_t **msgtype,
			void           **payload);


/**
 * gras_cblist_t:
 *
 * association between msg ID and cb list for a given process
 */
struct s_gras_cblist {
  long int id;
  gras_dynar_t *cbs; /* of gras_msg_cb_t */
};

void gras_cblist_free(void *cbl);

#endif  /* GRAS_MESSAGE_PRIVATE_H */
