/* 	$Id$	 */

/* Copyright (c) 2002,2004,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef METASIMGRID_PRIVATE_H
#define METASIMGRID_PRIVATE_H

#include "msg/msg.h"
#include "surf/surf.h"
#include "xbt/fifo.h"
#include "xbt/dynar.h"
#include "xbt/swag.h"
#include "xbt/dict.h"
#include "xbt/context.h"

/**************** datatypes **********************************/

typedef struct simdata_host {
  void *host;			/* SURF modeling */
  xbt_fifo_t *mbox;		/* array of FIFOs used as a mailboxes  */
  m_process_t *sleeping;	/* array of process used to know whether a local process is
				   waiting for a communication on a channel */
  xbt_fifo_t process_list;
} s_simdata_host_t;

/********************************* Task **************************************/

typedef struct simdata_task {
  surf_action_t compute;	/* SURF modeling of computation  */
  surf_action_t comm;	        /* SURF modeling of communication  */
  double message_size;		/* Data size  */
  double computation_amount;	/* Computation size  */
  xbt_dynar_t sleeping;		/* process to wake-up */
  m_process_t sender;
  int using;
} s_simdata_task_t;

/******************************* Process *************************************/

typedef struct simdata_process {
  m_host_t host;                /* the host on which the process is running */
  xbt_context_t context;	        /* the context that executes the scheduler fonction */
  int PID;			/* used for debugging purposes */
  int PPID;			/* The parent PID */
  m_task_t waiting_task;        
  int argc;                     /* arguments number if any */
  char **argv;                  /* arguments table if any */
  MSG_error_t last_errno;       /* the last value returned by a MSG_function */
} s_simdata_process_t;

/************************** Global variables ********************************/
typedef struct MSG_Global {
  xbt_fifo_t host;
  xbt_fifo_t process_to_run;
  xbt_fifo_t process_list;
  int max_channel;
  m_process_t current_process;
  xbt_dict_t registered_functions;
} s_MSG_Global_t, *MSG_Global_t;

extern MSG_Global_t msg_global;

/*************************************************************/

#define PROCESS_SET_ERRNO(val) (MSG_process_self()->simdata->last_errno=val)
#define PROCESS_GET_ERRNO() (MSG_process_self()->simdata->last_errno)
#define MSG_RETURN(val) do {PROCESS_SET_ERRNO(val);return(val);} while(0)
/* #define CHECK_ERRNO()  ASSERT((PROCESS_GET_ERRNO()!=MSG_HOST_FAILURE),"Host failed, you cannot call this function.") */

#define CHECK_HOST()  xbt_assert0(surf_workstation_resource->extension_public-> \
				  get_state(MSG_host_self()->simdata->host)==SURF_CPU_ON,\
                                  "Host failed, you cannot call this function.")

m_host_t __MSG_host_create(const char *name, void *workstation,
			   void *data);
void __MSG_host_destroy(m_host_t host);
void __MSG_task_execute(m_process_t process, m_task_t task);
MSG_error_t __MSG_wait_for_computation(m_process_t process, m_task_t task);
MSG_error_t __MSG_task_wait_event(m_process_t process, m_task_t task);

#endif
