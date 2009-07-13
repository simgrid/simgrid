/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donnassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_PRIVATE_H
#define SIMIX_PRIVATE_H

#include <stdio.h>
#include "simix/simix.h"
#include "surf/surf.h"
#include "xbt/fifo.h"
#include "xbt/swag.h"
#include "xbt/dict.h"
#include "xbt/function_types.h"

/******************************** Datatypes ***********************************/


/*********************************** Host *************************************/

/** @brief Host datatype
    @ingroup m_datatypes_management_details */
typedef struct s_smx_host {
  char *name;              /**< @brief host name if any */
  void *host;              /* SURF modeling */
  xbt_swag_t process_list;
  void *data;              /**< @brief user data */
} s_smx_host_t;

/********************************** Simix Global ******************************/

typedef struct s_smx_context_factory *smx_context_factory_t;

typedef struct SIMIX_Global {
  smx_context_factory_t context_factory;
  xbt_dict_t host;
  xbt_swag_t process_to_run;
  xbt_swag_t process_list;
  xbt_swag_t process_to_destroy;
  smx_process_t current_process;
  smx_process_t maestro_process;
  xbt_dict_t registered_functions;
  smx_creation_func_t create_process_function;
  void_f_pvoid_t kill_process_function;
  void_f_pvoid_t cleanup_process_function;
} s_SIMIX_Global_t, *SIMIX_Global_t;

extern SIMIX_Global_t simix_global;

/******************************** Process *************************************/

typedef struct s_smx_context *smx_context_t;

/** @brief Process datatype
    @ingroup m_datatypes_management_details @{ */
     typedef struct s_smx_process {
       s_xbt_swag_hookup_t process_hookup;
       s_xbt_swag_hookup_t synchro_hookup;
       s_xbt_swag_hookup_t host_proc_hookup;
       s_xbt_swag_hookup_t destroy_hookup;

       char *name;              /**< @brief process name if any */
       smx_host_t smx_host;     /* the host on which the process is running */
       smx_context_t context;   /* the context that executes the scheduler function */
       int argc;                /* arguments number if any */
       char **argv;             /* arguments table if any */
       int blocked : 1;
       int suspended : 1;
       int iwannadie : 1;
       smx_mutex_t mutex;       /* mutex on which the process is blocked  */
       smx_cond_t cond;         /* cond on which the process is blocked  */
       xbt_dict_t properties;
       void *data;              /* kept for compatibility, it should be replaced with moddata */
       void_f_pvoid_t cleanup_func;
       void *cleanup_arg;

     } s_smx_process_t;
/** @} */

typedef struct s_smx_process_arg {
  const char *name;
  xbt_main_func_t code;
  void *data;
  char *hostname;
  int argc;
  char **argv;
  double kill_time;
  xbt_dict_t properties;
} s_smx_process_arg_t, *smx_process_arg_t;

/*************************** Mutex and Conditional ****************************/

typedef struct s_smx_mutex {

  /* KEEP IT IN SYNC WITH src/xbt_sg_thread.c::struct s_xbt_mutex */
  xbt_swag_t sleeping;          /* list of sleeping process */
  int refcount;
  /* KEEP IT IN SYNC WITH src/xbt_sg_thread.c::struct s_xbt_mutex */

} s_smx_mutex_t;

typedef struct s_smx_cond {

  /* KEEP IT IN SYNC WITH src/xbt_sg_thread.c::struct s_xbt_cond */
  xbt_swag_t sleeping;          /* list of sleeping process */
  smx_mutex_t mutex;
  xbt_fifo_t actions;           /* list of actions */
  /* KEEP IT IN SYNC WITH src/xbt_sg_thread.c::struct s_xbt_cond */

} s_smx_cond_t;

/********************************* Action *************************************/

/** @brief Action datatype
    @ingroup m_datatypes_management_details */
typedef struct s_smx_action {
  char *name;              /**< @brief action name if any */
  xbt_fifo_t cond_list;    /*< conditional variables that must be signaled when the action finish. */
  void *data;              /**< @brief user data */
  int refcount;            /**< @brief reference counter */
  surf_action_t surf_action;    /* SURF modeling of computation  */
  smx_host_t source;
} s_smx_action_t;

/************************** Configuration support *****************************/

extern int _simix_init_status;  /* 0: beginning of time; FIXME: KILLME ?
                                   1: pre-inited (cfg_set created);
                                   2: inited (running) */

#define SIMIX_CHECK_HOST()  xbt_assert0(surf_workstation_model->extension.workstation. \
				  get_state(SIMIX_host_self()->host)==SURF_RESOURCE_ON,\
                                  "Host failed, you cannot call this function.")

smx_host_t __SIMIX_host_create(const char *name, void *workstation, void *data);
void __SIMIX_host_destroy(void *host);
void __SIMIX_cond_wait(smx_cond_t cond);
void __SIMIX_cond_display_actions(smx_cond_t cond);
void __SIMIX_action_display_conditions(smx_action_t action);
void __SIMIX_create_maestro_process(void);

/******************************** Context *************************************/

int SIMIX_context_create_maestro(smx_process_t *process);

int SIMIX_context_new(smx_process_t *process, xbt_main_func_t code);

void SIMIX_context_kill(smx_process_t process);

void SIMIX_context_start(smx_process_t process);

void SIMIX_context_yield(void);

void SIMIX_context_schedule(smx_process_t process);

void SIMIX_context_empty_trash(void);

void SIMIX_context_stop(int exit_code);

void SIMIX_context_free(smx_process_t process);

void SIMIX_context_mod_init(void);

void SIMIX_context_mod_exit(void);
#endif
