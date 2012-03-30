/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef METASIMGRID_PRIVATE_H
#define METASIMGRID_PRIVATE_H

#include "msg/msg.h"
#include "simix/simix.h"
#include "surf/surf.h"
#include "xbt/fifo.h"
#include "xbt/dynar.h"
#include "xbt/swag.h"
#include "xbt/dict.h"
#include "xbt/config.h"
#include "instr/instr_private.h"

SG_BEGIN_DECL()

/**************** datatypes **********************************/
typedef struct simdata_host {
  smx_host_t smx_host;          /* SURF modeling                                                                */
  msg_mailbox_t *mailboxes;     /* mailboxes to store msg tasks of of the host  */
} s_simdata_host_t;

/********************************* Task **************************************/

typedef struct simdata_task {
  smx_action_t compute;         /* SIMIX modeling of computation */
  smx_action_t comm;            /* SIMIX modeling of communication */
  double message_size;          /* Data size */
  double computation_amount;    /* Computation size */
  m_process_t sender;
  m_process_t receiver;
  m_host_t source;
  double priority;
  double rate;
  int isused;  /* Indicates whether the task is used in SIMIX currently */
  int host_nb;                  /* ==0 if sequential task; parallel task if not */
  /*******  Parallel Tasks Only !!!! *******/
  smx_host_t *host_list;
  double *comp_amount;
  double *comm_amount;
} s_simdata_task_t;

/********************************* File **************************************/
typedef struct simdata_file {
  smx_file_t smx_file;
} s_simdata_file_t;

/*************** Begin GPU ***************/
typedef struct simdata_gpu_task {
  double computation_amount;    /* Computation size */
  double dispatch_latency;
  double collect_latency;
  int isused;  /* Indicates whether the task is used in SIMIX currently */
} s_simdata_gpu_task_t;
/*************** End GPU ***************/

/******************************* Process *************************************/

typedef struct simdata_process {
  m_host_t m_host;              /* the host on which the process is running */
  int PID;                      /* used for debugging purposes */
  int PPID;                     /* The parent PID */
  m_host_t put_host;            /* used for debugging purposes */
#ifdef MSG_USE_DEPRECATED
  m_channel_t put_channel;      /* used for debugging purposes */
#endif
  smx_action_t waiting_action;
  m_task_t waiting_task;
  char **argv;                  /* arguments table if any */
  int argc;                     /* arguments number if any */
  MSG_error_t last_errno;       /* the last value returned by a MSG_function */
  void* data;                   /* user data */
} s_simdata_process_t, *simdata_process_t;

typedef struct process_arg {
  const char *name;
  xbt_main_func_t code;
  void *data;
  m_host_t m_host;
  int argc;
  char **argv;
  double kill_time;
} s_process_arg_t, *process_arg_t;

typedef struct msg_comm {
  smx_action_t s_comm;          /* SIMIX communication object encapsulated (the same for both processes) */
  m_task_t task_sent;           /* task sent (NULL for the receiver) */
  m_task_t *task_received;      /* where the task will be received (NULL for the sender) */
  MSG_error_t status;           /* status of the communication once finished */
} s_msg_comm_t;

/************************** Global variables ********************************/
typedef struct MSG_Global {
  xbt_fifo_t host;
#ifdef MSG_USE_DEPRECATED
  int max_channel;
#endif
  int PID;
  int session;
  unsigned long int sent_msg;   /* Total amount of messages sent during the simulation */
  void (*task_copy_callback) (m_task_t task, m_process_t src, m_process_t dst);
  void_f_pvoid_t process_data_cleanup;
} s_MSG_Global_t, *MSG_Global_t;

/*extern MSG_Global_t msg_global;*/
XBT_PUBLIC_DATA(MSG_Global_t) msg_global;


/*************************************************************/

#ifdef MSG_USE_DEPRECATED
#  define PROCESS_SET_ERRNO(val) \
  (((simdata_process_t) SIMIX_process_self_get_data(SIMIX_process_self()))->last_errno=val)
#  define PROCESS_GET_ERRNO() \
  (((simdata_process_t) SIMIX_process_self_get_data(SIMIX_process_self()))->last_errno)
#define MSG_RETURN(val) do {PROCESS_SET_ERRNO(val);return(val);} while(0)
/* #define CHECK_ERRNO()  ASSERT((PROCESS_GET_ERRNO()!=MSG_HOST_FAILURE),"Host failed, you cannot call this function.") */

#else
#  define MSG_RETURN(val) return(val)
#endif

m_host_t __MSG_host_create(smx_host_t workstation, void *data);
void __MSG_host_destroy(m_host_t host);

void __MSG_display_process_status(void);

void MSG_process_cleanup_from_SIMIX(smx_process_t smx_proc);
void MSG_process_create_from_SIMIX(smx_process_t *process, const char *name,
                                   xbt_main_func_t code, void *data,
                                   const char *hostname, int argc,
                                   char **argv, xbt_dict_t properties);
void MSG_process_kill_from_SIMIX(smx_process_t p);
void MSG_comm_copy_data_from_SIMIX(smx_action_t comm, void* buff, size_t buff_size);

void _MSG_action_init(void);
void _MSG_action_exit(void);

SG_END_DECL()
#endif
