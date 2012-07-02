/* Copyright (c) 2004-2012. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_DATATYPE_H
#define MSG_DATATYPE_H
#include "xbt/misc.h"
#include "xbt/file_stat.h"
#include "simgrid/simix.h"
#include "simgrid_config.h"     // for HAVE_TRACING

SG_BEGIN_DECL()

/* ******************************** Mailbox ************************************ */

/** @brief Mailbox datatype
 *  @ingroup msg_task_usage
 *
 * Object representing a communication rendez-vous point, on which
 * the sender finds the receiver it wants to communicate with. As a
 * MSG user, you will only rarely manipulate any of these objects
 * directly, since most of the public interface (such as
 * #MSG_task_send and friends) hide this object behind a string
 * alias. That mean that you don't provide the mailbox on which you
 * want to send your task, but only the name of this mailbox. */
typedef struct s_smx_rvpoint *msg_mailbox_t;


/* ******************************** Host ************************************ */

typedef struct msg_host {
  xbt_swag_t vms;
  smx_host_t smx_host;          /**< SIMIX representation of this host   */
#ifdef MSG_USE_DEPRECATED
  msg_mailbox_t *mailboxes;     /**< the channels  */
#endif
} s_msg_host_t;

/** @brief Host datatype.
    @ingroup m_host_management

    A <em>location</em> (or <em>host</em>) is any possible place where
    a process may run. Thus it is represented as a <em>physical
    resource with computing capabilities</em>, some <em>mailboxes</em>
    to enable running process to communicate with remote ones, and
    some <em>private data</em> that can be only accessed by local
    process.
 */
typedef struct msg_host *msg_host_t;

/* ******************************** Task ************************************ */

typedef struct simdata_task *simdata_task_t;

typedef struct msg_task {
  char *name;                   /**< @brief task name if any */
  simdata_task_t simdata;       /**< @brief simulator data */
  void *data;                   /**< @brief user data */
#ifdef HAVE_TRACING
  long long int counter;        /* task unique identifier for instrumentation */
  char *category;               /* task category for instrumentation */
#endif
} s_msg_task_t;

/** @brief Task datatype.
    @ingroup m_task_management 

    A <em>task</em> may then be defined by a <em>computing
    amount</em>, a <em>message size</em> and some <em>private
    data</em>.
 */
typedef struct msg_task *msg_task_t;


/* ******************************** File ************************************ */
typedef struct simdata_file *simdata_file_t;

typedef struct msg_file {
  char *name;                   /**< @brief file name */
  simdata_file_t simdata;                /**< @brief simulator data  */
  void *data;                   /**< @brief user data */
} s_msg_file_t;

/** @brief File datatype.
    @ingroup msg_file_management 
 
    You should consider this as an opaque object.
 */
typedef struct msg_file *msg_file_t;

typedef s_file_stat_t s_msg_stat_t, *msg_stat_t;


/*************** Begin GPU ***************/
typedef struct simdata_gpu_task *simdata_gpu_task_t;

typedef struct msg_gpu_task {
  char *name;                   /**< @brief task name if any */
  simdata_gpu_task_t simdata;       /**< @brief simulator data */
#ifdef HAVE_TRACING
  long long int counter;        /* task unique identifier for instrumentation */
  char *category;               /* task category for instrumentation */
#endif
} s_msg_gpu_task_t;

/** @brief GPU task datatype.
    @ingroup m_task_management

    A <em>task</em> may then be defined by a <em>computing
    amount</em>, a <em>dispatch latency</em> and a <em>collect latency</em>.
    \see m_task_management
*/
typedef struct msg_gpu_task *msg_gpu_task_t;
/*************** End GPU ***************/

/**
 * \brief @brief Communication action.
 * \ingroup msg_task_usage
 *
 * Object representing an ongoing communication between processes. Such beast is usually obtained by using #MSG_task_isend, #MSG_task_irecv or friends.
 */
typedef struct msg_comm *msg_comm_t;

/** \brief Default value for an uninitialized #msg_task_t.
    \ingroup m_task_management 
*/
#define MSG_TASK_UNINITIALIZED NULL

/* ****************************** Process *********************************** */

/** @brief Process datatype.
    @ingroup m_process_management

    A process may be defined as a <em>code</em>, with some
    <em>private data</em>, executing in a <em>location</em>. 
 
    You should not access directly to the fields of the pointed
    structure, but always use the provided API to interact with
    processes.
 */
typedef struct s_smx_process *msg_process_t;

#ifdef MSG_USE_DEPRECATED

/* Compatibility typedefs */
typedef int                     m_channel_t;
typedef msg_gpu_task_t          m_gpu_task_t;
typedef msg_host_t              m_host_t;
typedef msg_process_t           m_process_t;
typedef msg_task_t              m_task_t;
typedef s_msg_gpu_task_t        s_m_gpu_task_t;
typedef s_msg_host_t            s_m_host_t;
typedef s_msg_task_t            s_m_task_t;
#endif

SG_END_DECL()
#endif
