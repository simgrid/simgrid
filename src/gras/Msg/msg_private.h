/* $Id$ */

/* messaging - high level communication (send/receive messages)             */

/* module's private interface masked even to other parts of GRAS.           */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_MESSAGE_PRIVATE_H
#define GRAS_MESSAGE_PRIVATE_H

#include "portable.h"

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dynar.h"
#include "xbt/set.h"
#include "gras/transport.h"
#include "gras/datadesc.h"
#include "gras/virtu.h"

#include "gras/messages.h"
#include "gras/timer.h"
#include "gras_modinter.h"

#include "gras/Msg/msg_interface.h"

extern char _GRAS_header[6];

extern int gras_msg_libdata_id; /* The identifier of our libdata */
 
extern const char *e_gras_msg_kind_names[e_gras_msg_kind_count];

/* declare either regular messages or RPC or whatever */
XBT_PUBLIC void 
gras_msgtype_declare_ext(const char           *name,
			 short int             version,
			 e_gras_msg_kind_t     kind, 
			 gras_datadesc_type_t  payload_request,
			 gras_datadesc_type_t  payload_answer);

/**
 * gras_msgtype_t:
 *
 * Message type descriptor. There one of these for each registered version.
 */
typedef struct s_gras_msgtype {
  /* headers for the data set */
  unsigned int   code;
  char          *name;
  unsigned int   name_len;
        
  /* payload */
  short int version;
  e_gras_msg_kind_t kind;
  gras_datadesc_type_t ctn_type;
  gras_datadesc_type_t answer_type; /* only used for RPC */
} s_gras_msgtype_t;

extern xbt_set_t _gras_msgtype_set; /* of gras_msgtype_t */
void gras_msgtype_free(void *msgtype);


/* functions to extract msg from socket or put it on wire (depend RL vs SG) */
XBT_PUBLIC void gras_msg_recv(gras_socket_t   sock,
		   gras_msg_t      msg/*OUT*/);
XBT_PUBLIC void gras_msg_send_ext(gras_socket_t   sock,
		     e_gras_msg_kind_t kind,
		       unsigned long int ID,
		       gras_msgtype_t  msgtype,
		       void           *payload);

/**
 * gras_cblist_t:
 *
 * association between msg ID and cb list for a given process
 */
struct s_gras_cblist {
  long int id;
  xbt_dynar_t cbs; /* of gras_msg_cb_t */
};

typedef struct s_gras_cblist gras_cblist_t;
XBT_PUBLIC void gras_cbl_free(void *); /* used to free the memory at the end */
XBT_PUBLIC void gras_cblist_free(void *cbl);

/**
 * gras_msg_cb_ctx_t:
 *
 * Context associated to a given callback (to either regular message or RPC)
 */
struct s_gras_msg_cb_ctx {
  gras_socket_t expeditor;
  gras_msgtype_t msgtype;
  unsigned long int ID;
  double timeout;
};
typedef struct s_gras_msg_cb_ctx s_gras_msg_cb_ctx_t;

/* ********* *
 * * TIMER * *
 * ********* */
typedef struct {
  double expiry;
  double period;
  void_f_void_t *action;
  int repeat;
} s_gras_timer_t, *gras_timer_t;

/* returns 0 if it handled a timer, or the delay until next timer, or -1 if no armed timer */
XBT_PUBLIC double gras_msg_timer_handle(void);



#endif  /* GRAS_MESSAGE_PRIVATE_H */
