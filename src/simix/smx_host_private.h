/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_HOST_PRIVATE_H
#define _SIMIX_HOST_PRIVATE_H

#include "simgrid/simix.h"
#include "smx_smurf_private.h"

/** @brief Host datatype */
typedef struct s_smx_host {
  char *name;              /**< @brief host name if any */
  void *host;                   /* SURF modeling */
  xbt_swag_t process_list;
  xbt_dynar_t auto_restart_processes;
  void *data;              /**< @brief user data */
} s_smx_host_t;

smx_host_t SIMIX_host_create(const char *name, void *workstation, void *data);
void SIMIX_host_destroy(void *host);

void SIMIX_host_add_auto_restart_process(smx_host_t host,
                                         const char *name,
                                         xbt_main_func_t code,
                                         void *data,
                                         const char *hostname,
                                         double kill_time,
                                         int argc, char **argv,
                                         xbt_dict_t properties,
                                         int auto_restart);

void SIMIX_host_restart_processes(smx_host_t host);
void SIMIX_host_autorestart(smx_host_t host);
xbt_dict_t SIMIX_host_get_properties(u_smx_scalar_t *args);
double SIMIX_host_get_speed(u_smx_scalar_t *args);
double SIMIX_host_get_available_speed(u_smx_scalar_t *args);
int SIMIX_host_get_state(u_smx_scalar_t *args);
smx_action_t SIMIX_host_execute(smx_process_t issuer, u_smx_scalar_t *args);
smx_action_t SIMIX_host_parallel_execute(u_smx_scalar_t *args);
void SIMIX_host_execution_destroy(u_smx_scalar_t *args);
void SIMIX_host_execution_cancel(u_smx_scalar_t *args);
double SIMIX_host_execution_get_remains(u_smx_scalar_t *args);
e_smx_state_t SIMIX_host_execution_get_state(u_smx_scalar_t *args);
void SIMIX_host_execution_set_priority(u_smx_scalar_t *args);
void SIMIX_pre_host_execution_wait(u_smx_scalar_t *args);

void SIMIX_host_execution_suspend(smx_action_t action);
void SIMIX_host_execution_resume(smx_action_t action);

void SIMIX_post_host_execute(smx_action_t action);

#ifdef HAVE_TRACING
void SIMIX_set_category(smx_action_t action, const char *category);
#endif

#endif

