/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donnassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_PRIVATE_H
#define SIMIX_PRIVATE_H

#include "simix/simix.h"
#include "surf/surf.h"
#include "xbt/fifo.h"
#include "xbt/swag.h"
#include "xbt/dict.h"
#include "xbt/context.h"
#include "xbt/config.h"

/******************************* Datatypes **********************************/


/********************************** Host ************************************/

typedef struct s_simdata_host {
  void *host;			/* SURF modeling */
  xbt_swag_t process_list;
} s_simdata_host_t;

/********************************* Simix Global ******************************/

typedef struct SIMIX_Global {
  xbt_fifo_t host;
  xbt_swag_t process_to_run;
  xbt_swag_t process_list;
 /* xbt_swag_t process_sleeping; */

  smx_process_t current_process;
  xbt_dict_t registered_functions;
/*  FILE *paje_output;
  int session; */
} s_SIMIX_Global_t, *SIMIX_Global_t;

extern SIMIX_Global_t simix_global;

/******************************* Process *************************************/

typedef struct s_simdata_process {
  smx_host_t host;                /* the host on which the process is running */
  xbt_context_t context;	        /* the context that executes the scheduler fonction */
  int blocked;
  int suspended;
  smx_mutex_t mutex;		/* mutex on which the process is blocked  */
  smx_cond_t cond;		/* cond on which the process is blocked  */
  smx_host_t put_host;		/* used for debugging purposes */
  smx_action_t block_action;	/* action that block the process when it does a mutex_lock or cond_wait */
  int argc;                     /* arguments number if any */
  char **argv;                  /* arguments table if any */
//  SIMIX_error_t last_errno;       /* the last value returned by a MSG_function */
// int paje_state;               /* the number of states stacked with Paje */
} s_simdata_process_t;

typedef struct process_arg {
  const char *name;
  smx_process_code_t code;
  void *data;
  smx_host_t host;
  int argc;
  char **argv;
  double kill_time;
} s_process_arg_t, *process_arg_t;

/********************************* Mutex and Conditional ****************************/

typedef struct s_smx_mutex {
	xbt_swag_t sleeping;			/* list of sleeping process */
	int using;

} s_smx_mutex_t;

typedef struct s_smx_cond {
	xbt_swag_t sleeping; 			/* list of sleeping process */
	smx_mutex_t  mutex;
	xbt_fifo_t actions;			/* list of actions */

} s_smx_cond_t;

/********************************* Action **************************************/

typedef struct s_simdata_action {
  surf_action_t surf_action;	/* SURF modeling of computation  */
  
  xbt_fifo_t cond_list;		/* conditional variables that must be signaled when the action finish. */
  smx_host_t source; 

/* control needed when the action blocks a process(__SIMIX_process_block). For each process that is blocked a new action is created, the process is stocked in cond_process variable and the "boolean" action_block is marked to 1 */
  int action_block;		/* simix control variable, system action or not */
  smx_process_t cond_process;	/* system process will wake up */		
  smx_cond_t timeout_cond;	/* useful to remove the process from the sleeping list when a timeout occurs */

} s_simdata_action_t;



/******************************* Configuration support **********************************/

void simix_config_init(void); /* create the config set, call this before use! */
void simix_config_finalize(void); /* destroy the config set, call this at cleanup. */
extern int _simix_init_status; /* 0: beginning of time; 
                                1: pre-inited (cfg_set created); 
                                2: inited (running) */
extern xbt_cfg_t _simix_cfg_set;






//#define PROCESS_SET_ERRNO(val) (SIMIX_process_self()->simdata->last_errno=val)
//#define PROCESS_GET_ERRNO() (SIMIX_process_self()->simdata->last_errno)
//#define SIMIX_RETURN(val) do {PROCESS_SET_ERRNO(val);return(val);} while(0)
/* #define CHECK_ERRNO()  ASSERT((PROCESS_GET_ERRNO()!=MSG_HOST_FAILURE),"Host failed, you cannot call this function.") */

#define CHECK_HOST()  xbt_assert0(surf_workstation_resource->extension_public-> \
				  get_state(SIMIX_host_self()->simdata->host)==SURF_CPU_ON,\
                                  "Host failed, you cannot call this function.")

smx_host_t __SIMIX_host_create(const char *name, void *workstation, void *data);
void __SIMIX_host_destroy(smx_host_t host);

int __SIMIX_process_block(double max_duration);
void __SIMIX_process_unblock(smx_process_t process);
int __SIMIX_process_isBlocked(smx_process_t process);

void __SIMIX_display_process_status(void);

void __SIMIX_wait_for_action(smx_process_t process, smx_action_t action);


/*
void __MSG_task_execute(smx_process_t process, m_task_t task);
MSG_error_t __MSG_task_wait_event(smx_process_t process, m_task_t task);
*/


#endif
