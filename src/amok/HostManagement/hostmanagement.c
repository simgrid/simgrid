/* $Id$ */

/* amok host management - servers main loop and remote host stopping        */

/* Copyright (c) 2006 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/host.h"
#include "amok/hostmanagement.h"
#include "gras/Virtu/virtu_interface.h" /* libdata */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(amok_hm,amok,"Host management");


/* libdata management */
static int amok_hm_libdata_id=-1;
typedef struct {
  /* set headers */
  unsigned int ID;
  char        *name;
  unsigned int name_len;

  /* payload */
  int done;
  xbt_dict_t groups;
} s_amok_hm_libdata_t, *amok_hm_libdata_t;

static void *amok_hm_libdata_new() {
  amok_hm_libdata_t res=xbt_new(s_amok_hm_libdata_t,1);
  res->name=xbt_strdup("amok_hm");
  res->name_len=0;
  res->done = 0;
  res->groups = xbt_dict_new();
  return res;
}
static void amok_hm_libdata_free(void *d) {
  amok_hm_libdata_t data=(amok_hm_libdata_t)d;
  free(data->name);
  xbt_dict_free(&data->groups);
  free (data);
}

/* Message callbacks */
static int amok_hm_cb_kill(gras_msg_cb_ctx_t ctx,
			   void             *payload_data) {

  amok_hm_libdata_t g=gras_libdata_by_id(amok_hm_libdata_id);
  g->done = 1;
  return 1;
}
static int amok_hm_cb_killrpc(gras_msg_cb_ctx_t ctx,
			      void             *payload_data) {

  amok_hm_libdata_t g=gras_libdata_by_id(amok_hm_libdata_id);
  g->done = 1;
  gras_msg_rpcreturn(30,ctx,NULL);
  return 1;
}

static int amok_hm_cb_get(gras_msg_cb_ctx_t ctx, void *payload) {
  amok_hm_libdata_t g=gras_libdata_by_id(amok_hm_libdata_id);
  char *name = *(void**)payload;
  xbt_dynar_t res = xbt_dict_get(g->groups, name);

  gras_msg_rpcreturn(30, ctx, &res);
  return 1;
}
static int amok_hm_cb_join(gras_msg_cb_ctx_t ctx, void *payload) {
  amok_hm_libdata_t g=gras_libdata_by_id(amok_hm_libdata_id);
  char *name = *(void**)payload;
  xbt_dynar_t group = xbt_dict_get(g->groups, name);
  
  gras_socket_t exp = gras_msg_cb_ctx_from(ctx);
  xbt_host_t dude = xbt_host_new(gras_socket_peer_name(exp),
				 gras_socket_peer_port(exp));

  VERB2("Contacted by %s:%d",dude->name,dude->port);
  xbt_dynar_push(group,&dude);

  gras_msg_rpcreturn(10, ctx, NULL);
  free(name);
  return 1;
}
static int amok_hm_cb_leave(gras_msg_cb_ctx_t ctx, void *payload) {
  amok_hm_libdata_t g=gras_libdata_by_id(amok_hm_libdata_id);
  char *name = *(void**)payload;
  xbt_dynar_t group = xbt_dict_get(g->groups, name);
  
  gras_socket_t exp = gras_msg_cb_ctx_from(ctx);
  xbt_host_t dude = xbt_host_new(gras_socket_peer_name(exp),
				 gras_socket_peer_port(exp));

  int cpt;
  xbt_host_t host_it;

  xbt_dynar_foreach(group, cpt, host_it) {
    if (!strcmp(host_it->name, dude->name) && 
	host_it->port == dude->port) {
      xbt_dynar_cursor_rm (group,&cpt);
      goto end;
    }
  }
  WARN3("Asked to remove %s:%d from group '%s', but not found. Ignoring",
	dude->name,dude->port, name);

 end:
  gras_msg_rpcreturn(30, ctx, NULL);
  return 1;
}

static int amok_hm_cb_shutdown(gras_msg_cb_ctx_t ctx, void *payload) {
  char *name = *(void**)payload;
  amok_hm_group_shutdown(name);

  gras_msg_rpcreturn(30, ctx, NULL);
  return 1;
}


/* Initialization stuff */
static short amok_hm_used = 0;

/** \brief Initialize the host management module. Every process must run it before use */
void amok_hm_init() {
  /* pure INIT part */
  if (! amok_hm_used) {

    /* dependencies */
    amok_base_init();

    /* module data on each process */
    amok_hm_libdata_id = gras_procdata_add("amok_hm",
					   amok_hm_libdata_new,
					   amok_hm_libdata_free);

    /* Datatype and message declarations */
    gras_msgtype_declare("amok_hm_kill",NULL);   
    gras_msgtype_declare_rpc("amok_hm_killrpc",NULL,NULL);   

    gras_msgtype_declare_rpc("amok_hm_get",
			     gras_datadesc_by_name("string"),
			     gras_datadesc_by_name("xbt_dynar_t"));
    gras_msgtype_declare_rpc("amok_hm_join",
			     gras_datadesc_by_name("string"),
			     NULL);
    gras_msgtype_declare_rpc("amok_hm_leave",
			     gras_datadesc_by_name("string"),
			     NULL);

    gras_msgtype_declare_rpc("amok_hm_shutdown",
			     gras_datadesc_by_name("string"),
			     NULL);
  }
  amok_hm_used++;

  /* JOIN part */
  gras_cb_register(gras_msgtype_by_name("amok_hm_kill"),
		   &amok_hm_cb_kill);
  gras_cb_register(gras_msgtype_by_name("amok_hm_killrpc"),
		   &amok_hm_cb_killrpc);

  gras_cb_register(gras_msgtype_by_name("amok_hm_get"),
		   &amok_hm_cb_get);
  gras_cb_register(gras_msgtype_by_name("amok_hm_join"),
		   &amok_hm_cb_join);
  gras_cb_register(gras_msgtype_by_name("amok_hm_leave"),
		   &amok_hm_cb_leave);
  gras_cb_register(gras_msgtype_by_name("amok_hm_shutdown"),
		   &amok_hm_cb_shutdown);
}

/** \brief Finalize the host management module. Every process should run it after use */
void amok_hm_exit() {
  /* pure EXIT part */
  amok_hm_used--;

  /* LEAVE part */
  gras_cb_unregister(gras_msgtype_by_name("amok_hm_kill"),
		     &amok_hm_cb_kill);
  gras_cb_unregister(gras_msgtype_by_name("amok_hm_killrpc"),
		     &amok_hm_cb_killrpc);

  gras_cb_unregister(gras_msgtype_by_name("amok_hm_get"),
		     &amok_hm_cb_get);
  gras_cb_unregister(gras_msgtype_by_name("amok_hm_join"),
		     &amok_hm_cb_join);
  gras_cb_unregister(gras_msgtype_by_name("amok_hm_leave"),
		     &amok_hm_cb_leave);

  gras_cb_unregister(gras_msgtype_by_name("amok_hm_shutdown"),
		     &amok_hm_cb_shutdown);
}


/** \brief Enter the main loop of the program. It won't return until we get a kill message. */
void amok_hm_mainloop(double timeOut) {
  amok_hm_libdata_t g=gras_libdata_by_id(amok_hm_libdata_id);
  
  while (!g->done) {
    gras_msg_handle(timeOut);
  }
}

/** \brief kill a buddy identified by its hostname and port */
void amok_hm_kill_hp(char *name,int port) {
  gras_socket_t sock=gras_socket_client(name,port);
  amok_hm_kill(sock);
  gras_socket_close(sock);
}

/** \brief kill a buddy to which we have a socket already */
void amok_hm_kill(gras_socket_t buddy) {
  gras_msg_send(buddy,gras_msgtype_by_name("amok_hm_kill"),NULL);
}

/** \brief kill syncronously a buddy (do not return before its death) */
void amok_hm_kill_sync(gras_socket_t buddy) {
  gras_msg_rpccall(buddy,30,gras_msgtype_by_name("amok_hm_killrpc"),NULL,NULL);
}


/** \brief create a new hostmanagement group located on local host 
 *
 * The dynar elements are of type xbt_host_t
 */
xbt_dynar_t amok_hm_group_new(const char *group_name) {
  amok_hm_libdata_t g;
  xbt_dynar_t res = xbt_dynar_new(sizeof(xbt_host_t),
				  xbt_host_free_voidp);

  xbt_assert0(amok_hm_libdata_id != -1,"Run amok_hm_init first!");
  g=gras_libdata_by_id(amok_hm_libdata_id);
   
  xbt_dict_set(g->groups,group_name,res,NULL); /*FIXME: leaking xbt_dynar_free_voidp);*/
  VERB1("Group %s created",group_name);

  return res;
}
/** \brief retrieve all members of the given remote group */
xbt_dynar_t amok_hm_group_get(gras_socket_t master, const char *group_name) {
  xbt_dynar_t res;
  
  gras_msg_rpccall(master,30,gras_msgtype_by_name("amok_hm_get"),
		   &group_name,&res);
  return res;
}

/** \brief add current host to the given remote group */
void        amok_hm_group_join(gras_socket_t master, const char *group_name) {
  VERB3("Join group '%s' on %s:%d",
	group_name,gras_socket_peer_name(master),gras_socket_peer_port(master));
  gras_msg_rpccall(master,30,gras_msgtype_by_name("amok_hm_join"),
		   &group_name,NULL);
  VERB3("Joined group '%s' on %s:%d",
	group_name,gras_socket_peer_name(master),gras_socket_peer_port(master));
}
/** \brief remove current host from the given remote group if found
 *
 * If not found, call is ignored 
 */
void        amok_hm_group_leave(gras_socket_t master, const char *group_name) {
  gras_msg_rpccall(master,30,gras_msgtype_by_name("amok_hm_leave"),
		   &group_name,NULL);
  VERB3("Leaved group '%s' on %s:%d",
	group_name,gras_socket_peer_name(master),gras_socket_peer_port(master));
}

/** \brief stops all members of the given local group */
void amok_hm_group_shutdown(const char *group_name) {
  amok_hm_libdata_t g=gras_libdata_by_id(amok_hm_libdata_id);
  xbt_dynar_t group = xbt_dict_get(g->groups, group_name);
  
  int cpt;
  xbt_host_t host_it;

  xbt_dynar_foreach(group, cpt, host_it) {
    amok_hm_kill_hp(host_it->name, host_it->port);
  }

  xbt_dynar_free(&group);
  xbt_dict_remove(g->groups,group_name);
}
/** \brief stops all members of the given remote group */
void amok_hm_group_shutdown_remote(gras_socket_t master, const char *group_name){
  gras_msg_rpccall(master,30,gras_msgtype_by_name("amok_hm_shutdown"),
		   &group_name,NULL);
}
