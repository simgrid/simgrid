/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/dict.h"
#include "xbt/RngStream.h"
#include "simgrid/platf_interface.h"
#include "surf/surf_routing.h"
#include "surf/surf.h"

#include "src/simix/smx_private.h"

#include "cpu_interface.hpp"
#include "host_interface.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_parse);
xbt_dynar_t sg_platf_link_cb_list = NULL;   // of sg_platf_link_cb_t
xbt_dynar_t sg_platf_cluster_cb_list = NULL; // of sg_platf_cluster_cb_t
xbt_dynar_t sg_platf_postparse_cb_list = NULL; // of void_f_void_t

xbt_dynar_t sg_platf_ASroute_cb_list = NULL; // of sg_platf_ASroute_cb_t
xbt_dynar_t sg_platf_bypassRoute_cb_list = NULL; // of sg_platf_bypassRoute_cb_t
xbt_dynar_t sg_platf_bypassASroute_cb_list = NULL; // of sg_platf_bypassASroute_cb_t

xbt_dynar_t sg_platf_trace_cb_list = NULL;

/* ***************************************** */
/* TUTORIAL: New TAG                         */

xbt_dynar_t sg_platf_gpu_cb_list = NULL;
/* ***************************************** */


static int surf_parse_models_setup_already_called = 0;

/* one RngStream for the platform, to respect some statistic rules */
static RngStream sg_platf_rng_stream = NULL;

/** Module management function: creates all internal data structures */
void sg_platf_init(void) {

  //FIXME : Ugly, but useful...
  if (sg_platf_gpu_cb_list)
    return; //Already initialized, so do nothing...

  sg_platf_link_cb_list = xbt_dynar_new(sizeof(sg_platf_link_cb_t), NULL);
  sg_platf_cluster_cb_list = xbt_dynar_new(sizeof(sg_platf_cluster_cb_t), NULL);
  sg_platf_postparse_cb_list = xbt_dynar_new(sizeof(sg_platf_link_cb_t),NULL);

  sg_platf_ASroute_cb_list = xbt_dynar_new(sizeof(sg_platf_route_cb_t), NULL);
  sg_platf_bypassRoute_cb_list = xbt_dynar_new(sizeof(sg_platf_route_cb_t), NULL);
  sg_platf_bypassASroute_cb_list = xbt_dynar_new(sizeof(sg_platf_route_cb_t), NULL);

  sg_platf_trace_cb_list = xbt_dynar_new(sizeof(sg_platf_trace_cb_t), NULL);

  /* ***************************************** */
  /* TUTORIAL: New TAG                         */

  sg_platf_gpu_cb_list = xbt_dynar_new(sizeof(sg_platf_gpu_cb_t), NULL);
  /* ***************************************** */
}
/** Module management function: frees all internal data structures */
void sg_platf_exit(void) {
  xbt_dynar_free(&sg_platf_link_cb_list);
  xbt_dynar_free(&sg_platf_postparse_cb_list);
  xbt_dynar_free(&sg_platf_cluster_cb_list);

  xbt_dynar_free(&sg_platf_trace_cb_list);

  xbt_dynar_free(&sg_platf_ASroute_cb_list);
  xbt_dynar_free(&sg_platf_bypassRoute_cb_list);
  xbt_dynar_free(&sg_platf_bypassASroute_cb_list);

  /* ***************************************** */
  /* TUTORIAL: New TAG                         */

  xbt_dynar_free(&sg_platf_gpu_cb_list);

  /* ***************************************** */

  /* make sure that we will reinit the models while loading the platf once reinited */
  surf_parse_models_setup_already_called = 0;
}

void sg_platf_new_host(sg_platf_host_cbarg_t host)
{
  xbt_assert(! sg_host_by_name(host->id),
		     "Refusing to create a second host named '%s'.", host->id);

  RoutingEdge *net = NULL;
  As* current_routing = routing_get_current();
  if (current_routing)
    net = routing_add_host(current_routing, host);

  Cpu *cpu = surf_cpu_model_pm->createCpu(
        host->id,
        host->power_peak,
        host->pstate,
        host->power_scale,
        host->power_trace,
        host->core_amount,
        host->initial_state,
        host->state_trace,
        host->properties);
  surf_host_model->createHost(host->id, net, cpu);

  if (TRACE_is_enabled() && TRACE_needs_platform())
    sg_instr_new_host(host);
}

/**
 * \brief Add a "router" to the network element list
 */
void sg_platf_new_router(sg_platf_router_cbarg_t router)
{
  As* current_routing = routing_get_current();

  if (current_routing->p_hierarchy == SURF_ROUTING_NULL)
    current_routing->p_hierarchy = SURF_ROUTING_BASE;
  xbt_assert(!xbt_lib_get_or_null(as_router_lib, router->id, ROUTING_ASR_LEVEL),
             "Reading a router, processing unit \"%s\" already exists",
             router->id);

  RoutingEdge *info = new RoutingEdgeImpl(xbt_strdup(router->id),
                                            -1,
                                            SURF_NETWORK_ELEMENT_ROUTER,
                                            current_routing);
  info->setId(current_routing->parsePU(info));
  xbt_lib_set(as_router_lib, router->id, ROUTING_ASR_LEVEL, (void *) info);
  XBT_DEBUG("Having set name '%s' id '%d'", router->id, info->getId());
  routingEdgeCreatedCallbacks(info);

  if (router->coord && strcmp(router->coord, "")) {
    unsigned int cursor;
    char*str;

    if (!COORD_ASR_LEVEL)
      xbt_die ("To use host coordinates, please add --cfg=network/coordinates:yes to your command line");
    /* Pre-parse the host coordinates */
    xbt_dynar_t ctn_str = xbt_str_split_str(router->coord, " ");
    xbt_dynar_t ctn = xbt_dynar_new(sizeof(double),NULL);
    xbt_dynar_foreach(ctn_str,cursor, str) {
      double val = atof(str);
      xbt_dynar_push(ctn,&val);
    }
    xbt_dynar_shrink(ctn, 0);
    xbt_dynar_free(&ctn_str);
    xbt_lib_set(as_router_lib, router->id, COORD_ASR_LEVEL, (void *) ctn);
    XBT_DEBUG("Having set router coordinates for '%s'",router->id);
  }

  if (TRACE_is_enabled())
    sg_instr_new_router(router);
}

void sg_platf_new_link(sg_platf_link_cbarg_t link){
  unsigned int iterator;
  sg_platf_link_cb_t fun;
  xbt_dynar_foreach(sg_platf_link_cb_list, iterator, fun) {
    fun(link);
  }
}

void sg_platf_new_cluster(sg_platf_cluster_cbarg_t cluster){
  unsigned int iterator;
  sg_platf_cluster_cb_t fun;
  xbt_dynar_foreach(sg_platf_cluster_cb_list, iterator, fun) {
    fun(cluster);
  }
}

void sg_platf_new_storage(sg_platf_storage_cbarg_t storage)
{
  xbt_assert(!xbt_lib_get_or_null(storage_lib, storage->id,ROUTING_STORAGE_LEVEL),
               "Reading a storage, processing unit \"%s\" already exists", storage->id);

  // Verification of an existing type_id
#ifndef NDEBUG
  void* storage_type = xbt_lib_get_or_null(storage_type_lib, storage->type_id,ROUTING_STORAGE_TYPE_LEVEL);
#endif
  xbt_assert(storage_type,"Reading a storage, type id \"%s\" does not exists", storage->type_id);

  XBT_DEBUG("ROUTING Create a storage name '%s' with type_id '%s' and content '%s'",
      storage->id,
      storage->type_id,
      storage->content);

  xbt_lib_set(storage_lib,
      storage->id,
      ROUTING_STORAGE_LEVEL,
      (void *) xbt_strdup(storage->type_id));

  void* stype = xbt_lib_get_or_null(storage_type_lib,
                                    storage->type_id,
                                    ROUTING_STORAGE_TYPE_LEVEL);
  if(!stype) xbt_die("No storage type '%s'",storage->type_id);

  // if storage content is not specified use the content of storage_type if exist
  if(!strcmp(storage->content,"") && strcmp(((storage_type_t) stype)->content,"")){
    storage->content = ((storage_type_t) stype)->content;
    storage->content_type = ((storage_type_t) stype)->content_type;
    XBT_DEBUG("For disk '%s' content is empty, inherit the content (of type %s) from storage type '%s' ",
        storage->id,((storage_type_t) stype)->content_type,
        ((storage_type_t) stype)->type_id);
  }

  XBT_DEBUG("SURF storage create resource\n\t\tid '%s'\n\t\ttype '%s' "
      "\n\t\tmodel '%s' \n\t\tcontent '%s'\n\t\tcontent_type '%s' "
      "\n\t\tproperties '%p''\n",
      storage->id,
      ((storage_type_t) stype)->model,
      ((storage_type_t) stype)->type_id,
      storage->content,
      storage->content_type,
      storage->properties);

  surf_storage_model->createStorage(storage->id,
                                     ((storage_type_t) stype)->type_id,
                                     storage->content,
                                     storage->content_type,
                                     storage->properties,
                                     storage->attach);
}
void sg_platf_new_storage_type(sg_platf_storage_type_cbarg_t storage_type){

  xbt_assert(!xbt_lib_get_or_null(storage_type_lib, storage_type->id,ROUTING_STORAGE_TYPE_LEVEL),
               "Reading a storage type, processing unit \"%s\" already exists", storage_type->id);

  storage_type_t stype = xbt_new0(s_storage_type_t, 1);
  stype->model = xbt_strdup(storage_type->model);
  stype->properties = storage_type->properties;
  stype->content = xbt_strdup(storage_type->content);
  stype->content_type = xbt_strdup(storage_type->content_type);
  stype->type_id = xbt_strdup(storage_type->id);
  stype->size = storage_type->size;
  stype->model_properties = storage_type->model_properties;

  XBT_DEBUG("ROUTING Create a storage type id '%s' with model '%s', "
      "content '%s', and content_type '%s'",
      stype->type_id,
      stype->model,
      storage_type->content,
      storage_type->content_type);

  xbt_lib_set(storage_type_lib,
      stype->type_id,
      ROUTING_STORAGE_TYPE_LEVEL,
      (void *) stype);
}
void sg_platf_new_mstorage(sg_platf_mstorage_cbarg_t mstorage)
{
  THROW_UNIMPLEMENTED;
//  mount_t mnt = xbt_new0(s_mount_t, 1);
//  mnt->id = xbt_strdup(mstorage->type_id);
//  mnt->name = xbt_strdup(mstorage->name);
//
//  if(!mount_list){
//    XBT_DEBUG("Creata a Mount list for %s",A_surfxml_host_id);
//    mount_list = xbt_dynar_new(sizeof(char *), NULL);
//  }
//  xbt_dynar_push(mount_list,(void *) mnt);
//  free(mnt->id);
//  free(mnt->name);
//  xbt_free(mnt);
//  XBT_DEBUG("ROUTING Mount a storage name '%s' with type_id '%s'",mstorage->name, mstorage->id);
}

static void mount_free(void *p)
{
  mount_t mnt = (mount_t) p;
  xbt_free(mnt->name);
}

void sg_platf_new_mount(sg_platf_mount_cbarg_t mount){
  // Verification of an existing storage
#ifndef NDEBUG
  void* storage = xbt_lib_get_or_null(storage_lib, mount->storageId, ROUTING_STORAGE_LEVEL);
#endif
  xbt_assert(storage,"Disk id \"%s\" does not exists", mount->storageId);

  XBT_DEBUG("ROUTING Mount '%s' on '%s'",mount->storageId, mount->name);

  s_mount_t mnt;
  mnt.storage = surf_storage_resource_priv(surf_storage_resource_by_name(mount->storageId));
  mnt.name = xbt_strdup(mount->name);

  if(!mount_list){
    XBT_DEBUG("Create a Mount list for %s",A_surfxml_host_id);
    mount_list = xbt_dynar_new(sizeof(s_mount_t), mount_free);
  }
  xbt_dynar_push(mount_list, &mnt);
}

void sg_platf_new_route(sg_platf_route_cbarg_t route)
{
  routing_get_current()->parseRoute(route);
}

void sg_platf_new_ASroute(sg_platf_route_cbarg_t ASroute) {
  unsigned int iterator;
  sg_platf_route_cb_t fun;
  xbt_dynar_foreach(sg_platf_ASroute_cb_list, iterator, fun) {
    fun(ASroute);
  }
}
void sg_platf_new_bypassRoute(sg_platf_route_cbarg_t bypassRoute) {
  unsigned int iterator;
  sg_platf_route_cb_t fun;
  xbt_dynar_foreach(sg_platf_bypassRoute_cb_list, iterator, fun) {
    fun(bypassRoute);
  }
}
void sg_platf_new_bypassASroute(sg_platf_route_cbarg_t bypassASroute) {
  unsigned int iterator;
  sg_platf_route_cb_t fun;
  xbt_dynar_foreach(sg_platf_bypassASroute_cb_list, iterator, fun) {
    fun(bypassASroute);
  }
}
void sg_platf_new_trace(sg_platf_trace_cbarg_t trace) {
  unsigned int iterator;
  sg_platf_trace_cb_t fun;
  xbt_dynar_foreach(sg_platf_trace_cb_list, iterator, fun) {
    fun(trace);
  }
}

void sg_platf_new_process(sg_platf_process_cbarg_t process)
{
  if (!simix_global)
    xbt_die("Cannot create process without SIMIX.");

  sg_host_t host = sg_host_by_name(process->host);
  if (!host)
    THROWF(arg_error, 0, "Host '%s' unknown", process->host);
  xbt_main_func_t parse_code = SIMIX_get_registered_function(process->function);
  xbt_assert(parse_code, "Function '%s' unknown", process->function);

  double start_time = process->start_time;
  double kill_time  = process->kill_time;
  int auto_restart = process->on_failure == SURF_PROCESS_ON_FAILURE_DIE ? 0 : 1;

  smx_process_arg_t arg = NULL;
  smx_process_t process_created = NULL;

  arg = xbt_new0(s_smx_process_arg_t, 1);
  arg->code = parse_code;
  arg->data = NULL;
  arg->hostname = sg_host_get_name(host);
  arg->argc = process->argc;
  arg->argv = xbt_new(char *,process->argc);
  int i;
  for (i=0; i<process->argc; i++)
    arg->argv[i] = xbt_strdup(process->argv[i]);
  arg->name = xbt_strdup(arg->argv[0]);
  arg->kill_time = kill_time;
  arg->properties = current_property_set;
  if (!sg_host_simix(host)->boot_processes) {
    sg_host_simix(host)->boot_processes = xbt_dynar_new(sizeof(smx_process_arg_t), _SIMIX_host_free_process_arg);
  }
  xbt_dynar_push_as(sg_host_simix(host)->boot_processes,smx_process_arg_t,arg);

  if (start_time > SIMIX_get_clock()) {
    arg = xbt_new0(s_smx_process_arg_t, 1);
    arg->name = (char*)(process->argv)[0];
    arg->code = parse_code;
    arg->data = NULL;
    arg->hostname = sg_host_get_name(host);
    arg->argc = process->argc;
    arg->argv = (char**)(process->argv);
    arg->kill_time = kill_time;
    arg->properties = current_property_set;

    XBT_DEBUG("Process %s(%s) will be started at time %f", arg->name,
           arg->hostname, start_time);
    SIMIX_timer_set(start_time, [](void* arg) {
      SIMIX_process_create_from_wrapper((smx_process_arg_t) arg);
    }, arg);
  } else {                      // start_time <= SIMIX_get_clock()
    XBT_DEBUG("Starting Process %s(%s) right now", process->argv[0], sg_host_get_name(host));

    if (simix_global->create_process_function)
      process_created = simix_global->create_process_function(
                                            (char*)(process->argv)[0],
                                            parse_code,
                                            NULL,
                                            sg_host_get_name(host),
                                            kill_time,
                                            process->argc,
                                            (char**)(process->argv),
                                            current_property_set,
                                            auto_restart, NULL);
    else
      process_created = simcall_process_create((char*)(process->argv)[0], parse_code, NULL, sg_host_get_name(host), kill_time, process->argc,
          (char**)process->argv, current_property_set,auto_restart);

    /* verify if process has been created (won't be the case if the host is currently dead, but that's fine) */
    if (!process_created) {
      return;
    }
  }
  current_property_set = NULL;
}

void sg_platf_route_begin (sg_platf_route_cbarg_t route){
  route->link_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
}
void sg_platf_ASroute_begin (sg_platf_route_cbarg_t ASroute){
  ASroute->link_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
}

void sg_platf_route_end (sg_platf_route_cbarg_t route){
  sg_platf_new_route(route);
}
void sg_platf_ASroute_end (sg_platf_route_cbarg_t ASroute){
  sg_platf_new_ASroute(ASroute);
}

void sg_platf_route_add_link (const char* link_id, sg_platf_route_cbarg_t route){
  char *link_name = xbt_strdup(link_id);
  xbt_dynar_push(route->link_list, &link_name);
}
void sg_platf_ASroute_add_link (const char* link_id, sg_platf_route_cbarg_t ASroute){
  char *link_name = xbt_strdup(link_id);
  xbt_dynar_push(ASroute->link_list, &link_name);
}

void sg_platf_begin() { /* Do nothing: just for symmetry of user code */ }

void sg_platf_end() {
  unsigned int iterator;
  void_f_void_t fun;
  xbt_dynar_foreach(sg_platf_postparse_cb_list, iterator, fun) {
    fun();
  }
}

void sg_platf_new_AS_begin(sg_platf_AS_cbarg_t AS)
{
  if (!surf_parse_models_setup_already_called) {
    /* Initialize the surf models. That must be done after we got all config, and before we need the models.
     * That is, after the last <config> tag, if any, and before the first of cluster|peer|AS|trace|trace_connect
     *
     * I'm not sure for <trace> and <trace_connect>, there may be a bug here
     * (FIXME: check it out by creating a file beginning with one of these tags)
     * but cluster and peer create ASes internally, so putting the code in there is ok.
     *
     * TODO, There used to be a guard protecting here against
     * xbt_dynar_length(sg_platf_AS_begin_cb_list) because we don't want to
     * initialize the models if we are parsing the file to get the deployment.
     * That could happen if the same file would be used for platf and deploy:
     * it'd contain AS tags even during the deploy parsing. Removing that guard
     * would result of the models to get re-inited when parsing for deploy.
     * Currently using the same file for platform and deployment is broken
     * however. This guard will have to ba adapted in order to make this feature
     * work again.
     */
    surf_parse_models_setup_already_called = 1;
    surf_config_models_setup();
  }

  routing_AS_begin(AS);
  if (TRACE_is_enabled())
    sg_instr_AS_begin(AS);
}

void sg_platf_new_AS_end()
{
  routing_AS_end();
  if (TRACE_is_enabled())
    sg_instr_AS_end();
}

/* ***************************************** */
/* TUTORIAL: New TAG                         */

void sg_platf_new_gpu(sg_platf_gpu_cbarg_t gpu) {
  unsigned int iterator;
  void_f_void_t fun;
  xbt_dynar_foreach(sg_platf_gpu_cb_list, iterator, fun) {
    fun();
  }
}

void sg_platf_gpu_add_cb(sg_platf_gpu_cb_t fct) {
  xbt_dynar_push(sg_platf_gpu_cb_list, &fct);
}

/* ***************************************** */

void sg_platf_link_add_cb(sg_platf_link_cb_t fct) {
  xbt_dynar_push(sg_platf_link_cb_list, &fct);
}
void sg_platf_cluster_add_cb(sg_platf_cluster_cb_t fct) {
  xbt_dynar_push(sg_platf_cluster_cb_list, &fct);
}
void sg_platf_postparse_add_cb(void_f_void_t fct) {
  xbt_dynar_push(sg_platf_postparse_cb_list, &fct);
}
void sg_platf_ASroute_add_cb(sg_platf_route_cb_t fct) {
  xbt_dynar_push(sg_platf_ASroute_cb_list, &fct);
}
void sg_platf_bypassRoute_add_cb(sg_platf_route_cb_t fct) {
  xbt_dynar_push(sg_platf_bypassRoute_cb_list, &fct);
}
void sg_platf_bypassASroute_add_cb(sg_platf_route_cb_t fct) {
  xbt_dynar_push(sg_platf_bypassASroute_cb_list, &fct);
}
void sg_platf_trace_add_cb(sg_platf_trace_cb_t fct) {
  xbt_dynar_push(sg_platf_trace_cb_list, &fct);
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
