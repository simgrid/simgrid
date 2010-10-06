/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_PRIVATE_H_
#define INSTR_PRIVATE_H_

#include "simgrid_config.h"

#ifdef HAVE_TRACING

extern int tracing_active; /* declared in paje.c */

#define IS_TRACING			  (tracing_active)
#define IS_TRACED(n)          (n->category)
#define IS_TRACING_TASKS      (TRACE_msg_task_is_enabled())
#define IS_TRACING_PLATFORM   (TRACE_platform_is_enabled())
#define IS_TRACING_PROCESSES  (TRACE_msg_process_is_enabled())
#define IS_TRACING_VOLUME     (TRACE_msg_volume_is_enabled())
#define IS_TRACING_SMPI       (TRACE_smpi_is_enabled())

#include "instr/instr.h"
#include "msg/msg.h"
#include "simix/private.h"

/* from paje.c */
void TRACE_paje_create_header(void);
void TRACE_paje_start (FILE *file);
FILE *TRACE_paje_end (void);
void pajeDefineContainerType(const char *alias, const char *containerType, const char *name);
void pajeDefineStateType(const char *alias, const char *containerType, const char *name);
void pajeDefineEventType(const char *alias, const char *containerType, const char *name);
void pajeDefineLinkType(const char *alias, const char *containerType, const char *sourceContainerType, const char *destContainerType, const char *name);
void pajeCreateContainer(double time, const char *alias, const char *type, const char *container, const char *name);
void pajeDestroyContainer (double time, const char *type, const char *container);
void pajeSetState (double time, const char *entityType, const char *container, const char *value);
void pajePushState (double time, const char *entityType, const char *container, const char *value);
void pajePopState (double time, const char *entityType, const char *container);
void pajeStartLink (double time, const char *entityType, const char *container, const char *value, const char *sourceContainer, const char *key);
void pajeStartLinkWithVolume (double time, const char *entityType, const char *container, const char *value, const char *sourceContainer, const char *key, double volume);
void pajeEndLink (double time, const char *entityType, const char *container, const char *value, const char *destContainer, const char *key);
void pajeDefineVariableType(const char *alias, const char *containerType, const char *name);
void pajeSetVariable (double time, const char *entityType, const char *container, const char *value);
void pajeAddVariable (double time, const char *entityType, const char *container, const char *value);
void pajeSubVariable (double time, const char *entityType, const char *container, const char *value);
void pajeNewEvent (double time, const char *entityType, const char *container, const char *value);

/* from general.c */
char *TRACE_paje_msg_container (m_task_t task, char *host, char *output, int len);
char *TRACE_paje_smx_container (smx_action_t action, int seqnumber, char *host, char *output, int len);
char *TRACE_paje_surf_container (void *action, int seqnumber, char *output, int len);
char *TRACE_host_container (m_host_t host, char *output, int len);
char *TRACE_task_container (m_task_t task, char *output, int len);
char *TRACE_process_container (m_process_t process, char *output, int len);
char *TRACE_process_alias_container (m_process_t process, m_host_t host, char *output, int len);
char *TRACE_task_alias_container (m_task_t task, m_process_t process, m_host_t host, char *output, int len);

/* from categories.c */
void TRACE_category_alloc (void);
void TRACE_category_release (void);
void TRACE_category_set (smx_process_t proc, const char *category);
char *TRACE_category_get (smx_process_t proc);
void TRACE_category_unset (smx_process_t proc);
void TRACE_msg_category_set (smx_process_t proc, m_task_t task);

/* declaration of instrumentation functions from msg_task_instr.c */
void TRACE_msg_task_alloc (void);
void TRACE_msg_task_release (void);
void TRACE_msg_task_create (m_task_t task);
void TRACE_msg_task_execute_start (m_task_t task);
void TRACE_msg_task_execute_end (m_task_t task);
void TRACE_msg_task_destroy (m_task_t task);
void TRACE_msg_task_get_start(void);
void TRACE_msg_task_get_end (double start_time, m_task_t task);
int TRACE_msg_task_put_start (m_task_t task); //returns TRUE if the task_put_end must be called
void TRACE_msg_task_put_end (void);

/* declaration of instrumentation functions from msg_process_instr.c */
void TRACE_msg_process_alloc (void);
void TRACE_msg_process_release (void);
void TRACE_msg_process_change_host (m_process_t process, m_host_t old_host, m_host_t new_host);
void TRACE_msg_process_kill (m_process_t process);
void TRACE_msg_process_suspend (m_process_t process);
void TRACE_msg_process_resume (m_process_t process);
void TRACE_msg_process_sleep_in (m_process_t process); //called from msg/gos.c
void TRACE_msg_process_sleep_out (m_process_t process);
void TRACE_msg_process_end (m_process_t process);

/* declaration of instrumentation functions from msg_volume.c */
void TRACE_msg_volume_start (m_task_t task);
void TRACE_msg_volume_finish (m_task_t task);

/* from smx.c */
void TRACE_smx_action_execute (smx_action_t act);
void TRACE_smx_action_communicate (smx_action_t act, smx_process_t proc);
void TRACE_smx_action_destroy (smx_action_t act);

/* from surf.c */
void TRACE_surf_alloc (void);
void TRACE_surf_release (void);
void TRACE_surf_host_declaration (char *name, double power);
void TRACE_surf_host_set_power (double date, char *resource, double power);
void TRACE_surf_host_define_id (const char *name, int host_id);
void TRACE_surf_host_vivaldi_parse (char *host, double x, double y, double h);
void TRACE_surf_link_declaration (void *link, char *name, double bw, double lat);
void TRACE_surf_link_set_bandwidth (double date, void *link, double bandwidth);
void TRACE_surf_link_set_latency (double date, void *link, double latency);
void TRACE_surf_save_onelink (void);
int TRACE_surf_link_is_traced (void *link);

//for tracing gtnets
void TRACE_surf_gtnets_communicate (void *action, int src, int dst);
int TRACE_surf_gtnets_get_src (void *action);
int TRACE_surf_gtnets_get_dst (void *action);
void TRACE_surf_gtnets_destroy (void *action);

/* from smpi_instr.c */
void TRACE_smpi_alloc (void);
void TRACE_smpi_release (void);
void TRACE_smpi_init (int rank);
void TRACE_smpi_finalize (int rank);
void TRACE_smpi_start (void);
void TRACE_smpi_collective_in (int rank, int root, const char *operation);
void TRACE_smpi_collective_out (int rank, int root, const char *operation);
void TRACE_smpi_ptp_in (int rank, int src, int dst, const char *operation);
void TRACE_smpi_ptp_out (int rank, int src, int dst, const char *operation);
void TRACE_smpi_send (int rank, int src, int dst);
void TRACE_smpi_recv (int rank, int src, int dst);

/* from instr_config.c */
int TRACE_is_configured (void);
int TRACE_smpi_is_enabled (void);
int TRACE_platform_is_enabled (void);
int TRACE_msg_task_is_enabled (void);
int TRACE_msg_process_is_enabled (void);
int TRACE_msg_volume_is_enabled (void);
char *TRACE_get_filename (void);
char *TRACE_get_platform_method (void);
void TRACE_global_init(int *argc, char **argv);

/* from resource_utilization.c */
void TRACE_surf_host_set_utilization (const char *name, smx_action_t smx_action, double value, double now, double delta);
void TRACE_surf_link_set_utilization (void *link, smx_action_t smx_action, double value, double now, double delta);
void TRACE_surf_resource_utilization_start (smx_action_t action);
void TRACE_surf_resource_utilization_event (smx_action_t action, double now, double delta, const char *variable, const char *resource, double value);
void TRACE_surf_resource_utilization_end (smx_action_t action);
void TRACE_surf_resource_utilization_alloc (void);
void TRACE_surf_resource_utilization_release (void);

#endif

#endif /* PRIVATE_H_ */
