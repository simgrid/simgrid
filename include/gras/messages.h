/* $Id$ */

/* messaging - high level communication (send/receive messages)             */

/* module's public interface exported to end user.                          */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003, 2004 Martin Quinson.                                 */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef GRAS_MESSAGES_H
#define GRAS_MESSAGES_H

#include "xbt/misc.h"
#include "gras/transport.h"
#include "gras/datadesc.h"

BEGIN_DECL

/* msgtype declaration and retrival */
typedef struct s_gras_msgtype gras_msgtype_t;

void gras_msgtype_declare  (const char            *name,
			    gras_datadesc_type_t  *payload);
void gras_msgtype_declare_v(const char            *name,
			    short int              version,
			    gras_datadesc_type_t  *payload);

gras_msgtype_t *gras_msgtype_by_name (const char     *name);
gras_msgtype_t *gras_msgtype_by_namev(const char      *name,
				      short int        version);

/**
 * gras_cb_t:
 * @msg: The message itself
 * @Returns: true if the message was consumed by the callback.
 *
 * Type of message callback functions. Once a such a function is registered to 
 * handle messages of a given type with RegisterCallback(), it will be called 
 * each time such a message incomes.
 *
 * If the callback accepts the message, it should free it after use.
 */
typedef int (*gras_cb_t)(gras_socket_t        *expeditor,
			 void                 *payload);
void gras_cb_register  (gras_msgtype_t *msgtype,
			gras_cb_t       cb);
void gras_cb_unregister(gras_msgtype_t *msgtype,
			gras_cb_t     cb);

gras_error_t gras_msg_send(gras_socket_t  *sock,
			   gras_msgtype_t *msgtype,
			   void           *payload);
gras_error_t gras_msg_wait(double                 timeout,    
			   gras_msgtype_t        *msgt_want,
			   gras_socket_t        **expeditor,
			   void                  *payload);
gras_error_t gras_msg_handle(double timeOut);


END_DECL

#endif /* GRAS_MSG_H */

