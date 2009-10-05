/*     $Id$      */

/* Copyright (c) 2002-2007 Arnaud Legrand.                                  */
/* Copyright (c) 2007 Bruno Donassolo.                                      */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef METASIMGRID_PRIVATE_H
#define METASIMGRID_PRIVATE_H

#include <stdio.h>
#include "msg/msg.h"
#include "simix/simix.h"
#include "surf/surf.h"
#include "xbt/fifo.h"
#include "xbt/dynar.h"
#include "xbt/swag.h"
#include "xbt/dict.h"
#include "xbt/config.h"

SG_BEGIN_DECL()

/**************** datatypes **********************************/
/* this structure represents a mailbox */
     typedef struct s_msg_mailbox {
       char *alias;             /* the key of the mailbox in the global dictionary */
       xbt_fifo_t tasks;        /* the list of the tasks in the mailbox */
       smx_cond_t cond;         /* the condition on the mailbox */
       char *hostname;          /* the name of the host containing the mailbox */
       smx_rdv_t rdv;           /* SIMIX rendez-vous point */
     } s_msg_mailbox_t;

     typedef struct simdata_host {
       smx_host_t smx_host;     /* SURF modeling                                                                */
       struct s_msg_mailbox **mailboxes;        /* mailboxes to store msg tasks of of the host  */
       smx_mutex_t mutex;       /* mutex to access the host                                     */
     } s_simdata_host_t;

/********************************* Task **************************************/

     typedef struct simdata_task {
       smx_action_t compute;    /* SURF modeling of computation  */
       smx_action_t comm;       /* SURF modeling of communication  */
       double message_size;     /* Data size  */
       double computation_amount;       /* Computation size  */
       smx_cond_t cond;
       smx_mutex_t mutex;       /* Task mutex */
       m_process_t sender;
       m_process_t receiver;
       m_host_t source;
       double priority;
       double rate;
       int refcount;
  /*******  Parallel Tasks Only !!!! *******/
       int host_nb;
       smx_host_t *host_list;
       double *comp_amount;
       double *comm_amount;
     } s_simdata_task_t;

/******************************* Process *************************************/

     typedef struct simdata_process {
       m_host_t m_host;         /* the host on which the process is running */
       smx_process_t s_process;
       int PID;                 /* used for debugging purposes */
       int PPID;                /* The parent PID */
       m_host_t put_host;       /* used for debugging purposes */
       m_channel_t put_channel; /* used for debugging purposes */
       smx_action_t waiting_action;
       int argc;                /* arguments number if any */
       char **argv;             /* arguments table if any */
       MSG_error_t last_errno;  /* the last value returned by a MSG_function */
     } s_simdata_process_t;

     typedef struct process_arg {
       const char *name;
       xbt_main_func_t code;
       void *data;
       m_host_t m_host;
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
       unsigned long int sent_msg;      /* Total amount of messages sent during the simulation */
     } s_MSG_Global_t, *MSG_Global_t;

/*extern MSG_Global_t msg_global;*/
XBT_PUBLIC_DATA(MSG_Global_t) msg_global;


/*************************************************************/

#define PROCESS_SET_ERRNO(val) (MSG_process_self()->simdata->last_errno=val)
#define PROCESS_GET_ERRNO() (MSG_process_self()->simdata->last_errno)
#define MSG_RETURN(val) do {PROCESS_SET_ERRNO(val);return(val);} while(0)
/* #define CHECK_ERRNO()  ASSERT((PROCESS_GET_ERRNO()!=MSG_HOST_FAILURE),"Host failed, you cannot call this function.") */

#define CHECK_HOST()  xbt_assert1(SIMIX_host_get_state(SIMIX_host_self())==1,\
                                  "Host failed, you cannot call this function. (state=%d)",SIMIX_host_get_state(SIMIX_host_self()))

     m_host_t __MSG_host_create(smx_host_t workstation, void *data);

     void __MSG_host_destroy(m_host_t host);

     void __MSG_display_process_status(void);

     void __MSG_process_cleanup(void *arg);
     void *_MSG_process_create_from_SIMIX(const char *name,
                                          xbt_main_func_t code, void *data,
                                          char *hostname, int argc,
                                          char **argv, xbt_dict_t properties);
     void _MSG_process_kill_from_SIMIX(void *p);

     void _MSG_action_init(void);

SG_END_DECL()
#endif
