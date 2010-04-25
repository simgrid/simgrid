/* Copyright (c) 2004, 2005, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_DATATYPE_H
#define MSG_DATATYPE_H
#include "xbt/misc.h"
#include "instr/tracing_config.h" // for HAVE_TRACING

SG_BEGIN_DECL()

/* ******************************** Host ************************************ */
/** @defgroup m_datatypes_management_details Details on MSG datatypes
    @ingroup  m_datatypes_management*/
     typedef struct simdata_host *simdata_host_t;
/** @brief Host datatype 
    @ingroup m_datatypes_management_details */
     typedef struct m_host {
       char *name;              /**< @brief host name if any */
       simdata_host_t simdata;  /**< @brief simulator data */
       void *data;              /**< @brief user data */
     } s_m_host_t;
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
     typedef struct m_host *m_host_t;
/** @} */
/* ******************************** Task ************************************ */

     typedef struct simdata_task *simdata_task_t;
/** @brief Task datatype 
    @ingroup m_datatypes_management_details */
     typedef struct m_task {
       char *name;              /**< @brief task name if any */
       simdata_task_t simdata;  /**< @brief simulator data */
       void *data;              /**< @brief user data */
#ifdef HAVE_TRACING
       long long int counter;   /* task unique identifier for instrumentation */
       char *category;      /* task category for instrumentation */
#endif
     } s_m_task_t;
/** @brief Task datatype  
    @ingroup m_datatypes_management 

    A <em>task</em> may then be defined by a <em>computing
    amount</em>, a <em>message size</em> and some <em>private
    data</em>.
    \see m_task_management
  @{ */
     typedef struct m_task *m_task_t;

/** \brief Default value for an uninitialized #m_task_t.
    \ingroup m_datatypes_management 
*/
#define MSG_TASK_UNINITIALIZED NULL

/** @} */



/* ****************************** Process *********************************** */
     typedef struct simdata_process *simdata_process_t;
/** @brief Process datatype 
    @ingroup m_datatypes_management_details @{ */
     typedef struct m_process {
       char *name;              /**< @brief process name if any */
       simdata_process_t simdata;
                                /**< @brief simulator data */
       void *data;              /**< @brief user data */
       char *category;      /* process category for instrumentation */
     } s_m_process_t;
/** @} */
/** @brief Agent datatype  
    @ingroup m_datatypes_management 

    An agent may be defined as a <em>code</em>, with some <em>private
    data</em>, executing in a <em>location</em>.
    \see m_process_management
  @{ */
     typedef struct m_process *m_process_t;
/** @} */

/* ********************************* Channel ******************************** */
/** @brief Channel datatype  
    @ingroup m_datatypes_management 

    A <em>channel</em>  is a number and identifies a mailbox type (just as a 
    port number does).
    \see m_channel_management
   @{ */
     typedef int m_channel_t;
/** @} */

/* ******************************** Mailbox ************************************ */

     typedef struct s_msg_mailbox *msg_mailbox_t;
/** @brief Mailbox datatype
    @ingroup m_datatypes_management_details @{ */

     msg_mailbox_t MSG_mailbox_create(const char *alias);
     void MSG_mailbox_free(void *mailbox);


/** @} */


/* ***************************** Error handling ***************************** */
/** @brief Error handling 
    @ingroup m_datatypes_management 
    @{
*/ /* Keep these code as binary values: java bindings manipulate | of these values */
     typedef enum {
       MSG_OK = 0,            /**< @brief Everything is right. Keep on going this way ! */
       MSG_TIMEOUT=1,         /**< @brief nothing good happened before the timer you provided elapsed */
       MSG_TRANSFER_FAILURE=2,/**< @brief There has been a problem during you task
      transfer. Either the network is down or the remote host has been
      shutdown. */
       MSG_HOST_FAILURE=4,    /**< @brief System shutdown. The host on which you are
      running has just been rebooted. Free your datastructures and
      return now !*/
       MSG_TASK_CANCELLED=8,  /**< @brief Canceled task. This task has been canceled by somebody!*/
     } MSG_error_t;
/** @} */

SG_END_DECL()
#endif
