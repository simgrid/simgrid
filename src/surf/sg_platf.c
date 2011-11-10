/* Copyright (c) 2006, 2007, 2008, 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/dict.h"
#include "simgrid/platf_interface.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_parse);
xbt_dynar_t sg_platf_host_cb_list = NULL;   // of sg_platf_host_cb_t
xbt_dynar_t sg_platf_link_cb_list = NULL;   // of sg_platf_link_cb_t
xbt_dynar_t sg_platf_router_cb_list = NULL; // of sg_platf_router_cb_t
xbt_dynar_t sg_platf_peer_cb_list = NULL; // of sg_platf_peer_cb_t
xbt_dynar_t sg_platf_AS_begin_cb_list = NULL; //of sg_platf_AS_begin_cb_t
xbt_dynar_t sg_platf_AS_end_cb_list = NULL; //of void_f_void_t
xbt_dynar_t sg_platf_postparse_cb_list = NULL; // of void_f_void_t

/** Module management function: creates all internal data structures */
void sg_platf_init(void) {
  sg_platf_host_cb_list = xbt_dynar_new(sizeof(sg_platf_host_cb_t), NULL);
  sg_platf_router_cb_list = xbt_dynar_new(sizeof(sg_platf_host_cb_t), NULL);
  sg_platf_link_cb_list = xbt_dynar_new(sizeof(sg_platf_host_cb_t), NULL);
  sg_platf_peer_cb_list = xbt_dynar_new(sizeof(sg_platf_peer_cb_t), NULL);
  sg_platf_postparse_cb_list = xbt_dynar_new(sizeof(sg_platf_link_cb_t),NULL);
  sg_platf_AS_begin_cb_list = xbt_dynar_new(sizeof(sg_platf_AS_begin_cb_t),NULL);
  sg_platf_AS_end_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);
}
/** Module management function: frees all internal data structures */
void sg_platf_exit(void) {
  xbt_dynar_free(&sg_platf_host_cb_list);
  xbt_dynar_free(&sg_platf_router_cb_list);
  xbt_dynar_free(&sg_platf_link_cb_list);
  xbt_dynar_free(&sg_platf_postparse_cb_list);
  xbt_dynar_free(&sg_platf_peer_cb_list);
  xbt_dynar_free(&sg_platf_AS_begin_cb_list);
  xbt_dynar_free(&sg_platf_AS_end_cb_list);
}

void sg_platf_new_host(sg_platf_host_cbarg_t h){
  unsigned int iterator;
  sg_platf_host_cb_t fun;
  xbt_dynar_foreach(sg_platf_host_cb_list, iterator, fun) {
    (*fun) (h);
  }
}
void sg_platf_new_router(sg_platf_router_cbarg_t router) {
  unsigned int iterator;
  sg_platf_router_cb_t fun;
  xbt_dynar_foreach(sg_platf_router_cb_list, iterator, fun) {
    (*fun) (router);
  }
}
void sg_platf_new_link(sg_platf_link_cbarg_t link){
  unsigned int iterator;
  sg_platf_link_cb_t fun;
  xbt_dynar_foreach(sg_platf_link_cb_list, iterator, fun) {
    (*fun) (link);
  }
}
void sg_platf_new_peer(sg_platf_peer_cbarg_t peer){
  unsigned int iterator;
  sg_platf_peer_cb_t fun;
  xbt_dynar_foreach(sg_platf_link_cb_list, iterator, fun) {
    (*fun) (peer);
  }
}

void sg_platf_begin() { /* Do nothing: just for symmetry of user code */ }

void sg_platf_end() {
  unsigned int iterator;
  void_f_void_t fun;
  xbt_dynar_foreach(sg_platf_postparse_cb_list, iterator, fun) {
    (*fun) ();
  }
}

void sg_platf_new_AS_begin(const char *id, const char *routing) {
  unsigned int iterator;
  sg_platf_AS_begin_cb_t fun;
  xbt_dynar_foreach(sg_platf_AS_begin_cb_list, iterator, fun) {
    (*fun) (id,routing);
  }
}

void sg_platf_new_AS_end() {
  unsigned int iterator;
  void_f_void_t fun;
  xbt_dynar_foreach(sg_platf_AS_end_cb_list, iterator, fun) {
    (*fun) ();
  }
}


void sg_platf_host_add_cb(sg_platf_host_cb_t fct) {
  xbt_dynar_push(sg_platf_host_cb_list, &fct);
}
void sg_platf_link_add_cb(sg_platf_link_cb_t fct) {
  xbt_dynar_push(sg_platf_link_cb_list, &fct);
}
void sg_platf_router_add_cb(sg_platf_router_cb_t fct) {
  xbt_dynar_push(sg_platf_router_cb_list, &fct);
}
void sg_platf_peer_add_cb(sg_platf_peer_cb_t fct) {
  xbt_dynar_push(sg_platf_peer_cb_list, &fct);
}
void sg_platf_postparse_add_cb(void_f_void_t fct) {
  xbt_dynar_push(sg_platf_postparse_cb_list, &fct);
}
void sg_platf_AS_begin_add_cb(sg_platf_AS_begin_cb_t fct) {
  xbt_dynar_push(sg_platf_AS_begin_cb_list, &fct);
}
void sg_platf_AS_end_add_cb(void_f_void_t fct) {
  xbt_dynar_push(sg_platf_AS_end_cb_list, &fct);
}

