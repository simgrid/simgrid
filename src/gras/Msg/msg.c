/* $Id$ */

/* messaging - Function related to messaging (code shared between RL and SG)*/

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "xbt/ex_interface.h"
#include "gras/Msg/msg_private.h"
#include "gras/Virtu/virtu_interface.h"
#include "gras/DataDesc/datadesc_interface.h"
#include "gras/Transport/transport_interface.h" /* gras_select */
#include "portable.h" /* execinfo when available to propagate exceptions */

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(gras_msg,gras,"High level messaging");

xbt_set_t _gras_msgtype_set = NULL;
static char *make_namev(const char *name, short int ver);
char _GRAS_header[6];
const char *e_gras_msg_kind_names[e_gras_msg_kind_count]=
  {"UNKNOWN","ONEWAY","RPC call","RPC answer","RPC error"};

/*
 * Creating procdata for this module
 */
static void *gras_msg_procdata_new() {
   gras_msg_procdata_t res = xbt_new(s_gras_msg_procdata_t,1);
   
   res->name = xbt_strdup("gras_msg");
   res->name_len = 0;
   res->msg_queue     = xbt_dynar_new(sizeof(s_gras_msg_t),   NULL);
   res->msg_waitqueue = xbt_dynar_new(sizeof(s_gras_msg_t),   NULL);
   res->cbl_list      = xbt_dynar_new(sizeof(gras_cblist_t *),gras_cbl_free);
   res->timers        = xbt_dynar_new(sizeof(s_gras_timer_t), NULL);
   
   return (void*)res;
}

/*
 * Freeing procdata for this module
 */
static void gras_msg_procdata_free(void *data) {
   gras_msg_procdata_t res = (gras_msg_procdata_t)data;
   
   xbt_dynar_free(&( res->msg_queue ));
   xbt_dynar_free(&( res->msg_waitqueue ));
   xbt_dynar_free(&( res->cbl_list ));
   xbt_dynar_free(&( res->timers ));

   free(res->name);
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
   
  gras_msg_ctx_mallocator = 
     xbt_mallocator_new(1000,
			gras_msg_ctx_mallocator_new_f,
			gras_msg_ctx_mallocator_free_f,
			gras_msg_ctx_mallocator_reset_f);
}

/*
 * Finalize the msg module
 */
void
gras_msg_exit(void) {
  VERB0("Exiting Msg");
  xbt_set_free(&_gras_msgtype_set);

  xbt_mallocator_free(gras_msg_ctx_mallocator);
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
 * Dump all declared message types (debugging purpose)
 */
void gras_msgtype_dumpall(void) {   
  xbt_set_cursor_t cursor;
  gras_msgtype_t msgtype=NULL;
   
  INFO0("Dump of all registered messages:");
  xbt_set_foreach(_gras_msgtype_set, cursor, msgtype) {
    INFO6("  Message name: %s (v%d) %s; %s%s%s", 
	  msgtype->name, msgtype->version, e_gras_msg_kind_names[msgtype->kind],
	  gras_datadesc_get_name(msgtype->ctn_type),
	  (msgtype->kind==e_gras_msg_kind_rpccall ? " -> ":""),
	  (msgtype->kind==e_gras_msg_kind_rpccall ? gras_datadesc_get_name(msgtype->answer_type) : ""));
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

/* Internal function doing the crude work of registering messages */
void 
gras_msgtype_declare_ext(const char           *name,
			 short int             version,
			 e_gras_msg_kind_t     kind, 
			 gras_datadesc_type_t  payload_request,
			 gras_datadesc_type_t  payload_answer) {

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
    VERB2("Re-register version %d of message '%s' (same kind & payload, ignored).",
	  version, name);
    xbt_assert3(msgtype->kind == kind,
		"Message %s re-registered as a %s (it was known as a %s)",
		namev,e_gras_msg_kind_names[kind],e_gras_msg_kind_names[msgtype->kind]);
    xbt_assert3(!gras_datadesc_type_cmp(msgtype->ctn_type, payload_request),
		 "Message %s re-registred with another payload (%s was %s)",
		 namev,gras_datadesc_get_name(payload_request),
		 gras_datadesc_get_name(msgtype->ctn_type));

    xbt_assert3(!gras_datadesc_type_cmp(msgtype->answer_type, payload_answer),
	     "Message %s re-registred with another answer payload (%s was %s)",
		 namev,gras_datadesc_get_name(payload_answer),
		 gras_datadesc_get_name(msgtype->answer_type));

    return ; /* do really ignore it */

  }

  VERB4("Register version %d of message '%s' "
	"(payload: %s; answer payload: %s).", 
	version, name, gras_datadesc_get_name(payload_request),
	gras_datadesc_get_name(payload_answer));    

  msgtype = xbt_new(s_gras_msgtype_t,1);
  msgtype->name = (namev == name ? strdup(name) : namev);
  msgtype->name_len = strlen(namev);
  msgtype->version = version;
  msgtype->kind = kind;
  msgtype->ctn_type = payload_request;
  msgtype->answer_type = payload_answer;

  xbt_set_add(_gras_msgtype_set, (xbt_set_elm_t)msgtype,
	       &gras_msgtype_free);
}


/** @brief declare a new message type of the given name. It only accepts the given datadesc as payload
 *
 * @param name: name as it should be used for logging messages (must be uniq)
 * @param payload: datadescription of the payload
 */
void gras_msgtype_declare(const char           *name,
			  gras_datadesc_type_t  payload) {
   gras_msgtype_declare_ext(name, 0, e_gras_msg_kind_oneway, payload, NULL);
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
 
   gras_msgtype_declare_ext(name, version, 
			    e_gras_msg_kind_oneway, payload, NULL);
}

/** @brief retrieve an existing message type from its name (raises an exception if it does not exist). */
gras_msgtype_t gras_msgtype_by_name (const char *name) {
  return gras_msgtype_by_namev(name,0);
}
/** @brief retrieve an existing message type from its name (or NULL if it does not exist). */
gras_msgtype_t gras_msgtype_by_name_or_null (const char *name) {
  xbt_ex_t e;
  gras_msgtype_t res = NULL;
   
  TRY {
     res = gras_msgtype_by_namev(name,0);
  } CATCH(e) {
     res = NULL;
     xbt_ex_free(e);
  }
  return res;
}

/** @brief retrieve an existing message type from its name and version. */
gras_msgtype_t gras_msgtype_by_namev(const char      *name,
				     short int        version) {
  gras_msgtype_t res = NULL;
  char *namev = make_namev(name,version);
  volatile int found=0;
  xbt_ex_t e;

  TRY {
    res = (gras_msgtype_t)xbt_set_get_by_name(_gras_msgtype_set, namev);
    found=1;
  } CATCH(e) {
    xbt_ex_free(e);
  }
  if (!found)
    THROW1(not_found_error,0,"No registred message of that name: %s",name);

  if (name != namev) 
    free(namev);
  
  return res;
}
/** @brief retrieve an existing message type from its name and version. */
gras_msgtype_t gras_msgtype_by_id(int id) {
  return (gras_msgtype_t)xbt_set_get_by_id(_gras_msgtype_set, id);
}

/** \brief Waits for a message to come in over a given socket. 
 *
 * @param timeout: How long should we wait for this message.
 * @param msgt_want: type of awaited msg (or NULL if I'm enclined to accept any message)
 * @param expe_want: awaited expeditot (match on hostname, not port; NULL if not relevant)
 * @param filter: function returning true or false when passed a payload. Messages for which it returns false are not selected. (NULL if not relevant)
 * @param filter_ctx: context passed as second argument of the filter (a pattern to match?)
 * @param[out] msg_got: where to write the message we got
 *
 * Every message of another type received before the one waited will be queued
 * and used by subsequent call to this function or gras_msg_handle().
 */

void
gras_msg_wait_ext(double           timeout,    

		  gras_msgtype_t   msgt_want,
		  gras_socket_t    expe_want,
		  gras_msg_filter_t filter,
		  void             *filter_ctx, 

		  gras_msg_t       msg_got) {

  s_gras_msg_t msg;
  double start, now;
  gras_msg_procdata_t pd=(gras_msg_procdata_t)gras_libdata_by_id(gras_msg_libdata_id);
  int cpt;

  xbt_assert0(msg_got,"msg_got is an output parameter");

  start = gras_os_time();
  VERB1("Waiting for message '%s'",msgt_want?msgt_want->name:"(any)");

  xbt_dynar_foreach(pd->msg_waitqueue,cpt,msg){
    if ( (   !msgt_want || (msg.type->code == msgt_want->code)) 
	 && (!expe_want || (!strcmp( gras_socket_peer_name(msg.expe),
				     gras_socket_peer_name(expe_want))))
	 && (!filter || filter(&msg,filter_ctx))) {

      memcpy(msg_got,&msg,sizeof(s_gras_msg_t));
      xbt_dynar_cursor_rm(pd->msg_waitqueue, &cpt);
      VERB0("The waited message was queued");
      return;
    }
  }

  xbt_dynar_foreach(pd->msg_queue,cpt,msg){
    if ( (   !msgt_want || (msg.type->code == msgt_want->code)) 
	 && (!expe_want || (!strcmp( gras_socket_peer_name(msg.expe),
				     gras_socket_peer_name(expe_want))))
	 && (!filter || filter(&msg,filter_ctx))) {

      memcpy(msg_got,&msg,sizeof(s_gras_msg_t));
      xbt_dynar_cursor_rm(pd->msg_queue, &cpt);
      VERB0("The waited message was queued");
      return;
    }
  }

  while (1) {
    int need_restart;
    xbt_ex_t e;

  restart_receive: /* Goto here when the receive of a message failed */
    need_restart=0;
    now=gras_os_time();
    memset(&msg,sizeof(msg),0);

    TRY {
      msg.expe = gras_trp_select(timeout ? timeout - now + start : 0);
      gras_msg_recv(msg.expe, &msg);
    } CATCH(e) {
      if (e.category == system_error &&
	  !strncmp("Socket closed by remote side",e.msg,
		  strlen("Socket closed by remote side"))) {
	xbt_ex_free(e);
	need_restart=1;
      }	else {
	RETHROW;
      }
    }
    if (need_restart)
      goto restart_receive;

    DEBUG0("Got a message from the socket");

    if ( (   !msgt_want || (msg.type->code == msgt_want->code)) 
	 && (!expe_want || (!strcmp( gras_socket_peer_name(msg.expe),
				     gras_socket_peer_name(expe_want))))
	 && (!filter || filter(&msg,filter_ctx))) {

      memcpy(msg_got,&msg,sizeof(s_gras_msg_t));
      DEBUG0("Message matches expectations. Use it.");
      return;
    }
    DEBUG0("Message does not match expectations. Queue it.");

    /* not expected msg type. Queue it for later */
    xbt_dynar_push(pd->msg_queue,&msg);
    
    now=gras_os_time();
    if (now - start + 0.001 > timeout) {
      THROW1(timeout_error,  now-start+0.001-timeout,
	     "Timeout while waiting for msg '%s'",
	     msgt_want?msgt_want->name:"(any)");
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
  s_gras_msg_t msg;

  gras_msg_wait_ext(timeout,
		    msgt_want, NULL,      NULL, NULL,
		    &msg);

  if (msgt_want->ctn_type) {
    xbt_assert1(payload,
		"Message type '%s' convey a payload you must accept",
		msgt_want->name);
  } else {
    xbt_assert1(!payload,
		"No payload was declared for message type '%s'",
		msgt_want->name);
  }

  if (payload) {
    memcpy(payload,msg.payl,msg.payl_size);
    free(msg.payl);
  }

  if (expeditor)
    *expeditor = msg.expe;
}

static int gras_msg_wait_or_filter(gras_msg_t msg, void *ctx) {
  xbt_dynar_t dyn=(xbt_dynar_t)ctx;
  int res =  xbt_dynar_member(dyn,msg->type);
  if (res)
    VERB1("Got matching message (type=%s)",msg->type->name);
  else
    VERB0("Got message not matching our expectations");
  return res;
}
/** \brief Waits for a message to come in over a given socket. 
 *
 * @param timeout: How long should we wait for this message.
 * @param msgt_want: a dynar containing all accepted message type
 * @param[out] ctx: the context of received message (in case it's a RPC call we want to answer to)
 * @param[out] msgt_got: indice in the dynar of the type of the received message 
 * @param[out] payload: where to write the payload of the incomming message
 * @return the error code (or no_error).
 *
 * Every message of a type not in the accepted list received before the one
 * waited will be queued and used by subsequent call to this function or
 * gras_msg_handle().
 *
 * If you are interested in the context, pass the address of a s_gras_msg_cb_ctx_t variable.
 */
void gras_msg_wait_or(double         timeout,
                      xbt_dynar_t    msgt_want,
		      gras_msg_cb_ctx_t *ctx,
                      int           *msgt_got,
                      void          *payload) {
  s_gras_msg_t msg;

  VERB1("Wait %f seconds for several message types",timeout);
  gras_msg_wait_ext(timeout,
		    NULL, NULL,      
		    &gras_msg_wait_or_filter, (void*)msgt_want,
		    &msg);

  if (msg.type->ctn_type) {
    xbt_assert1(payload,
		"Message type '%s' convey a payload you must accept",
		msg.type->name);
  } /* don't check the other side since some of the types may have a payload */

  if (payload && msg.type->ctn_type) {
    memcpy(payload,msg.payl,msg.payl_size);
    free(msg.payl);
  }

  if (ctx) 
    *ctx=gras_msg_cb_ctx_new(msg.expe, msg.type, msg.ID,
			     (msg.kind == e_gras_msg_kind_rpccall), 60);

  if (msgt_got)
    *msgt_got = xbt_dynar_search(msgt_want,msg.type);
}


/** \brief Send the data pointed by \a payload as a message of type
 * \a msgtype to the peer \a sock */
void
gras_msg_send(gras_socket_t   sock,
	      gras_msgtype_t  msgtype,
	      void           *payload) {

  if (msgtype->ctn_type) {
    xbt_assert1(payload,
		"Message type '%s' convey a payload you must provide",
		msgtype->name);
  } else {
    xbt_assert1(!payload,
		"No payload was declared for message type '%s'",
		msgtype->name);
  }

  DEBUG2("Send a oneway message of type '%s'. Payload=%p",
	 msgtype->name,payload);
  gras_msg_send_ext(sock, e_gras_msg_kind_oneway,0, msgtype, payload);
  VERB2("Sent a oneway message of type '%s'. Payload=%p",
	msgtype->name,payload);
}

/** @brief Handle all messages arriving within the given period
 *
 * @param period: How long to wait for incoming messages (in seconds)
 *
 * Messages are dealed with just like gras_msg_handle() would do. The
 * difference is that gras_msg_handle() handles at most one message (or wait up
 * to timeout second when no message arrives) while this function handles any
 * amount of messages, and lasts the given period in any case.
 */
void 
gras_msg_handleall(double period) {
  xbt_ex_t e;
  double begin=gras_os_time();
  double now;

  do {
    now=gras_os_time();
    TRY{
      if (period - now + begin > 0)
	gras_msg_handle(period - now + begin);
    } CATCH(e) {
      if (e.category != timeout_error) 
	RETHROW0("Error while waiting for messages: %s");
      xbt_ex_free(e);
    }
  } while (now - begin < period);
}

/** @brief Handle an incomming message or timer (or wait up to \a timeOut seconds)
 *
 * @param timeOut: How long to wait for incoming messages (in seconds)
 * @return the error code (or no_error).
 *
 * Messages are passed to the callbacks. See also gras_msg_handleall().
 */
void
gras_msg_handle(double timeOut) {
  
  double          untiltimer;
   
  int             cpt;
  int volatile ran_ok;

  s_gras_msg_t    msg;

  gras_msg_procdata_t pd=(gras_msg_procdata_t)gras_libdata_by_id(gras_msg_libdata_id);
  gras_cblist_t  *list=NULL;
  gras_msg_cb_t       cb;
  s_gras_msg_cb_ctx_t ctx;
   
  int timerexpected, timeouted;
  xbt_ex_t e;

  VERB1("Handling message within the next %.2fs",timeOut);
  
  untiltimer = gras_msg_timer_handle();
  DEBUG1("Next timer in %f sec", untiltimer);
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
  } else {
    TRY {
      msg.expe = gras_trp_select(timeOut);
    } CATCH(e) {
      if (e.category != timeout_error)
	RETHROW;
      xbt_ex_free(e);
      timeouted = 1;
    }

    if (!timeouted) {
      TRY {
	/* FIXME: if not the right kind, queue it and recall ourself or goto >:-) */
	gras_msg_recv(msg.expe, &msg);
	DEBUG1("Received a msg from the socket kind:%s",
	       e_gras_msg_kind_names[msg.kind]);
    
      } CATCH(e) {
	RETHROW4("Error while receiving a message on select()ed socket %p to [%s]%s:%d: %s",
		 msg.expe,
		 gras_socket_peer_proc(msg.expe),gras_socket_peer_name(msg.expe),
		 gras_socket_peer_port(msg.expe));
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
       THROW1(timeout_error, 0, "No new message or timer (delay was %f)",
	      timeOut);
     }
     
  }
   
  /* A message was already there or arrived in the meanwhile. handle it */
  xbt_dynar_foreach(pd->cbl_list,cpt,list) {
    if (list->id == msg.type->code) {
      break;
    } else {
      list=NULL;
    }
  }
  if (!list) {
    INFO3("No callback for message '%s' from %s:%d. Queue it for later gras_msg_wait() use.",
	  msg.type->name,
	  gras_socket_peer_name(msg.expe),gras_socket_peer_port(msg.expe));
    xbt_dynar_push(pd->msg_waitqueue,&msg);
    return; /* FIXME: maybe we should call ourselves again until the end of the timer or a proper msg is got */
  }
  
  ctx.expeditor = msg.expe;
  ctx.ID = msg.ID;
  ctx.msgtype = msg.type;
  ctx.answer_due = (msg.kind == e_gras_msg_kind_rpccall);

  switch (msg.kind) {
  case e_gras_msg_kind_oneway:
  case e_gras_msg_kind_rpccall:
    ran_ok=0;
    TRY {
      xbt_dynar_foreach(list->cbs,cpt,cb) { 
	if (!ran_ok) {
	  DEBUG4("Use the callback #%d (@%p) for incomming msg %s (payload_size=%d)",
		cpt+1,cb,msg.type->name,msg.payl_size);
	  if ((*cb)(&ctx,msg.payl)) {
	    /* cb handled the message */
	    free(msg.payl);
	    ran_ok = 1;
	  }
	}
      }
    } CATCH(e) {
      free(msg.payl);
      if (msg.type->kind == e_gras_msg_kind_rpccall) {
	/* The callback raised an exception, propagate it on the network */
	if (!e.remote) { /* the exception is born on this machine */
	  e.host = (char*)gras_os_myname();
	  xbt_ex_setup_backtrace(&e);
	} 
	VERB5("Propagate %s exception ('%s') from '%s' RPC cb back to %s:%d",
	      (e.remote ? "remote" : "local"),
	      e.msg,
	      msg.type->name,
	      gras_socket_peer_name(msg.expe),
	      gras_socket_peer_port(msg.expe));
	gras_msg_send_ext(msg.expe, e_gras_msg_kind_rpcerror,
			  msg.ID, msg.type, &e);
	xbt_ex_free(e);
	ctx.answer_due = 0;
	ran_ok=1;
      } else {
	RETHROW0("Callback raised an exception: %s");
      }
    }
    xbt_assert0(!(ctx.answer_due),
		"RPC callback didn't call gras_msg_rpcreturn");

    if (!ran_ok)
      THROW1(mismatch_error,0,
	     "Message '%s' refused by all registered callbacks", msg.type->name);
    /* FIXME: gras_datadesc_free not implemented => leaking the payload */
    break;


  case e_gras_msg_kind_rpcanswer:
    INFO1("Unexpected RPC answer discarded (type: %s)", msg.type->name);
    WARN0("FIXME: gras_datadesc_free not implemented => leaking the payload");
    return;

  case e_gras_msg_kind_rpcerror:
    INFO1("Unexpected RPC error discarded (type: %s)", msg.type->name);
    WARN0("FIXME: gras_datadesc_free not implemented => leaking the payload");
    return;

  default:
    THROW1(unknown_error,0,
    	   "Cannot handle messages of kind %d yet",msg.type->kind);
  }

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

/** \brief Retrieve the expeditor of the message */
gras_socket_t gras_msg_cb_ctx_from(gras_msg_cb_ctx_t ctx) {
  return ctx->expeditor;
}
/* \brief Creates a new message exchange context (user should never have to) */
gras_msg_cb_ctx_t gras_msg_cb_ctx_new(gras_socket_t expe, 
				      gras_msgtype_t msgtype,
				      unsigned long int ID,
				      int answer_due,
				      double timeout) {
  gras_msg_cb_ctx_t res=xbt_new(s_gras_msg_cb_ctx_t,1);
  res->expeditor = expe;
  res->msgtype = msgtype;
  res->ID = ID;
  res->timeout = timeout;
  res->answer_due = answer_due;

  return res;
}
/* \brief Frees a message exchange context 
 *
 * This function is mainly useful with \ref gras_msg_wait_or, ie seldom.
 */
void gras_msg_cb_ctx_free(gras_msg_cb_ctx_t ctx) {
  free(ctx);
}
