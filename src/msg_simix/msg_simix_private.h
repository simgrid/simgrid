/* 	$Id$	 */

/* Copyright (c) 2002,2004,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef METASIMGRID_PRIVATE_H
#define METASIMGRID_PRIVATE_H

#include <stdio.h>
#include "msg/msg.h"
#include "simix/simix.h"
#include "xbt/fifo.h"
#include "xbt/dynar.h"
#include "xbt/swag.h"
#include "xbt/dict.h"
#include "xbt/context.h"
#include "xbt/config.h"

/**************** datatypes **********************************/

typedef struct simdata_host {
  smx_host_t host;			/* SURF modeling */
  xbt_fifo_t *mbox;		/* array of FIFOs used as a mailboxes  */
  smx_cond_t *sleeping;	/* array of process used to know whether a local process is
				   waiting for a communication on a channel */
	smx_mutex_t mutex;
} s_simdata_host_t;

/********************************* Task **************************************/

typedef struct simdata_task {
  smx_action_t compute;	/* SURF modeling of computation  */
  smx_action_t comm;	        /* SURF modeling of communication  */
  double message_size;		/* Data size  */
  double computation_amount;	/* Computation size  */
	smx_cond_t cond;
	smx_mutex_t mutex;
  m_process_t sender;
  m_host_t source;
  double priority;
  double rate;
  int using;
  /*******  Parallel Tasks Only !!!! *******/
  int host_nb;
  void * *host_list;            /* SURF modeling */
  double *comp_amount;
  double *comm_amount;
} s_simdata_task_t;

/******************************* Process *************************************/

typedef struct simdata_process {
  m_host_t host;                /* the host on which the process is running */
	smx_process_t smx_process;
  int PID;			/* used for debugging purposes */
  int PPID;			/* The parent PID */
  //m_task_t waiting_task;        
  int blocked;
  int suspended;
  m_host_t put_host;		/* used for debugging purposes */
  m_channel_t put_channel;	/* used for debugging purposes */
  int argc;                     /* arguments number if any */
  char **argv;                  /* arguments table if any */
  MSG_error_t last_errno;       /* the last value returned by a MSG_function */
} s_simdata_process_t;

typedef struct process_arg {
  const char *name;
  m_process_code_t code;
  void *data;
  m_host_t host;
  int argc;
  char **argv;
  double kill_time;
} s_process_arg_t, *process_arg_t;

/************************** Global variables ********************************/
typedef struct MSG_Global {
  xbt_fifo_t host;
  xbt_fifo_t process_list;
  int max_channel;
  int PID;
  int session;
} s_MSG_Global_t, *MSG_Global_t;

extern MSG_Global_t msg_global;
      
/*************************************************************/

#define PROCESS_SET_ERRNO(val) (MSG_process_self()->simdata->last_errno=val)
#define PROCESS_GET_ERRNO() (MSG_process_self()->simdata->last_errno)
//#define MSG_RETURN(val) do {PROCESS_SET_ERRNO(val);return(val);} while(0)
#define MSG_RETURN(val) do {return(val);} while(0)
/* #define CHECK_ERRNO()  ASSERT((PROCESS_GET_ERRNO()!=MSG_HOST_FAILURE),"Host failed, you cannot call this function.") */

#define CHECK_HOST()  xbt_assert0(SIMIX_host_get_state(SIMIX_host_self())==1,\
                                  "Host failed, you cannot call this function.")

m_host_t __MSG_host_create(smx_host_t workstation, void *data);
void __MSG_host_destroy(m_host_t host);
void __MSG_task_execute(m_process_t process, m_task_t task);
MSG_error_t __MSG_wait_for_computation(m_process_t process, m_task_t task);
MSG_error_t __MSG_task_wait_event(m_process_t process, m_task_t task);

int __MSG_process_block(double max_duration, const char *info);
MSG_error_t __MSG_process_unblock(m_process_t process);
int __MSG_process_isBlocked(m_process_t process);

void __MSG_display_process_status(void);

m_process_t __MSG_process_create_with_arguments(const char *name,
					      m_process_code_t code, void *data,
													      char * hostname, int argc, char **argv);


#endif
