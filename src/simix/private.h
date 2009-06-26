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
#include "xbt/context.h"
#include "xbt/function_types.h"

/******************************* Datatypes **********************************/


/********************************** Host ************************************/

typedef struct s_smx_simdata_host {
  void *host;                   /* SURF modeling */
  xbt_swag_t process_list;
} s_smx_simdata_host_t;

/********************************* Simix Global ******************************/

typedef struct SIMIX_Global {
  xbt_dict_t host;
  xbt_swag_t process_to_run;
  xbt_swag_t process_list;

  smx_process_t current_process;
  xbt_dict_t registered_functions;
  smx_creation_func_t create_process_function;
  void_f_pvoid_t kill_process_function;
  void_f_pvoid_t cleanup_process_function;
} s_SIMIX_Global_t, *SIMIX_Global_t;

extern SIMIX_Global_t simix_global;

/******************************* Process *************************************/

typedef struct s_smx_simdata_process {
  smx_host_t smx_host;          /* the host on which the process is running */
  xbt_context_t context;        /* the context that executes the scheduler fonction */
  int blocked;
  int suspended;
  smx_mutex_t mutex;            /* mutex on which the process is blocked  */
  smx_cond_t cond;              /* cond on which the process is blocked  */
  int argc;                     /* arguments number if any */
  char **argv;                  /* arguments table if any */
  xbt_dict_t properties;
} s_smx_simdata_process_t;

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

/********************************* Mutex and Conditional ****************************/

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

/********************************* Action **************************************/

typedef struct s_smx_simdata_action {
  surf_action_t surf_action;    /* SURF modeling of computation  */

  smx_host_t source;

} s_smx_simdata_action_t;



/******************************* Other **********************************/


#define SIMIX_CHECK_HOST()  xbt_assert0(surf_workstation_model->extension_public-> \
				  get_state(SIMIX_host_self()->simdata->host)==SURF_CPU_ON,\
                                  "Host failed, you cannot call this function.")

smx_host_t __SIMIX_host_create(const char *name, void *workstation,
                               void *data);
void __SIMIX_host_destroy(void *host);

void __SIMIX_cond_wait(smx_cond_t cond);

void __SIMIX_cond_display_actions(smx_cond_t cond);
void __SIMIX_action_display_conditions(smx_action_t action);

#endif
