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
  double rate;
  int using;
} s_simdata_task_t;

/******************************* Process *************************************/

typedef struct simdata_process {
  m_host_t host;                /* the host on which the process is running */
  xbt_context_t context;	        /* the context that executes the scheduler fonction */
  int PID;			/* used for debugging purposes */
  int PPID;			/* The parent PID */
  m_task_t waiting_task;        
  int blocked;
  int suspended;
  m_host_t put_host;		/* used for debugging purposes */
  m_channel_t put_channel;	/* used for debugging purposes */
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
  FILE *paje_output;
  int paje_maxPID;
  int PID;
  int session;
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

MSG_error_t __MSG_process_block(void);
MSG_error_t __MSG_process_unblock(m_process_t process);
int __MSG_process_isBlocked(m_process_t process);

#ifdef ALVIN_SPECIAL_LOGING
#define PAJE_PROCESS_STATE(process,state)\
  if(msg_global->paje_output) \
    fprintf(msg_global->paje_output,"10 %lg S_t P%d %s\n",\
            surf_get_clock(), process->simdata->PID,state)
#define PAJE_PROCESS_PUSH_STATE(process,state)\
  if(msg_global->paje_output) \
    fprintf(msg_global->paje_output,"11 %lg S_t P%d %s\n",\
            surf_get_clock(), process->simdata->PID,state)
#define PAJE_PROCESS_POP_STATE(process)\
  if(msg_global->paje_output) \
    fprintf(msg_global->paje_output,"12 %lg S_t P%d\n",\
            surf_get_clock(), process->simdata->PID)
#define PAJE_PROCESS_FREE(process)
#define PAJE_PROCESS_NEW(process)\
  if((msg_global->paje_output)) {\
    if((msg_global->session==0) || ((msg_global->session>0) && (process->simdata->PID > msg_global->paje_maxPID))) \
    fprintf(msg_global->paje_output,"7 %lg P%d P_t %p \"%s %d (%d)\"\n", \
            surf_get_clock(), process->simdata->PID, process->simdata->host, \
            process->name, process->simdata->PID, msg_global->session);\
    if(msg_global->paje_maxPID<process->simdata->PID) msg_global->paje_maxPID=process->simdata->PID;\
    }
#define PAJE_COMM_START(process,task,channel)\
  if(msg_global->paje_output) \
    fprintf(msg_global->paje_output,\
	    "16	%lg	Comm	CUR	COMM_%d	P%d	%p\n", \
            surf_get_clock(), channel, process->simdata->PID, task)
#define PAJE_COMM_STOP(process,task,channel)\
  if(msg_global->paje_output) \
    fprintf(msg_global->paje_output,\
	    "17	%lg	Comm	CUR	COMM_%d	P%d	%p\n", \
            surf_get_clock(), channel, process->simdata->PID, task)
#define PAJE_HOST_NEW(host)\
  if(msg_global->paje_output)\
    fprintf(msg_global->paje_output,"7 %lg %p H_t CUR \"%s\"\n",surf_get_clock(), \
	    host, host->name)
#define PAJE_HOST_FREE(host)\
  if(msg_global->paje_output)\
    fprintf(msg_global->paje_output,"8 %lg %p H_t\n",surf_get_clock(), host)

#else

#define PAJE_PROCESS_STATE(process,state)\
  if(msg_global->paje_output) \
    fprintf(msg_global->paje_output,"10 %lg S_t %p %s\n",\
            surf_get_clock(), process,state)
#define PAJE_PROCESS_PUSH_STATE(process,state)\
  if(msg_global->paje_output) \
    fprintf(msg_global->paje_output,"11 %lg S_t %p %s\n",\
            surf_get_clock(), process,state)
#define PAJE_PROCESS_POP_STATE(process)\
  if(msg_global->paje_output) \
    fprintf(msg_global->paje_output,"12 %lg S_t %p\n",\
            surf_get_clock(), process)

#define PAJE_PROCESS_FREE(process)\
  if(msg_global->paje_output) \
    fprintf(msg_global->paje_output,"8 %lg %p P_t\n", \
	    surf_get_clock(), process)
#define PAJE_PROCESS_NEW(process)\
  if(msg_global->paje_output) \
    fprintf(msg_global->paje_output,"7 %lg %p P_t %p \"%s %d (%d)\"\n", \
            surf_get_clock(), process, process->simdata->host, \
            process->name, process->simdata->PID, msg_global->session)
#define PAJE_COMM_START(process,task,channel)\
  if(msg_global->paje_output) \
    fprintf(msg_global->paje_output,\
	    "16	%lg	Comm	CUR	COMM_%d	%p	%p\n", \
            surf_get_clock(), channel, process, task)
#define PAJE_COMM_STOP(process,task,channel)\
  if(msg_global->paje_output) \
    fprintf(msg_global->paje_output,\
	    "17	%lg	Comm	CUR	COMM_%d	%p	%p\n", \
            surf_get_clock(), channel, process, task)
#define PAJE_HOST_NEW(host)\
  if(msg_global->paje_output)\
    fprintf(msg_global->paje_output,"7 %lg %p H_t CUR \"%s\"\n",surf_get_clock(), \
	    host, host->name)
#define PAJE_HOST_FREE(host)\
  if(msg_global->paje_output)\
    fprintf(msg_global->paje_output,"8 %lg %p H_t\n",surf_get_clock(), host);

#endif /* Alvin_Special_Loging */
#endif
