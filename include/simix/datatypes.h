/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_DATATYPES_H
#define _SIMIX_DATATYPES_H
#include "xbt/misc.h"
#include "xbt/swag.h"
#include "xbt/fifo.h"
#include "xbt/dict.h"

SG_BEGIN_DECL()

/* ****************************** File *********************************** */
typedef struct s_smx_file *smx_file_t;



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
typedef enum {
  SIMIX_WAITING,
  SIMIX_READY,
  SIMIX_RUNNING,
  SIMIX_DONE,
  SIMIX_CANCELED,
  SIMIX_FAILED,
  SIMIX_SRC_HOST_FAILURE,
  SIMIX_DST_HOST_FAILURE,
  SIMIX_SRC_TIMEOUT,
  SIMIX_DST_TIMEOUT,
  SIMIX_LINK_FAILURE
} e_smx_state_t;
/** @} */


typedef struct s_smx_timer* smx_timer_t;


/* ******************************** Synchro ************************************ */
typedef struct s_smx_mutex *smx_mutex_t;
typedef struct s_smx_cond *smx_cond_t;
typedef struct s_smx_sem *smx_sem_t;

/********************************** Action *************************************/
typedef struct s_smx_action *smx_action_t; /* FIXME: replace by specialized action handlers */



/* ****************************** Process *********************************** */
/** @brief Agent datatype  
    @ingroup m_datatypes_management 

    An agent may be defined as a <em>code</em>, with some <em>private
    data</em>, executing in a <em>location</em>.
    \see m_process_management
  @{ */
typedef struct s_smx_process *smx_process_t;
/** @} */


/*
 * Type of function that creates a process.
 * The function must accept the following parameters:
 * void* process: the process created will be stored there
 * const char *name: a name for the object. It is for user-level information and can be NULL
 * xbt_main_func_t code: is a function describing the behavior of the agent
 * void *data: data a pointer to any data one may want to attach to the new object.
 * smx_host_t host: the location where the new agent is executed
 * int argc, char **argv: parameters passed to code
 * xbt_dict_t pros: properties
 */
typedef void (*smx_creation_func_t) ( /* process */ smx_process_t*,
                                      /* name */ const char*,
                                      /* code */ xbt_main_func_t,
                                      /* userdata */ void*,
                                      /* hostname */ const char*,
                                      /* argc */ int,
                                      /* argv */ char**,
                                      /* props */ xbt_dict_t);


/******************************* Networking ***********************************/
typedef struct s_smx_rvpoint *smx_rdv_t;


SG_END_DECL()
#endif
