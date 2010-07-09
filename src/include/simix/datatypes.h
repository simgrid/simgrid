/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_DATATYPE_H
#define SIMIX_DATATYPE_H
#include "xbt/misc.h"
#include "xbt/swag.h"
#include "xbt/fifo.h"

SG_BEGIN_DECL()

/* ******************************** Host ************************************ */
/** @defgroup m_datatypes_management_details Details on SIMIX datatypes */
/** @brief Host datatype  
    @ingroup m_datatypes_management

    A <em>location</em> (or <em>host</em>) is any possible place where
    a process may run. Thus it is represented as a <em>physical
    resource with computing capabilities</em>, some <em>mailboxes</em>
    to enable running process to communicate with remote ones, and
    some <em>private data</em> that can be only accessed by local
    process.

    \see m_host_management
  @{ */
     typedef struct s_smx_host *smx_host_t;
/** @} */


/* ******************************** Syncro ************************************ */
     typedef struct s_smx_mutex {
       xbt_swag_t sleeping;          /* list of sleeping process */
       int refcount;
     } s_smx_mutex_t;
     typedef s_smx_mutex_t *smx_mutex_t;

     typedef struct s_smx_cond {
       xbt_swag_t sleeping;          /* list of sleeping process */
       smx_mutex_t mutex;
       xbt_fifo_t actions;           /* list of actions */
     } s_smx_cond_t;
     typedef s_smx_cond_t *smx_cond_t;

     typedef struct s_smx_sem {
       xbt_fifo_t sleeping;          /* list of sleeping process */
       int capacity;
       xbt_fifo_t actions;           /* list of actions */
     } s_smx_sem_t;
     typedef s_smx_sem_t *smx_sem_t;

/********************************** Action *************************************/
     typedef struct s_smx_action *smx_action_t;

/* ****************************** Process *********************************** */
/** @brief Agent datatype  
    @ingroup m_datatypes_management 

    An agent may be defined as a <em>code</em>, with some <em>private
    data</em>, executing in a <em>location</em>.
    \see m_process_management
  @{ */
     typedef struct s_smx_process *smx_process_t;
/** @} */

     typedef struct s_smx_context *smx_context_t;

/******************************* Networking ***********************************/
    typedef struct s_smx_rvpoint *smx_rdv_t;
    typedef struct s_smx_comm *smx_comm_t;
    typedef enum {comm_send,
                  comm_recv
    } smx_comm_type_t;



SG_END_DECL()
#endif
