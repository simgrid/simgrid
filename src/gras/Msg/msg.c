/* $Id$ */

/* messaging - Function related to messaging (code shared between RL and SG)*/

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "gras/Msg/msg_private.h"
#include "gras/DataDesc/datadesc_interface.h"
#include "gras/Transport/transport_interface.h" /* gras_trp_chunk_send/recv */
#include "gras/Virtu/virtu_interface.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_msg,gras,"High level messaging");

xbt_set_t _gras_msgtype_set = NULL;
static char GRAS_header[6];
static char *make_namev(const char *name, short int ver);

/*
 * Creating procdata for this module
 */
static void *gras_msg_procdata_new() {
   gras_msg_procdata_t res = xbt_new(s_gras_msg_procdata_t,1);
   
   res->msg_queue = xbt_dynar_new(sizeof(s_gras_msg_t),   NULL);
   res->cbl_list  = xbt_dynar_new(sizeof(gras_cblist_t *),gras_cbl_free);
   res->timers    = xbt_dynar_new(sizeof(s_gras_timer_t), NULL);
   
   return (void*)res;
}

/*
 * Freeing procdata for this module
 */
static void gras_msg_procdata_free(void *data) {
   gras_msg_procdata_t res = (gras_msg_procdata_t)data;
   
   xbt_dynar_free(&( res->msg_queue ));
   xbt_dynar_free(&( res->cbl_list ));
   xbt_dynar_free(&( res->timers ));
   
   free(res);
}

/*
 * Module registration
 */
void gras_msg_register() {
   gras_procdata_add("gras_msg",gras_msg_procdata_new, gras_msg_procdata_free);
}

/*
 * Initialize this submodule.
 */
void gras_msg_init(void) {
  /* only initialize once */
  if (_gras_msgtype_set != NULL)
    return;

  VERB0("Initializing Msg");
  
  _gras_msgtype_set = xbt_set_new();

  memcpy(GRAS_header,"GRAS", 4);
  GRAS_header[4]=GRAS_PROTOCOL_VERSION;
  GRAS_header[5]=(char)GRAS_THISARCH;
}

/*
 * Finalize the msg module
 */
void
gras_msg_exit(void) {
  VERB0("Exiting Msg");
  xbt_set_free(&_gras_msgtype_set);
}

/*
 * Reclamed memory
 */
void gras_msgtype_free(void *t) {
  gras_msgtype_t msgtype=(gras_msgtype_t)t;
  if (msgtype) {
    free(msgtype->name);
    free(msgtype);
  }
}

/**
 * make_namev:
 *
 * Returns the versionned name of the message. If the version is 0, that's 
 * the name unchanged. Pay attention to this before free'ing the result.
 */
static char *make_namev(const char *name, short int ver) {
  char *namev;

  if (!ver)
    return (char *)name;

  namev = (char*)xbt_malloc(strlen(name)+2+3+1);

  if (namev)
      sprintf(namev,"%s_v%d",name,ver);

  return namev;
}

/** @brief declare a new message type of the given name. It only accepts the given datadesc as payload
 *
 * @param name: name as it should be used for logging messages (must be uniq)
 * @param payload: datadescription of the payload
 */
void gras_msgtype_declare(const char           *name,
			  gras_datadesc_type_t  payload) {
   gras_msgtype_declare_v(name, 0, payload);
}

/** @brief declare a new versionned message type of the given name and payload
 *
 * @param name: name as it should be used for logging messages (must be uniq)
 * @param version: something like versionning symbol
 * @param payload: datadescription of the payload
 *
 * Registers a message to the GRAS mechanism. Use this version instead of 
 * gras_msgtype_declare when you change the semantic or syntax of a message and
 * want your programs to be able to deal with both versions. Internally, each
 * will be handled as an independent message type, so you can register 
 * differents for each of them.
 */
void
gras_msgtype_declare_v(const char           *name,
		       short int             version,
		       gras_datadesc_type_t  payload) {
 
  xbt_error_t   errcode;
  gras_msgtype_t msgtype;
  char *namev=make_namev(name,version);
      
  errcode = xbt_set_get_by_name(_gras_msgtype_set,
				 namev,(xbt_set_elm_t*)&msgtype);

  if (errcode == no_error) {
    VERB2("Re-register version %d of message '%s' (same payload, ignored).",
	  version, name);
    xbt_assert3(!gras_datadesc_type_cmp(msgtype->ctn_type, payload),
		 "Message %s re-registred with another payload (%s was %s)",
		 namev,gras_datadesc_get_name(payload),
		 gras_datadesc_get_name(msgtype->ctn_type));

    return ; /* do really ignore it */

  }
  xbt_assert_error(mismatch_error); /* expect this error */
  VERB3("Register version %d of message '%s' (payload: %s).", 
	version, name, gras_datadesc_get_name(payload));    

  msgtype = xbt_new(s_gras_msgtype_t,1);
  msgtype->name = (namev == name ? strdup(name) : namev);
  msgtype->name_len = strlen(namev);
  msgtype->version = version;
  msgtype->ctn_type = payload;

  xbt_set_add(_gras_msgtype_set, (xbt_set_elm_t)msgtype,
	       &gras_msgtype_free);
}

/** @brief retrive an existing message type from its name. */
gras_msgtype_t gras_msgtype_by_name (const char *name) {
  return gras_msgtype_by_namev(name,0);
}

/** @brief retrive an existing message type from its name and version. */
gras_msgtype_t gras_msgtype_by_namev(const char      *name,
				     short int        version) {
  gras_msgtype_t res;

  xbt_error_t errcode;
  char *namev = make_namev(name,version); 

  errcode = xbt_set_get_by_name(_gras_msgtype_set, namev,
				 (xbt_set_elm_t*)&res);
  if (errcode != no_error)
    res = NULL;
  if (!res) 
     WARN1("msgtype_by_name(%s) returns NULL",namev);
  if (name != namev) 
    free(namev);
  
  return res;
}

/** \brief Send the data pointed by \a payload as a message of type
 * \a msgtype to the peer \a sock */
xbt_error_t
gras_msg_send(gras_socket_t   sock,
	      gras_msgtype_t  msgtype,
	      void           *payload) {

  xbt_error_t errcode;
  static gras_datadesc_type_t string_type=NULL;

  if (!msgtype)
    RAISE0(mismatch_error,
	   "Cannot send the NULL message (did msgtype_by_name fail?)");

  if (!string_type) {
    string_type = gras_datadesc_by_name("string");
    xbt_assert(string_type);
  }

  DEBUG3("send '%s' to %s:%d", msgtype->name, 
	 gras_socket_peer_name(sock),gras_socket_peer_port(sock));
  TRY(gras_trp_chunk_send(sock, GRAS_header, 6));

  TRY(gras_datadesc_send(sock, string_type,   &msgtype->name));
  if (msgtype->ctn_type)
    TRY(gras_datadesc_send(sock, msgtype->ctn_type, payload));
  TRY(gras_trp_flush(sock));

  return no_error;
}
/*
 * receive the next message on the given socket.  
 */
xbt_error_t
gras_msg_recv(gras_socket_t    sock,
	      gras_msgtype_t  *msgtype,
	      void           **payload,
	      int             *payload_size) {

  xbt_error_t errcode;
  static gras_datadesc_type_t string_type=NULL;
  char header[6];
  int cpt;
  int r_arch;
  char *msg_name=NULL;

  if (!string_type) {
    string_type=gras_datadesc_by_name("string");
    xbt_assert(string_type);
  }
  
  TRY(gras_trp_chunk_recv(sock, header, 6));
  for (cpt=0; cpt<4; cpt++)
    if (header[cpt] != GRAS_header[cpt])
      RAISE2(mismatch_error,"Incoming bytes do not look like a GRAS message (header='%.4s' not '%.4s')",header,GRAS_header);
  if (header[4] != GRAS_header[4]) 
    RAISE2(mismatch_error,"GRAS protocol mismatch (got %d, use %d)",
	   (int)header[4], (int)GRAS_header[4]);
  r_arch = (int)header[5];
  DEBUG2("Handle an incoming message using protocol %d (remote is %s)",
	 (int)header[4],gras_datadesc_arch_name(r_arch));

  TRY(gras_datadesc_recv(sock, string_type, r_arch, &msg_name));
  errcode = xbt_set_get_by_name(_gras_msgtype_set,
				 msg_name,(xbt_set_elm_t*)msgtype);
  if (errcode != no_error)
    RAISE2(errcode,
	   "Got error %s while retrieving the type associated to messages '%s'",
	   xbt_error_name(errcode),msg_name);
  /* FIXME: Survive unknown messages */
  free(msg_name);

  if ((*msgtype)->ctn_type) {
    *payload_size=gras_datadesc_size((*msgtype)->ctn_type);
    xbt_assert2(*payload_size > 0,
		"%s %s",
		"Dynamic array as payload is forbided for now (FIXME?).",
		"Reference to dynamic array is allowed.");
    *payload = xbt_malloc(*payload_size);
    TRY(gras_datadesc_recv(sock, (*msgtype)->ctn_type, r_arch, *payload));
  } else {
    *payload = NULL;
    *payload_size = 0;
  }
  return no_error;
}

/** \brief Waits for a message to come in over a given socket. 
 *
 * @param timeout: How long should we wait for this message.
 * @param msgt_want: type of awaited msg
 * @param[out] expeditor: where to create a socket to answer the incomming message
 * @param[out] payload: where to write the payload of the incomming message
 * @return the error code (or no_error).
 *
 * Every message of another type received before the one waited will be queued
 * and used by subsequent call to this function or gras_msg_handle().
 */
xbt_error_t
gras_msg_wait(double           timeout,    
	      gras_msgtype_t   msgt_want,
	      gras_socket_t   *expeditor,
	      void            *payload) {

  gras_msgtype_t msgt_got;
  void *payload_got;
  int payload_size_got;
  xbt_error_t errcode;
  double start, now;
  gras_msg_procdata_t pd=(gras_msg_procdata_t)gras_libdata_get("gras_msg");
  int cpt;
  s_gras_msg_t msg;
  gras_socket_t expeditor_res = NULL;
  
  payload_got = NULL;

  if (!msgt_want)
    RAISE0(mismatch_error,
	   "Cannot wait for the NULL message (did msgtype_by_name fail?)");

  VERB1("Waiting for message '%s'",msgt_want->name);

  start = now = gras_os_time();

  xbt_dynar_foreach(pd->msg_queue,cpt,msg){
    if (msg.type->code == msgt_want->code) {
      if (expeditor)
        *expeditor = msg.expeditor;
      memcpy(payload, msg.payload, msg.payload_size);
      free(msg.payload);
      xbt_dynar_cursor_rm(pd->msg_queue, &cpt);
      VERB0("The waited message was queued");
      return no_error;
    }
  }

  while (1) {
    TRY(gras_trp_select(timeout - now + start, &expeditor_res));
    TRY(gras_msg_recv(expeditor_res, &msgt_got, &payload_got, &payload_size_got));
    if (msgt_got->code == msgt_want->code) {
      if (expeditor)
      	*expeditor=expeditor_res;
      memcpy(payload, payload_got, payload_size_got);
      free(payload_got);
      VERB0("Got waited message");
      return no_error;
    }

    /* not expected msg type. Queue it for later */
    msg.expeditor = expeditor_res;
    msg.type      = msgt_got;
    msg.payload   = payload;
    msg.payload_size = payload_size_got;
    xbt_dynar_push(pd->msg_queue,&msg);
    
    now=gras_os_time();
    if (now - start + 0.001 < timeout) {
      RAISE1(timeout_error,"Timeout while waiting for msg %s",msgt_want->name);
    }
  }

  RAISE_IMPOSSIBLE;
}

/** @brief Handle an incomming message or timer (or wait up to \a timeOut seconds)
 *
 * @param timeOut: How long to wait for incoming messages (in seconds)
 * @return the error code (or no_error).
 *
 * Messages are passed to the callbacks.
 */
xbt_error_t 
gras_msg_handle(double timeOut) {
  
  double          untiltimer;
   
  xbt_error_t    errcode;
  int             cpt;

  s_gras_msg_t    msg;
  gras_socket_t   expeditor;
  void           *payload=NULL;
  int             payload_size;
  gras_msgtype_t  msgtype;

  gras_msg_procdata_t pd=(gras_msg_procdata_t)gras_libdata_get("gras_msg");
  gras_cblist_t  *list=NULL;
  gras_msg_cb_t       cb;
   
  int timerexpected;

  VERB1("Handling message within the next %.2fs",timeOut);
  
  untiltimer = gras_msg_timer_handle();
  DEBUG2("[%.0f] Next timer in %f sec", gras_os_time(), untiltimer);
  if (untiltimer == 0.0) {
     /* A timer was already elapsed and handled */
     return no_error;
  }
  if (untiltimer != -1.0) {
     timerexpected = 1;
     timeOut = MIN(timeOut, untiltimer);
  } else {
     timerexpected = 0;
  }
   
  /* get a message (from the queue or from the net) */
  if (xbt_dynar_length(pd->msg_queue)) {
    DEBUG0("Get a message from the queue");
    xbt_dynar_shift(pd->msg_queue,&msg);
    expeditor = msg.expeditor;
    msgtype   = msg.type;
    payload   = msg.payload;
    errcode   = no_error;
  } else {
    errcode = gras_trp_select(timeOut, &expeditor);
    if (errcode != no_error && errcode != timeout_error)
       return errcode;
    if (errcode != timeout_error)
       TRY(gras_msg_recv(expeditor, &msgtype, &payload, &payload_size));
  }

  if (errcode == timeout_error ) {
     if (timerexpected) {
	  
	/* A timer elapsed before the arrival of any message even if we select()ed a bit */
	untiltimer = gras_msg_timer_handle();
	if (untiltimer == 0.0) {
	   return no_error;
	} else {
	   xbt_assert1(untiltimer>0, "Negative timer (%f). I'm puzzeled", untiltimer);
	   ERROR1("No timer elapsed, in contrary to expectations (next in %f sec)",
		  untiltimer);
	   return timeout_error;
	}
	
     } else {
	/* select timeouted, and no timer elapsed. Nothing to do */
	return timeout_error;
     }
     
  }
   
  /* A message was already there or arrived in the meanwhile. handle it */
  xbt_dynar_foreach(pd->cbl_list,cpt,list) {
    if (list->id == msgtype->code) {
      break;
    } else {
      list=NULL;
    }
  }
  if (!list) {
    INFO1("No callback for the incomming '%s' message. Discarded.", 
	  msgtype->name);
    WARN0("FIXME: gras_datadesc_free not implemented => leaking the payload");
    return no_error;
  }
  
  xbt_dynar_foreach(list->cbs,cpt,cb) { 
    VERB3("Use the callback #%d (@%p) for incomming msg %s",
          cpt+1,cb,msgtype->name);
    if ((*cb)(expeditor,payload)) {
      /* cb handled the message */
      free(payload);
      return no_error;
    }
  }

  INFO1("Message '%s' refused by all registered callbacks", msgtype->name);
  WARN0("FIXME: gras_datadesc_free not implemented => leaking the payload");
  return mismatch_error;
}

void
gras_cbl_free(void *data){
  gras_cblist_t *list=*(void**)data;
  if (list) {
    xbt_dynar_free(&( list->cbs ));
    free(list);
  }
}

/** \brief Bind the given callback to the given message type 
 *
 * Several callbacks can be attached to a given message type. The lastly added one will get the message first, and 
 * if it returns false, the message will be passed to the second one. 
 * And so on until one of the callbacks accepts the message.
 */
void
gras_cb_register(gras_msgtype_t msgtype,
		 gras_msg_cb_t cb) {
  gras_msg_procdata_t pd=(gras_msg_procdata_t)gras_libdata_get("gras_msg");
  gras_cblist_t *list=NULL;
  int cpt;

  DEBUG2("Register %p as callback to '%s'",cb,msgtype->name);

  /* search the list of cb for this message on this host (creating if NULL) */
  xbt_dynar_foreach(pd->cbl_list,cpt,list) {
    if (list->id == msgtype->code) {
      break;
    } else {
      list=NULL;
    }
  }
  if (!list) {
    /* First cb? Create room */
    list = xbt_new(gras_cblist_t,1);
    list->id = msgtype->code;
    list->cbs = xbt_dynar_new(sizeof(gras_msg_cb_t), NULL);
    xbt_dynar_push(pd->cbl_list,&list);
  }

  /* Insert the new one into the set */
  xbt_dynar_insert_at(list->cbs,0,&cb);
}

/** \brief Unbind the given callback from the given message type */
void
gras_cb_unregister(gras_msgtype_t msgtype,
		   gras_msg_cb_t cb) {

  gras_msg_procdata_t pd=(gras_msg_procdata_t)gras_libdata_get("gras_msg");
  gras_cblist_t *list;
  gras_msg_cb_t cb_cpt;
  int cpt;
  int found = 0;

  /* search the list of cb for this message on this host */
  xbt_dynar_foreach(pd->cbl_list,cpt,list) {
    if (list->id == msgtype->code) {
      break;
    } else {
      list=NULL;
    }
  }

  /* Remove it from the set */
  if (list) {
    xbt_dynar_foreach(list->cbs,cpt,cb_cpt) {
      if (cb == cb_cpt) {
	xbt_dynar_cursor_rm(list->cbs, &cpt);
	found = 1;
      }
    }
  }
  if (!found)
    VERB1("Ignoring removal of unexisting callback to msg id %d",
	  msgtype->code);
}
