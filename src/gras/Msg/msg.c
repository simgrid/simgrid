/* $Id$ */

/* messaging - Function related to messaging (code shared between RL and SG)*/

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras/Msg/msg_private.h"
#include "gras/DataDesc/datadesc_interface.h"
#include "gras/Transport/transport_interface.h" /* gras_trp_chunk_send/recv */
#include "gras/Virtu/virtu_interface.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg,gras,"High level messaging");

xbt_set_t _gras_msgtype_set = NULL;
static char GRAS_header[6];
static char *make_namev(const char *name, short int ver);

/**
 * gras_msg_init:
 *
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

/**
 * gras_msg_exit:
 *
 * Finalize the msg module
 **/
void
gras_msg_exit(void) {
  VERB0("Exiting Msg");
  xbt_set_free(&_gras_msgtype_set);
}

/**
 * gras_msgtype_free:
 *
 * Reclamed memory
 */
void gras_msgtype_free(void *t) {
  gras_msgtype_t msgtype=(gras_msgtype_t)t;
  if (msgtype) {
    xbt_free(msgtype->name);
    xbt_free(msgtype);
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

/**
 * gras_msgtype_declare:
 * @name: name as it should be used for logging messages (must be uniq)
 * @payload: datadescription of the payload
 *
 * Registers a message to the GRAS mecanism.
 */
void gras_msgtype_declare(const char           *name,
			  gras_datadesc_type_t  payload) {
   gras_msgtype_declare_v(name, 0, payload);
}

/**
 * gras_msgtype_declare_v:
 * @name: name as it should be used for logging messages (must be uniq)
 * @version: something like versionning symbol
 * @payload: datadescription of the payload
 *
 * Registers a message to the GRAS mecanism. Use this version instead of 
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

/**
 * gras_msgtype_by_name:
 *
 * Retrieve a datatype description from its name
 */
gras_msgtype_t gras_msgtype_by_name (const char *name) {
  return gras_msgtype_by_namev(name,0);
}
/**
 * gras_msgtype_by_namev:
 *
 * Retrieve a datatype description from its name and version
 */
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
    xbt_free(namev);
  
  return res;
}

/**
 * gras_msg_send:
 *
 * Send the given message on the given socket 
 */
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
  TRY(gras_datadesc_send(sock, msgtype->ctn_type, payload));
  TRY(gras_trp_flush(sock));

  return no_error;
}
/**
 * gras_msg_recv:
 *
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
      RAISE0(mismatch_error,"Incoming bytes do not look like a GRAS message");
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
  xbt_free(msg_name);

  *payload_size=gras_datadesc_size((*msgtype)->ctn_type);
  xbt_assert2(*payload_size > 0,
	       "%s %s",
	       "Dynamic array as payload is forbided for now (FIXME?).",
	       "Reference to dynamic array is allowed.");
  *payload = xbt_malloc(*payload_size);
  TRY(gras_datadesc_recv(sock, (*msgtype)->ctn_type, r_arch, *payload));

  return no_error;
}

/**
 * gras_msg_wait:
 * @timeout: How long should we wait for this message.
 * @id: id of awaited msg
 * @Returns: the error code (or no_error).
 *
 * Waits for a message to come in over a given socket.
 *
 * Every message of another type received before the one waited will be queued
 * and used by subsequent call to this function or MsgHandle().
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
  gras_procdata_t *pd=gras_procdata_get();
  int cpt;
  gras_msg_t msg;
  
  *expeditor = NULL;
  payload_got = NULL;

  if (!msgt_want)
    RAISE0(mismatch_error,
	   "Cannot wait for the NULL message (did msgtype_by_name fail?)");

  VERB1("Waiting for message %s",msgt_want->name);

  start = now = gras_os_time();

  xbt_dynar_foreach(pd->msg_queue,cpt,msg){
    if (msg.type->code == msgt_want->code) {
      *expeditor = msg.expeditor;
      memcpy(payload, msg.payload, msg.payload_size);
      xbt_free(msg.payload);
      xbt_dynar_cursor_rm(pd->msg_queue, &cpt);
      VERB0("The waited message was queued");
      return no_error;
    }
  }

  while (1) {
    TRY(gras_trp_select(timeout - now + start, expeditor));
    TRY(gras_msg_recv(*expeditor, &msgt_got, &payload_got, &payload_size_got));
    if (msgt_got->code == msgt_want->code) {
      memcpy(payload, payload_got, payload_size_got);
      xbt_free(payload_got);
      VERB0("Got waited message");
      return no_error;
    }

    /* not expected msg type. Queue it for later */
    msg.expeditor = *expeditor;
    msg.type      =  msgt_got;
    msg.payload   =  payload;
    msg.payload_size = payload_size_got;
    xbt_dynar_push(pd->msg_queue,&msg);
    
    now=gras_os_time();
    if (now - start + 0.001 < timeout) {
      RAISE1(timeout_error,"Timeout while waiting for msg %s",msgt_want->name);
    }
  }

  RAISE_IMPOSSIBLE;
}

/**
 * gras_msg_handle:
 * @timeOut: How long to wait for incoming messages
 * @Returns: the error code (or no_error).
 *
 * Waits up to #timeOut# seconds to see if a message comes in; if so, calls the
 * registered listener for that message (see RegisterCallback()).
 */
xbt_error_t 
gras_msg_handle(double timeOut) {
  
  xbt_error_t    errcode;
  int             cpt;

  gras_msg_t      msg;
  gras_socket_t   expeditor;
  void           *payload=NULL;
  int             payload_size;
  gras_msgtype_t  msgtype;

  gras_procdata_t*pd=gras_procdata_get();
  gras_cblist_t  *list;
  gras_cb_t       cb;



  VERB1("Handling message within the next %.2fs",timeOut);
  
  /* get a message (from the queue or from the net) */
  if (xbt_dynar_length(pd->msg_queue)) {
    xbt_dynar_shift(pd->msg_queue,&msg);
    expeditor = msg.expeditor;
    msgtype   = msg.type;
    payload   = msg.payload;
    
  } else {
    TRY(gras_trp_select(timeOut, &expeditor));
    TRY(gras_msg_recv(expeditor, &msgtype, &payload, &payload_size));
  }
      
  /* handle it */
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
    INFO3("Use the callback #%d (@%p) for incomming msg %s",
	  cpt+1,cb,msgtype->name);
    if ((*cb)(expeditor,payload)) {
      /* cb handled the message */
      xbt_free(payload);
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
    xbt_free(list);
  }
}

void
gras_cb_register(gras_msgtype_t msgtype,
		 gras_cb_t cb) {
  gras_procdata_t *pd=gras_procdata_get();
  gras_cblist_t *list=NULL;
  int cpt;

  DEBUG2("Register %p as callback to %s",cb,msgtype->name);

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
    list->cbs = xbt_dynar_new(sizeof(gras_cb_t), NULL);
    xbt_dynar_push(pd->cbl_list,&list);
  }

  /* Insert the new one into the set */
  xbt_dynar_insert_at(list->cbs,0,&cb);
}

void
gras_cb_unregister(gras_msgtype_t msgtype,
		   gras_cb_t cb) {

  gras_procdata_t *pd=gras_procdata_get();
  gras_cblist_t *list;
  gras_cb_t cb_cpt;
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
