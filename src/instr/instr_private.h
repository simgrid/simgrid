/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_PRIVATE_H_
#define INSTR_PRIVATE_H_

#include "simgrid_config.h"

#ifdef HAVE_TRACING

#define INSTR_DEFAULT_STR_SIZE 500

#include "instr/instr.h"
#include "msg/msg.h"
#include "simdag/private.h"
#include "simix/private.h"

typedef enum {
  TYPE_VARIABLE,
  TYPE_LINK,
  TYPE_CONTAINER,
  TYPE_STATE,
  TYPE_EVENT,
} e_entity_types;

typedef struct s_type *type_t;
typedef struct s_type {
  char *id;
  char *name;
  char *color;
  e_entity_types kind;
  struct s_type *father;
  xbt_dict_t children;
}s_type_t;

typedef enum {
  INSTR_HOST,
  INSTR_LINK,
  INSTR_ROUTER,
  INSTR_AS,
  INSTR_SMPI,
  INSTR_MSG_PROCESS,
  INSTR_MSG_TASK,
} e_container_types;

typedef struct s_container *container_t;
typedef struct s_container {
  char *name;     /* Unique name of this container */
  char *id;       /* Unique id of this container */
  type_t type;    /* Type of this container */
  int level;      /* Level in the hierarchy, root level is 0 */
  e_container_types kind; /* This container is of what kind */
  struct s_container *father;
  xbt_dict_t children;
}s_container_t;

extern xbt_dict_t created_categories;
extern double TRACE_last_timestamp_to_dump;

/* from paje.c */
void TRACE_paje_create_header(void);
void TRACE_paje_start(void);
void TRACE_paje_end(void);
void TRACE_paje_dump_buffer (int force);
void new_pajeDefineContainerType(type_t type);
void new_pajeDefineVariableType(type_t type);
void new_pajeDefineStateType(type_t type);
void new_pajeDefineEventType(type_t type);
void new_pajeDefineLinkType(type_t type, type_t source, type_t dest);
void new_pajeCreateContainer (container_t container);
void new_pajeDestroyContainer (container_t container);
void new_pajeSetVariable (double timestamp, container_t container, type_t type, double value);
void new_pajeAddVariable (double timestamp, container_t container, type_t type, double value);
void new_pajeSubVariable (double timestamp, container_t container, type_t type, double value);
void new_pajeSetState (double timestamp, container_t container, type_t type, const char *value);
void new_pajePushState (double timestamp, container_t container, type_t type, const char *value);
void new_pajePopState (double timestamp, container_t container, type_t type);
void new_pajeStartLink (double timestamp, container_t container, type_t type, container_t sourceContainer, const char *value, const char *key);
void new_pajeEndLink (double timestamp, container_t container, type_t type, container_t destContainer, const char *value, const char *key);
void new_pajeNewEvent (double timestamp, container_t container, type_t type, const char *value);

/* declaration of instrumentation functions from msg_task_instr.c */
char *TRACE_task_container(m_task_t task, char *output, int len);
void TRACE_msg_task_create(m_task_t task);
void TRACE_msg_task_execute_start(m_task_t task);
void TRACE_msg_task_execute_end(m_task_t task);
void TRACE_msg_task_destroy(m_task_t task);
void TRACE_msg_task_get_start(void);
void TRACE_msg_task_get_end(double start_time, m_task_t task);
int TRACE_msg_task_put_start(m_task_t task);    //returns TRUE if the task_put_end must be called
void TRACE_msg_task_put_end(void);

/* declaration of instrumentation functions from msg_process_instr.c */
char *TRACE_process_alias_container(m_process_t process, m_host_t host,
                                    char *output, int len);
char *TRACE_process_container(m_process_t process, char *output, int len);
void TRACE_msg_process_change_host(m_process_t process, m_host_t old_host,
                                   m_host_t new_host);
void TRACE_msg_process_kill(m_process_t process);
void TRACE_msg_process_suspend(m_process_t process);
void TRACE_msg_process_resume(m_process_t process);
void TRACE_msg_process_sleep_in(m_process_t process);   //called from msg/gos.c
void TRACE_msg_process_sleep_out(m_process_t process);
void TRACE_msg_process_end(m_process_t process);

/* declaration of instrumentation functions from msg_volume.c */
void TRACE_msg_volume_start(m_task_t task);
void TRACE_msg_volume_finish(m_task_t task);

/* from smx.c */
void TRACE_smx_host_execute(smx_action_t act);
void TRACE_smx_action_communicate(smx_action_t act, smx_process_t proc);
void TRACE_smx_action_destroy(smx_action_t act);

/* from surf_instr.c */
void TRACE_surf_alloc(void);
void TRACE_surf_release(void);
void TRACE_surf_host_set_power(double date, const char *resource, double power);
void TRACE_surf_link_set_bandwidth(double date, const char *resource, double bandwidth);
void TRACE_surf_link_set_latency(double date, const char *resource, double latency);
void TRACE_surf_action(surf_action_t surf_action, const char *category);

//for tracing gtnets
void TRACE_surf_gtnets_communicate(void *action, int src, int dst);
int TRACE_surf_gtnets_get_src(void *action);
int TRACE_surf_gtnets_get_dst(void *action);
void TRACE_surf_gtnets_destroy(void *action);

/* from smpi_instr.c */
void TRACE_internal_smpi_set_category (const char *category);
const char *TRACE_internal_smpi_get_category (void);
void TRACE_smpi_alloc(void);
void TRACE_smpi_release(void);
void TRACE_smpi_init(int rank);
void TRACE_smpi_finalize(int rank);
void TRACE_smpi_start(void);
void TRACE_smpi_collective_in(int rank, int root, const char *operation);
void TRACE_smpi_collective_out(int rank, int root, const char *operation);
void TRACE_smpi_ptp_in(int rank, int src, int dst, const char *operation);
void TRACE_smpi_ptp_out(int rank, int src, int dst, const char *operation);
void TRACE_smpi_send(int rank, int src, int dst);
void TRACE_smpi_recv(int rank, int src, int dst);

/* from instr_config.c */
int TRACE_start (void);
int TRACE_end (void);
void TRACE_activate (void);
void TRACE_desactivate (void);
int TRACE_is_active (void);
int TRACE_is_enabled(void);
int TRACE_is_configured(void);
int TRACE_smpi_is_enabled(void);
int TRACE_smpi_is_grouped(void);
int TRACE_categorized (void);
int TRACE_uncategorized (void);
int TRACE_msg_task_is_enabled(void);
int TRACE_msg_process_is_enabled(void);
int TRACE_msg_volume_is_enabled(void);
char *TRACE_get_filename(void);
char *TRACE_get_platform_method(void);
char *TRACE_get_triva_uncat_conf (void);
char *TRACE_get_triva_cat_conf (void);
void TRACE_global_init(int *argc, char **argv);
void TRACE_help(int detailed);
void TRACE_generate_triva_uncat_conf (void);
void TRACE_generate_triva_cat_conf (void);

/* from resource_utilization.c */
void TRACE_surf_host_set_utilization(const char *resource,
                                     smx_action_t smx_action,
                                     surf_action_t surf_action,
                                     double value, double now,
                                     double delta);
void TRACE_surf_link_set_utilization(const char *resource, smx_action_t smx_action,
                                     surf_action_t surf_action,
                                     double value, double now,
                                     double delta);
void TRACE_surf_resource_utilization_start(smx_action_t action);
void TRACE_surf_resource_utilization_event(smx_action_t action, double now,
                                           double delta,
                                           const char *variable,
                                           const char *resource,
                                           double value);
void TRACE_surf_resource_utilization_end(smx_action_t action);
void TRACE_surf_resource_utilization_alloc(void);
void TRACE_surf_resource_utilization_release(void);

/* sd_instr.c */
void TRACE_sd_task_create(SD_task_t task);
void TRACE_sd_task_destroy(SD_task_t task);

/* instr_paje.c */
extern xbt_dict_t trivaNodeTypes;
extern xbt_dict_t trivaEdgeTypes;
container_t newContainer (const char *name, e_container_types kind, container_t father);
container_t getContainer (const char *name);
container_t getContainerByName (const char *name);
char *getContainerIdByName (const char *name);
char *getVariableTypeIdByName (const char *name, type_t father);
container_t getRootContainer (void);
void instr_paje_init (container_t root);
type_t getRootType (void);
type_t getContainerType (const char *name, type_t father);
type_t getEventType (const char *name, const char *color, type_t father);
type_t getVariableType (const char *name, const char *color, type_t father);
type_t getLinkType (const char *name, type_t father, type_t source, type_t dest);
type_t getStateType (const char *name, type_t father);
type_t getType (const char *name);
void destroyContainer (container_t container);
void destroyAllContainers (void);

/* instr_routing.c */
void instr_routing_define_callbacks (void);
void instr_new_user_variable_type (const char *new_typename, const char *color);
void instr_new_user_link_variable_type  (const char *new_typename, const char *color);
void instr_new_user_host_variable_type  (const char *new_typename, const char *color);
int instr_platform_traced (void);

#endif /* HAVE_TRACING */
#endif /* INSTR_PRIVATE_H_ */
