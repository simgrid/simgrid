/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_PROCESS_PRIVATE_H
#define _SIMIX_PROCESS_PRIVATE_H

#include "simgrid/simix.h"
#include "smx_smurf_private.h"

typedef struct s_smx_process_exit_fun {
  int_f_pvoid_t fun;
  void *arg;
} s_smx_process_exit_fun_t, *smx_process_exit_fun_t;

typedef struct s_smx_process_arg {
  char *name;
  xbt_main_func_t code;
  void *data;
  const char *hostname;
  int argc;
  char **argv;
  double kill_time;
  xbt_dict_t properties;
  unsigned auto_restart:1;
} s_smx_process_arg_t, *smx_process_arg_t;


/** @brief Process datatype */
typedef struct s_smx_process {
  s_xbt_swag_hookup_t process_hookup;
  s_xbt_swag_hookup_t synchro_hookup;   /* process_to_run or mutex->sleeping and co */
  s_xbt_swag_hookup_t host_proc_hookup;
  s_xbt_swag_hookup_t destroy_hookup;

  unsigned long pid;
  unsigned long ppid;
  char *name;                   /**< @brief process name if any */
  smx_host_t smx_host;          /* the host on which the process is running */
  smx_context_t context;        /* the context (uctx/raw/thread) that executes the user function */
  xbt_running_ctx_t *running_ctx;
  unsigned doexception:1;
  unsigned blocked:1;
  unsigned suspended:1;
  unsigned auto_restart:1;

  smx_host_t new_host;          /* if not null, the host on which the process must migrate to */
  smx_action_t waiting_action;  /* the current blocking action if any */
  xbt_fifo_t comms;       /* the current non-blocking communication actions */
  xbt_dict_t properties;
  s_smx_simcall_t simcall;
  void *data;                   /* kept for compatibility, it should be replaced with moddata */
  xbt_dynar_t on_exit;     /* list of functions executed when the process dies */

  xbt_main_func_t code;
  int argc;
  char **argv;
  double kill_time;

} s_smx_process_t;


void SIMIX_process_create(smx_process_t *process,
                          const char *name,
                          xbt_main_func_t code,
                          void *data,
                          const char *hostname,
                          double kill_time,
                          int argc, char **argv,
                          xbt_dict_t properties,
                          int auto_restart);
void SIMIX_process_runall(void);
void SIMIX_process_kill(smx_process_t process, smx_process_t issuer);
void SIMIX_process_killall(smx_process_t issuer, int reset_pid);
smx_process_t SIMIX_process_create_from_wrapper(smx_process_arg_t args);
void SIMIX_create_maestro_process(void);
void SIMIX_process_stop(smx_process_t arg);
void SIMIX_process_cleanup(smx_process_t arg);
void SIMIX_process_empty_trash(void);
void SIMIX_process_yield(smx_process_t self);
xbt_running_ctx_t *SIMIX_process_get_running_context(void);
void SIMIX_process_exception_terminate(xbt_ex_t * e);
void SIMIX_pre_process_change_host(smx_simcall_t, smx_process_t process,
           smx_host_t dest);
void SIMIX_process_change_host(smx_process_t process,
             smx_host_t dest);
void SIMIX_pre_process_suspend(smx_simcall_t simcall, smx_process_t process);
smx_action_t SIMIX_process_suspend(smx_process_t process, smx_process_t issuer);
void SIMIX_process_resume(smx_process_t process, smx_process_t issuer);
int SIMIX_process_get_PID(smx_process_t self);
int SIMIX_process_get_PPID(smx_process_t self);
void* SIMIX_process_get_data(smx_process_t process);
void SIMIX_process_set_data(smx_process_t process, void *data);
smx_host_t SIMIX_process_get_host(smx_process_t process);
const char* SIMIX_process_get_name(smx_process_t process);
smx_process_t SIMIX_process_get_by_name(const char* name);
int SIMIX_process_is_suspended(smx_process_t process);
xbt_dict_t SIMIX_process_get_properties(smx_process_t process);
void SIMIX_pre_process_sleep(smx_simcall_t simcall, double duration);
smx_action_t SIMIX_process_sleep(smx_process_t process, double duration);
void SIMIX_post_process_sleep(smx_action_t action);

void SIMIX_process_sleep_suspend(smx_action_t action);
void SIMIX_process_sleep_resume(smx_action_t action);
void SIMIX_process_sleep_destroy(smx_action_t action);
void SIMIX_process_auto_restart_set(smx_process_t process, int auto_restart);
smx_process_t SIMIX_process_restart(smx_process_t process, smx_process_t issuer);

// pre prototypes
void SIMIX_pre_process_create(smx_simcall_t simcall, smx_process_t *process,
		              const char *name, xbt_main_func_t code, void *data,
			      const char *hostname, double kill_time, int argc,
			      char **argv, xbt_dict_t properties, int auto_restart);
void SIMIX_pre_process_kill(smx_simcall_t simcall, smx_process_t process);
void SIMIX_pre_process_killall(smx_simcall_t simcall, int reset_pid);
void SIMIX_pre_process_cleanup(smx_simcall_t simcall, smx_process_t process);
void SIMIX_pre_process_resume(smx_simcall_t simcall, smx_process_t process);
int SIMIX_pre_process_count(smx_simcall_t simcall);
void* SIMIX_pre_process_self_get_data(smx_simcall_t simcall, smx_process_t process);
int SIMIX_pre_process_get_PID(smx_simcall_t simcall, smx_process_t self);
int SIMIX_pre_process_get_PPID(smx_simcall_t simcall, smx_process_t self);
void* SIMIX_pre_process_get_data(smx_simcall_t simcall, smx_process_t process);
void SIMIX_pre_process_set_data(smx_simcall_t simcall, smx_process_t process,
                                void *data);
smx_host_t SIMIX_pre_process_get_host(smx_simcall_t simcall, smx_process_t process);
const char* SIMIX_pre_process_get_name(smx_simcall_t simcall, smx_process_t process);
int SIMIX_pre_process_is_suspended(smx_simcall_t simcall, smx_process_t process);
xbt_dict_t SIMIX_pre_process_get_properties(smx_simcall_t simcall, smx_process_t process);
void SIMIX_pre_process_on_exit(smx_simcall_t simcall, smx_process_t process,
	                       int_f_pvoid_t fun, void *data);
void SIMIX_pre_process_auto_restart_set(smx_simcall_t simcall, smx_process_t process,
		                        int auto_restart);
smx_process_t SIMIX_pre_process_restart(smx_simcall_t simcall, smx_process_t process);
#endif
