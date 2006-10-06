/* $Id$ */

/* messaging - high level communication (send/receive messages)             */

/* module's public interface exported within GRAS, but not to end user.     */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_MSG_INTERFACE_H
#define GRAS_MSG_INTERFACE_H

#include "gras/transport.h"

/*
 * Data of this module specific to each process 
 * (used by sg_process.c to check some usual errors at the end of the simulation)
 * FIXME: it could be cleaned up ?
 */
typedef struct {
  /* set headers */
  unsigned int ID;
  char        *name;
  unsigned int name_len;

  /*queue of msgs storing the ones got while msg_wait'ing for something else */
  xbt_dynar_t msg_queue; /* elm type: s_gras_msg_t */

  /* registered callbacks for each message */
  xbt_dynar_t cbl_list; /* elm type: gras_cblist_t */
   
  /* registered timers */
  xbt_dynar_t timers; /* elm type: s_gras_timer_t */

} s_gras_msg_procdata_t,*gras_msg_procdata_t;


XBT_PUBLIC void gras_msg_send_namev(gras_socket_t  sock, 
			 const char    *namev, 
			 void          *payload);

#define GRAS_PROTOCOL_VERSION '\0';


#endif  /* GRAS_MSG_INTERFACE_H */
