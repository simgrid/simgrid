/* messaging - high level communication (send/receive messages)             */

/* module's private interface masked even to other parts of GRAS.           */

/* Copyright (c) 2004, 2005, 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_MESSAGE_PRIVATE_H
#define GRAS_MESSAGE_PRIVATE_H

#include "portable.h"

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dynar.h"
#include "xbt/queue.h"
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
void
gras_msgtype_declare_ext(const char *name,
                         short int version,
                         e_gras_msg_kind_t kind,
                         gras_datadesc_type_t payload_request,
                         gras_datadesc_type_t payload_answer);

/**
 * gras_msgtype_t:
 *
 * Message type descriptor. There one of these for each registered version.
 */
typedef struct s_gras_msgtype {
  /* headers for the data set */
  unsigned int code;
  char *name;
  unsigned int name_len;

  /* payload */
  short int version;
  e_gras_msg_kind_t kind;
  gras_datadesc_type_t ctn_type;
  gras_datadesc_type_t answer_type;     /* only used for RPC */
} s_gras_msgtype_t;

extern xbt_set_t _gras_msgtype_set;     /* of gras_msgtype_t */
void gras_msgtype_free(void *msgtype);


/* functions to extract msg from socket or put it on wire (depend RL vs SG) */
gras_msg_t gras_msg_recv_any(void); /* Get first message arriving */
void gras_msg_recv(gras_socket_t sock, gras_msg_t msg /*OUT*/);
void gras_msg_send_ext(gras_socket_t sock,
                       e_gras_msg_kind_t kind,
                       unsigned long int ID,
                       gras_msgtype_t msgtype, void *payload);

/* The thread in charge of receiving messages and queuing them */
typedef struct s_gras_msg_listener_ *gras_msg_listener_t;
gras_msg_listener_t gras_msg_listener_launch(xbt_queue_t msg_exchange);
/* The caller has the responsability to cleanup the queues himself */
void gras_msg_listener_shutdown(void);

/**
 * gras_cblist_t:
 *
 * association between msg ID and cb list for a given process
 */
struct s_gras_cblist {
  long int id;
  xbt_dynar_t cbs;              /* of gras_msg_cb_t */
};

typedef struct s_gras_cblist gras_cblist_t;
void gras_cbl_free(void *);     /* used to free the memory at the end */
void gras_cblist_free(void *cbl);

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
  int answer_due;               /* Whether the callback is expected to return a result (for sanity checks) */
};
typedef struct s_gras_msg_cb_ctx s_gras_msg_cb_ctx_t;

/* ********* *
 * * TIMER * *
 * ********* */
typedef struct {
  double expiry;
  double period;
  void_f_void_t action;
  int repeat;
} s_gras_timer_t, *gras_timer_t;

/* returns 0 if it handled a timer, or the delay until next timer, or -1 if no armed timer */
double gras_msg_timer_handle(void);

gras_msg_cb_ctx_t gras_msg_cb_ctx_new(gras_socket_t expe,
                                      gras_msgtype_t msgtype,
                                      unsigned long int ID,
                                      int answer_due, double timeout);



/* We deploy a mallocator on the RPC contextes */
#include "xbt/mallocator.h"
extern xbt_mallocator_t gras_msg_ctx_mallocator;
void *gras_msg_ctx_mallocator_new_f(void);
void gras_msg_ctx_mallocator_free_f(void *dict);
void gras_msg_ctx_mallocator_reset_f(void *dict);


#endif /* GRAS_MESSAGE_PRIVATE_H */
