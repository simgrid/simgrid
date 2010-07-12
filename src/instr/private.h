/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_PRIVATE_H_
#define INSTR_PRIVATE_H_

#include "simgrid_config.h"

#ifdef HAVE_TRACING

extern int tracing_active; /* declared in paje.c */
extern int trace_mask; /* declared in interface.c */

#define IS_TRACING			  (tracing_active)
#define IS_TRACED(n)          (n->category)
#define IS_TRACING_TASKS      ((TRACE_TASK)&trace_mask)
#define IS_TRACING_PLATFORM   ((TRACE_PLATFORM)&trace_mask)
#define IS_TRACING_PROCESSES  ((TRACE_PROCESS)&trace_mask)
#define IS_TRACING_VOLUME     ((TRACE_VOLUME)&trace_mask)

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
void pajeCreateContainerWithPower (double time, const char *alias, const char *type, const char *container, const char *name, double power);
void pajeCreateContainerWithBandwidthLatency (double time, const char *alias, const char *type, const char *container, const char *name, double bw, double lat);
void pajeCreateContainerWithBandwidthLatencySrcDst (double time, const char *alias, const char *type, const char *container, const char *name, double bw, double lat, const char *src, const char *dst);
void pajeDestroyContainer (double time, const char *type, const char *container);
void pajeSetState (double time, const char *entityType, const char *container, const char *value);
void pajeSetStateWithPowerUsed (double time, const char *entityType, const char *container, const char *value, double powerUsed);
void pajeSetStateWithHost (double time, const char *entityType, const char *container, const char *value, const char *host);
void pajePushState (double time, const char *entityType, const char *container, const char *value);
void pajePushStateWithHost (double time, const char *entityType, const char *container, const char *value, const char *host);
void pajePushStateWithPowerUsed (double time, const char *entityType, const char *container, const char *value, double powerUsed);
void pajePushStateWithBandwidthUsed (double time, const char *entityType, const char *container, const char *value, double bwUsed);
void pajePopState (double time, const char *entityType, const char *container);
void pajeStartLink (double time, const char *entityType, const char *container, const char *value, const char *sourceContainer, const char *key);
void pajeStartLinkWithBandwidthLatency (double time, const char *entityType, const char *container, const char *value, const char *sourceContainer, const char *key, double bw, double lat);
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

/* declaration of instrumentation functions from msg_task_instr.c */
void __TRACE_msg_init (void);
void __TRACE_current_category_set (m_task_t task);
void __TRACE_current_category_unset (void);
char *__TRACE_current_category_get (smx_process_t proc);
void __TRACE_task_location (m_task_t task);
void __TRACE_task_location_present (m_task_t task);
void __TRACE_task_location_not_present (m_task_t task);
void TRACE_msg_task_create (m_task_t task);
void TRACE_msg_task_execute_start (m_task_t task);
void TRACE_msg_task_execute_end (m_task_t task);
void TRACE_msg_task_destroy (m_task_t task);
void TRACE_msg_task_get_start(void);
void TRACE_msg_task_get_end (double start_time, m_task_t task);
int TRACE_msg_task_put_start (m_task_t task); //returns TRUE if the task_put_end must be called
void TRACE_msg_task_put_end (void);

/* declaration of instrumentation functions from msg_process_instr.c */
void __TRACE_msg_process_init (void);
void __TRACE_msg_process_location (m_process_t process);
void __TRACE_msg_process_present (m_process_t process);
void TRACE_msg_process_change_host (m_process_t process, m_host_t old_host, m_host_t new_host);
void TRACE_msg_process_kill (m_process_t process);
void TRACE_msg_process_suspend (m_process_t process);
void TRACE_msg_process_resume (m_process_t process);
void TRACE_msg_process_sleep_in (m_process_t process); //called from msg/gos.c
void TRACE_msg_process_sleep_out (m_process_t process);
void TRACE_msg_process_end (m_process_t process);

/* declaration of instrumentation functions from msg_volume.c */
void __TRACE_msg_volume_start (m_task_t task);
void __TRACE_msg_volume_finish (m_task_t task);

/* from smx.c */
void TRACE_smx_action_execute (smx_action_t act);
void TRACE_smx_action_communicate (smx_action_t act, smx_process_t proc);
void TRACE_smx_action_destroy (smx_action_t act);

/* from surf.c */
void __TRACE_surf_init (void);
void __TRACE_surf_finalize (void);
void __TRACE_surf_check_variable_set_to_zero (double now, const char *variable, const char *resource);
void __TRACE_surf_update_action_state_resource (double now, double delta, const char *type, const char *name, double value);
void __TRACE_surf_set_resource_variable (double date, const char *variable, const char *resource, double value);
void TRACE_surf_host_declaration (char *name, double power);
void TRACE_surf_host_set_power (double date, char *resource, double power);
void TRACE_surf_host_set_utilization (const char *name, smx_action_t smx_action, double value, double now, double delta);
void TRACE_surf_host_define_id (const char *name, int host_id);
void TRACE_surf_host_vivaldi_parse (char *host, double x, double y, double h);
void TRACE_surf_link_declaration (char *name, double bw, double lat);
void TRACE_surf_link_set_bandwidth (double date, char *resource, double bandwidth);
void TRACE_surf_link_set_latency (double date, char *resource, double latency);
void TRACE_surf_link_set_utilization (const char *name, smx_action_t smx_action, double value, double now, double delta);
void TRACE_surf_link_save_endpoints (char *link_name, int src, int dst);
void TRACE_surf_link_missing (void);
void TRACE_msg_clean (void);

//for tracing gtnets
void TRACE_surf_gtnets_communicate (void *action, int src, int dst);
int TRACE_surf_gtnets_get_src (void *action);
int TRACE_surf_gtnets_get_dst (void *action);
void TRACE_surf_gtnets_destroy (void *action);

#endif

#endif /* PRIVATE_H_ */
