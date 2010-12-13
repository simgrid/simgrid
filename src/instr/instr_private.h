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

/* from paje.c */
void TRACE_paje_create_header(void);
void TRACE_paje_start(void);
void TRACE_paje_end(void);
void pajeDefineContainerType(const char *alias, const char *containerType,
                             const char *name);
void pajeDefineStateType(const char *alias, const char *containerType,
                         const char *name);
void pajeDefineEventType(const char *alias, const char *containerType,
                         const char *name);
void pajeDefineLinkType(const char *alias, const char *containerType,
                        const char *sourceContainerType,
                        const char *destContainerType, const char *name);
void pajeCreateContainer(double time, const char *alias, const char *type,
                         const char *container, const char *name);
void pajeDestroyContainer(double time, const char *type,
                          const char *container);
void pajeSetState(double time, const char *entityType,
                  const char *container, const char *value);
void pajePushState(double time, const char *entityType,
                   const char *container, const char *value);
void pajePopState(double time, const char *entityType,
                  const char *container);
void pajeStartLink(double time, const char *entityType,
                   const char *container, const char *value,
                   const char *sourceContainer, const char *key);
void pajeStartLinkWithVolume(double time, const char *entityType,
                             const char *container, const char *value,
                             const char *sourceContainer, const char *key,
                             double volume);
void pajeEndLink(double time, const char *entityType,
                 const char *container, const char *value,
                 const char *destContainer, const char *key);
void pajeDefineVariableType(const char *alias, const char *containerType,
                            const char *name);
void pajeDefineVariableTypeWithColor(const char *alias, const char *containerType,
                            const char *name, const char *color);
void pajeSetVariable(double time, const char *entityType,
                     const char *container, const char *value);
void pajeAddVariable(double time, const char *entityType,
                     const char *container, const char *value);
void pajeSubVariable(double time, const char *entityType,
                     const char *container, const char *value);
void pajeNewEvent(double time, const char *entityType,
                  const char *container, const char *value);

/* declaration of instrumentation functions from msg_task_instr.c */
char *TRACE_task_container(m_task_t task, char *output, int len);
void TRACE_msg_task_alloc(void);
void TRACE_msg_task_release(void);
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
void TRACE_msg_process_alloc(void);
void TRACE_msg_process_release(void);
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

/* instr_routing.c */
void instr_routing_define_callbacks (void);
int instr_link_is_traced (const char *name);
char *instr_link_type (const char *name);
char *instr_host_type (const char *name);
void instr_destroy_platform (void);

#endif /* HAVE_TRACING */
#endif /* INSTR_PRIVATE_H_ */
