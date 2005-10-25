/* $Id$ */

/* messaging - Function related to messaging (code shared between RL and SG)*/

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "gras/Msg/msg_private.h"
#include "gras/Virtu/virtu_interface.h"
#include "gras/DataDesc/datadesc_interface.h"
#include "gras/Transport/transport_interface.h" /* gras_select */

#define MIN(a,b) ((a) < (b) ? (a) : (b))

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_msg,gras,"High level messaging");

xbt_set_t _gras_msgtype_set = NULL;
static char *make_namev(const char *name, short int ver);
char _GRAS_header[6];

/*
 * Creating procdata for this module
 */
static void *gras_msg_procdata_new() {
   gras_msg_procdata_t res = xbt_new(s_gras_msg_procdata_t,1);
   
   res->name = xbt_strdup("gras_msg");
   res->name_len = 0;
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
int gras_msg_libdata_id;
void gras_msg_register() {
   gras_msg_libdata_id = gras_procdata_add("gras_msg",gras_msg_procdata_new, gras_msg_procdata_free);
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

  memcpy(_GRAS_header,"GRAS", 4);
  _GRAS_header[4]=GRAS_PROTOCOL_VERSION;
  _GRAS_header[5]=(char)GRAS_THISARCH;
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
 
  gras_msgtype_t msgtype=NULL;
  char *namev=make_namev(name,version);
  volatile int found = 0;
  xbt_ex_t e;    
  
  TRY {
    msgtype = (gras_msgtype_t)xbt_set_get_by_name(_gras_msgtype_set,namev);
    found = 1;
  } CATCH(e) {
    if (e.category != not_found_error)
      RETHROW;
    xbt_ex_free(e);
  }

  if (found) {
    VERB2("Re-register version %d of message '%s' (same payload, ignored).",
	  version, name);
    xbt_assert3(!gras_datadesc_type_cmp(msgtype->ctn_type, payload),
		 "Message %s re-registred with another payload (%s was %s)",
		 namev,gras_datadesc_get_name(payload),
		 gras_datadesc_get_name(msgtype->ctn_type));

    return ; /* do really ignore it */

  }

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
  char *namev = make_namev(name,version); 

  res = (gras_msgtype_t)xbt_set_get_by_name(_gras_msgtype_set, namev);
  if (name != namev) 
    free(namev);
  
  return res;
}
/** @brief retrive an existing message type from its name and version. */
gras_msgtype_t gras_msgtype_by_id(int id) {
  return (gras_msgtype_t)xbt_set_get_by_id(_gras_msgtype_set, id);
}

/** \brief Waits for a message to come in over a given socket. 
 *
 * @param timeout: How long should we wait for this message.
 * @param msgt_want: type of awaited msg (or NULL if I'm enclined to accept any message)
 * @param expe_want: awaited expeditot (match on hostname, not port; NULL if not relevant)
 * @param payl_filter: function returning true or false when passed a payload. Messages for which it returns false are not selected. (NULL if not relevant)
 * @param filter_ctx: context passed as second argument of the filter (a pattern to match?)
 * @param[out] msgt_got: where to write the descriptor of the message we got
 * @param[out] expe_got: where to create a socket to answer the incomming message
 * @param[out] payl_got: where to write the payload of the incomming message
 *
 * Every message of another type received before the one waited will be queued
 * and used by subsequent call to this function or gras_msg_handle().
 */

void
gras_msg_wait_ext(double           timeout,    
		  gras_msgtype_t   msgt_want,
		  gras_socket_t    expe_want,
		  int_f_pvoid_pvoid_t payl_filter,
		  void               *filter_ctx, 
		  gras_msgtype_t  *msgt_got,
		  gras_socket_t   *expe_got,
		  void            *payl_got) {

  s_gras_msg_t msg;
  double start, now;
  gras_msg_procdata_t pd=(gras_msg_procdata_t)gras_libdata_by_id(gras_msg_libdata_id);
  int cpt;

  xbt_assert0(msgt_want,"Cannot wait for the NULL message");

  VERB1("Waiting for message '%s'",msgt_want->name);

  start = now = gras_os_time();

  xbt_dynar_foreach(pd->msg_queue,cpt,msg){
    if ( (   !msgt_want || (msg.type->code == msgt_want->code)) 
	 && (!expe_want || (!strcmp( gras_socket_peer_name(msg.expe),
				     gras_socket_peer_name(expe_want))))
	 && (!payl_filter || payl_filter(msg.payl,filter_ctx))) {

      if (expe_got)
        *expe_got = msg.expe;
      if (msgt_got)
	*msgt_got = msg.type;
      if (payl_got) 
	memcpy(payl_got, msg.payl, msg.payl_size);
      free(msg.payl);
      xbt_dynar_cursor_rm(pd->msg_queue, &cpt);
      VERB0("The waited message was queued");
      return;
    }
  }

  while (1) {
    memset(&msg,sizeof(msg),0);

    msg.expe = gras_trp_select(timeout - now + start);
    gras_msg_recv(msg.expe, &msg);

    if ( (   !msgt_want || (msg.type->code == msgt_want->code)) 
	 && (!expe_want || (!strcmp( gras_socket_peer_name(msg.expe),
				     gras_socket_peer_name(expe_want))))
	 && (!payl_filter || payl_filter(msg.payl,filter_ctx))) {

      if (expe_got)
      	*expe_got=msg.expe;
      if (msgt_got)
	*msgt_got = msg.type;
      if (payl_got) 
	memcpy(payl_got, msg.payl, msg.payl_size);
      free(msg.payl);
      return;
    }

    /* not expected msg type. Queue it for later */
    xbt_dynar_push(pd->msg_queue,&msg);
    
    now=gras_os_time();
    if (now - start + 0.001 < timeout) {
      THROW1(timeout_error,  now-start+0.001-timeout,
	     "Timeout while waiting for msg %s",msgt_want->name);
    }
  }

  THROW_IMPOSSIBLE;
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
void
gras_msg_wait(double           timeout,    
	      gras_msgtype_t   msgt_want,
	      gras_socket_t   *expeditor,
	      void            *payload) {

  return gras_msg_wait_ext(timeout,
			   msgt_want, NULL,      NULL, NULL,
			   NULL,      expeditor, payload);
}


/** \brief Send the data pointed by \a payload as a message of type
 * \a msgtype to the peer \a sock */
void
gras_msg_send(gras_socket_t   sock,
	      gras_msgtype_t  msgtype,
	      void           *payload) {

  gras_msg_send_ext(sock, e_gras_msg_kind_oneway, msgtype, payload);
}

/** @brief Handle an incomming message or timer (or wait up to \a timeOut seconds)
 *
 * @param timeOut: How long to wait for incoming messages (in seconds)
 * @return the error code (or no_error).
 *
 * Messages are passed to the callbacks.
 */
void
gras_msg_handle(double timeOut) {
  
  double          untiltimer;
   
  int             cpt;

  s_gras_msg_t    msg;
  gras_socket_t   expeditor=NULL;
  void           *payload=NULL;
  gras_msgtype_t  msgtype=NULL;

  gras_msg_procdata_t pd=(gras_msg_procdata_t)gras_libdata_by_id(gras_msg_libdata_id);
  gras_cblist_t  *list=NULL;
  gras_msg_cb_t       cb;
   
  int timerexpected, timeouted;
  xbt_ex_t e;

  VERB1("Handling message within the next %.2fs",timeOut);
  
  untiltimer = gras_msg_timer_handle();
  DEBUG2("[%.0f] Next timer in %f sec", gras_os_time(), untiltimer);
  if (untiltimer == 0.0) {
     /* A timer was already elapsed and handled */
     return;
  }
  if (untiltimer != -1.0) {
     timerexpected = 1;
     timeOut = MIN(timeOut, untiltimer);
  } else {
     timerexpected = 0;
  }
   
  /* get a message (from the queue or from the net) */
  timeouted = 0;
  if (xbt_dynar_length(pd->msg_queue)) {
    DEBUG0("Get a message from the queue");
    xbt_dynar_shift(pd->msg_queue,&msg);
    expeditor = msg.expe;
    msgtype   = msg.type;
    payload   = msg.payl;
  } else {
    TRY {
      expeditor = gras_trp_select(timeOut);
    } CATCH(e) {
      if (e.category != timeout_error)
	RETHROW;
      xbt_ex_free(e);
      timeouted = 1;
    }

    if (!timeouted) {
      TRY {
	/* FIXME: if not the right kind, queue it and recall ourself or goto >:-) */
	gras_msg_recv(expeditor, &msg);
	msgtype   = msg.type;
	payload   = msg.payl;
      } CATCH(e) {
	RETHROW1("Error caught while receiving a message on select()ed socket %p: %s",
	  	 expeditor);
      }
    }
  }

  if (timeouted) {
     if (timerexpected) {
	  
	/* A timer elapsed before the arrival of any message even if we select()ed a bit */
	untiltimer = gras_msg_timer_handle();
	if (untiltimer == 0.0) {
	  /* we served a timer, we're done */
	  return;
	} else {
	   xbt_assert1(untiltimer>0, "Negative timer (%f). I'm 'puzzeled'", untiltimer);
	   WARN1("No timer elapsed, in contrary to expectations (next in %f sec)",
		  untiltimer);
	   THROW1(timeout_error,0,
		  "No timer elapsed, in contrary to expectations (next in %f sec)",
		  untiltimer);
	}
	
     } else {
	/* select timeouted, and no timer elapsed. Nothing to do */
       THROW0(timeout_error, 0, "No new message or timer");
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
    return;
  }
  
  xbt_dynar_foreach(list->cbs,cpt,cb) { 
    VERB3("Use the callback #%d (@%p) for incomming msg %s",
          cpt+1,cb,msgtype->name);
    if ((*cb)(expeditor,payload)) {
      /* cb handled the message */
      free(payload);
      return;
    }
  }

  /* FIXME: gras_datadesc_free not implemented => leaking the payload */
  THROW1(mismatch_error,0,
	 "Message '%s' refused by all registered callbacks", msgtype->name);
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
  gras_msg_procdata_t pd=(gras_msg_procdata_t)gras_libdata_by_id(gras_msg_libdata_id);
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

  gras_msg_procdata_t pd=(gras_msg_procdata_t)gras_libdata_by_id(gras_msg_libdata_id);
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
