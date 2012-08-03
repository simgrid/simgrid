/* Copyright (c) 2006, 2007, 2008, 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/dict.h"
#include "xbt/RngStream.h"
#include "simgrid/platf_interface.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_parse);
xbt_dynar_t sg_platf_host_cb_list = NULL;   // of sg_platf_host_cb_t
xbt_dynar_t sg_platf_host_link_cb_list = NULL;   // of sg_platf_host_link_cb_t
xbt_dynar_t sg_platf_link_cb_list = NULL;   // of sg_platf_link_cb_t
xbt_dynar_t sg_platf_router_cb_list = NULL; // of sg_platf_router_cb_t
xbt_dynar_t sg_platf_peer_cb_list = NULL; // of sg_platf_peer_cb_t
xbt_dynar_t sg_platf_cluster_cb_list = NULL; // of sg_platf_cluster_cb_t
xbt_dynar_t sg_platf_cabinet_cb_list = NULL; // of sg_platf_cluster_cb_t
xbt_dynar_t sg_platf_AS_begin_cb_list = NULL; //of sg_platf_AS_begin_cb_t
xbt_dynar_t sg_platf_AS_end_cb_list = NULL; //of void_f_void_t
xbt_dynar_t sg_platf_postparse_cb_list = NULL; // of void_f_void_t

xbt_dynar_t sg_platf_storage_cb_list = NULL; // of sg_platf_storage_cb_t
xbt_dynar_t sg_platf_storage_type_cb_list = NULL; // of sg_platf_storage_cb_t
xbt_dynar_t sg_platf_mstorage_cb_list = NULL; // of sg_platf_storage_cb_t
xbt_dynar_t sg_platf_mount_cb_list = NULL; // of sg_platf_storage_cb_t

static int surf_parse_models_setup_already_called;

/* one RngStream for the platform, to respect some statistic rules */
static RngStream sg_platf_rng_stream = NULL;

/** Module management function: creates all internal data structures */
void sg_platf_init(void) {

  //FIXME : Ugly, but useful...
  if(sg_platf_host_cb_list)
    return; //Already initialized, so do nothing...

  sg_platf_host_cb_list = xbt_dynar_new(sizeof(sg_platf_host_cb_t), NULL);
  sg_platf_host_link_cb_list = xbt_dynar_new(sizeof(sg_platf_host_link_cb_t), NULL);
  sg_platf_router_cb_list = xbt_dynar_new(sizeof(sg_platf_host_cb_t), NULL);
  sg_platf_link_cb_list = xbt_dynar_new(sizeof(sg_platf_host_cb_t), NULL);
  sg_platf_peer_cb_list = xbt_dynar_new(sizeof(sg_platf_peer_cb_t), NULL);
  sg_platf_cluster_cb_list = xbt_dynar_new(sizeof(sg_platf_cluster_cb_t), NULL);
  sg_platf_cabinet_cb_list = xbt_dynar_new(sizeof(sg_platf_cabinet_cb_t), NULL);
  sg_platf_postparse_cb_list = xbt_dynar_new(sizeof(sg_platf_link_cb_t),NULL);
  sg_platf_AS_begin_cb_list = xbt_dynar_new(sizeof(sg_platf_AS_begin_cb_t),NULL);
  sg_platf_AS_end_cb_list = xbt_dynar_new(sizeof(void_f_void_t),NULL);

  sg_platf_storage_cb_list = xbt_dynar_new(sizeof(sg_platf_storage_cb_t), NULL);
  sg_platf_storage_type_cb_list = xbt_dynar_new(sizeof(sg_platf_storage_cb_t), NULL);
  sg_platf_mstorage_cb_list = xbt_dynar_new(sizeof(sg_platf_storage_cb_t), NULL);
  sg_platf_mount_cb_list = xbt_dynar_new(sizeof(sg_platf_storage_cb_t), NULL);
}
/** Module management function: frees all internal data structures */
void sg_platf_exit(void) {
  xbt_dynar_free(&sg_platf_host_cb_list);
  xbt_dynar_free(&sg_platf_host_link_cb_list);
  xbt_dynar_free(&sg_platf_router_cb_list);
  xbt_dynar_free(&sg_platf_link_cb_list);
  xbt_dynar_free(&sg_platf_postparse_cb_list);
  xbt_dynar_free(&sg_platf_peer_cb_list);
  xbt_dynar_free(&sg_platf_cluster_cb_list);
  xbt_dynar_free(&sg_platf_cabinet_cb_list);
  xbt_dynar_free(&sg_platf_AS_begin_cb_list);
  xbt_dynar_free(&sg_platf_AS_end_cb_list);

  xbt_dynar_free(&sg_platf_storage_cb_list);
  xbt_dynar_free(&sg_platf_storage_type_cb_list);
  xbt_dynar_free(&sg_platf_mstorage_cb_list);
  xbt_dynar_free(&sg_platf_mount_cb_list);

  /* make sure that we will reinit the models while loading the platf once reinited */
  surf_parse_models_setup_already_called = 0;
}

void sg_platf_new_host(sg_platf_host_cbarg_t h){
  unsigned int iterator;
  sg_platf_host_cb_t fun;
  xbt_dynar_foreach(sg_platf_host_cb_list, iterator, fun) {
    fun(h);
  }
}
void sg_platf_new_host_link(sg_platf_host_link_cbarg_t h){
  unsigned int iterator;
  sg_platf_host_link_cb_t fun;
  xbt_dynar_foreach(sg_platf_host_link_cb_list, iterator, fun) {
    fun(h);
  }
}
void sg_platf_new_router(sg_platf_router_cbarg_t router) {
  unsigned int iterator;
  sg_platf_router_cb_t fun;
  xbt_dynar_foreach(sg_platf_router_cb_list, iterator, fun) {
    fun(router);
  }
}
void sg_platf_new_link(sg_platf_link_cbarg_t link){
  unsigned int iterator;
  sg_platf_link_cb_t fun;
  xbt_dynar_foreach(sg_platf_link_cb_list, iterator, fun) {
    fun(link);
  }
}
void sg_platf_new_peer(sg_platf_peer_cbarg_t peer){
  unsigned int iterator;
  sg_platf_peer_cb_t fun;
  xbt_dynar_foreach(sg_platf_peer_cb_list, iterator, fun) {
    fun(peer);
  }
}
void sg_platf_new_cluster(sg_platf_cluster_cbarg_t cluster){
  unsigned int iterator;
  sg_platf_cluster_cb_t fun;
  xbt_dynar_foreach(sg_platf_cluster_cb_list, iterator, fun) {
    fun(cluster);
  }
}
void sg_platf_new_cabinet(sg_platf_cabinet_cbarg_t cabinet){
  unsigned int iterator;
  sg_platf_cabinet_cb_t fun;
  xbt_dynar_foreach(sg_platf_cabinet_cb_list, iterator, fun) {
    fun(cabinet);
  }
}
void sg_platf_new_storage(sg_platf_storage_cbarg_t storage){
  unsigned int iterator;
  sg_platf_storage_cb_t fun;
  xbt_dynar_foreach(sg_platf_storage_cb_list, iterator, fun) {
    fun(storage);
  }
}
void sg_platf_new_storage_type(sg_platf_storage_type_cbarg_t storage_type){
  unsigned int iterator;
  sg_platf_storage_type_cb_t fun;
  xbt_dynar_foreach(sg_platf_storage_type_cb_list, iterator, fun) {
    fun(storage_type);
  }
}
void sg_platf_new_mstorage(sg_platf_mstorage_cbarg_t mstorage){
  unsigned int iterator;
  sg_platf_mstorage_cb_t fun;
  xbt_dynar_foreach(sg_platf_mstorage_cb_list, iterator, fun) {
    fun(mstorage);
  }
}
void sg_platf_new_mount(sg_platf_mount_cbarg_t mount){
  unsigned int iterator;
  sg_platf_mount_cb_t fun;
  xbt_dynar_foreach(sg_platf_mount_cb_list, iterator, fun) {
    fun(mount);
  }
}

void sg_platf_begin() { /* Do nothing: just for symmetry of user code */ }

void sg_platf_end() {
  unsigned int iterator;
  void_f_void_t fun;
  xbt_dynar_foreach(sg_platf_postparse_cb_list, iterator, fun) {
    fun();
  }
}

static int surf_parse_models_setup_already_called = 0;

void sg_platf_new_AS_begin(const char *id, int routing) {
  unsigned int iterator;
  sg_platf_AS_begin_cb_t fun;

  if (!surf_parse_models_setup_already_called && !xbt_dynar_is_empty(sg_platf_AS_begin_cb_list)) {
    /* Initialize the surf models. That must be done after we got all config, and before we need the models.
     * That is, after the last <config> tag, if any, and before the first of cluster|peer|AS|trace|trace_connect
     *
     * I'm not sure for <trace> and <trace_connect>, there may be a bug here
     * (FIXME: check it out by creating a file beginning with one of these tags)
     * but cluster and peer create ASes internally, so putting the code in there is ok.
     *
     * We are also guarding against xbt_dynar_length(sg_platf_AS_begin_cb_list) because we don't
     * want to initialize the models if we are parsing the file to get the deployment. That could happen if
     * the same file would be used for platf and deploy: it'd contain AS tags even during the deploy parsing.
     * Removing that guard would result of the models to get re-inited when parsing for deploy.
     */
    surf_parse_models_setup_already_called = 1;
    surf_config_models_setup();
  }

  xbt_dynar_foreach(sg_platf_AS_begin_cb_list, iterator, fun) {
    fun(id, routing);
  }
}

void sg_platf_new_AS_end() {
  unsigned int iterator;
  void_f_void_t fun;
  xbt_dynar_foreach(sg_platf_AS_end_cb_list, iterator, fun) {
    fun();
  }
}


void sg_platf_host_add_cb(sg_platf_host_cb_t fct) {
  xbt_dynar_push(sg_platf_host_cb_list, &fct);
}
void sg_platf_host_link_add_cb(sg_platf_host_link_cb_t fct) {
  xbt_dynar_push(sg_platf_host_link_cb_list, &fct);
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
void sg_platf_cluster_add_cb(sg_platf_cluster_cb_t fct) {
  xbt_dynar_push(sg_platf_cluster_cb_list, &fct);
}
void sg_platf_cabinet_add_cb(sg_platf_cabinet_cb_t fct) {
  xbt_dynar_push(sg_platf_cabinet_cb_list, &fct);
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
void sg_platf_storage_add_cb(sg_platf_storage_cb_t fct) {
  xbt_dynar_push(sg_platf_storage_cb_list, &fct);
}
void sg_platf_storage_type_add_cb(sg_platf_storage_type_cb_t fct) {
  xbt_dynar_push(sg_platf_storage_type_cb_list, &fct);
}
void sg_platf_mstorage_add_cb(sg_platf_mstorage_cb_t fct) {
  xbt_dynar_push(sg_platf_mstorage_cb_list, &fct);
}
void sg_platf_mount_add_cb(sg_platf_mount_cb_t fct) {
  xbt_dynar_push(sg_platf_mount_cb_list, &fct);
}


void sg_platf_rng_stream_init(unsigned long seed[6]) {
  RngStream_SetPackageSeed(seed);
  sg_platf_rng_stream = RngStream_CreateStream(NULL);
}

RngStream sg_platf_rng_stream_get(const char* id) {
  RngStream stream = NULL;
  unsigned int id_hash;

  stream = RngStream_CopyStream(sg_platf_rng_stream);
  id_hash = xbt_str_hash(id);
  RngStream_AdvanceState(stream, 0, (long)id_hash);

  return stream;
}
