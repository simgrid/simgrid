/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_DATATYPE_H
#define MSG_DATATYPE_H
#include "xbt/misc.h"

BEGIN_DECL()

/** \defgroup m_datatypes_management MSG Data Types 
    \brief This section describes the different datatypes provided by MSG.
*/

/********************************* Host **************************************/

/** @name Host datatype  
    \ingroup m_datatypes_management

    A <em>location</em> (or <em>host</em>) is any possible place where
    a process may run. Thus it is represented as a <em>physical
    resource with computing capabilities</em>, some <em>mailboxes</em>
    to enable running process to communicate with remote ones, and
    some <em>private data</em> that can be only accessed by local
    process.

    \see m_host_management
*/
/* @{ */
typedef struct simdata_host *simdata_host_t;
typedef struct m_host {
  char *name;			/**< host name if any */
  simdata_host_t simdata;	/**< simulator data */
  void *data;			/**< user data */
} s_m_host_t, *m_host_t;
/* @} */
/********************************* Task **************************************/

/** @name Task datatype  
    \ingroup m_datatypes_management 

    A <em>task</em> may then be defined by a <em>computing
    amount</em>, a <em>message size</em> and some <em>private
    data</em>.
    \see m_task_management
*/
/* @{ */
typedef struct simdata_task *simdata_task_t;
typedef struct m_task {
  char *name;			/* host name if any */
  simdata_task_t simdata;	/* simulator data */
  void *data;			/* user data */
} s_m_task_t, *m_task_t;

/** \brief Default value for an uninitialized #m_task_t.
    \ingroup m_datatypes_management 
*/
#define MSG_TASK_UNINITIALIZED NULL

/* @} */

/******************************* Process *************************************/

/** @name Agent datatype  
    \ingroup m_datatypes_management 

    An agent may be defined as a <em>code</em>, with some <em>private
    data</em>, executing in a <em>location</em>.
    \see m_process_management
*/
/* @{ */
typedef struct simdata_process *simdata_process_t;
typedef struct m_process {
  /** A name */
  /** process name if any */
  char *name;			
  simdata_process_t simdata;	/* simulator data */
  void *data;			/* user data */
} s_m_process_t, *m_process_t;

/** 
    The code of an agent is a m_process_code_t, i.e. a function with no arguments 
    returning no value.
    \see m_process_management
*/
typedef int(*m_process_code_t)(int argc,char *argv[]) ;
/* @} */

/********************************** Channel **********************************/
/** @name Channel datatype  
    \ingroup m_datatypes_management 

    A <em>channel</em>  is a number and identifies a mailbox type (just as a 
    port number does).
    \see m_channel_management
*/
/* @{ */
typedef int m_channel_t;
/* @} */

/****************************** Error handling *******************************/
/** \brief Error handling
*/typedef enum {
  MSG_OK = 0,  /**< Everything is right. Keep on going this way ! */
  MSG_WARNING, /**< Mmmh! Something must be not perfectly clean. But I
      may be a paranoid freak... ! */
  MSG_TRANSFER_FAILURE, /**< There has been a problem during you task
      transfer. Either the network is down or the remote host has been
      shutdown. */
  MSG_HOST_FAILURE, /**< System shutdown. The host on which you are
      running has just been rebooted. Free your datastructures and
      return now !*/
  MSG_FATAL /**< You've done something wrong. You'd better look at it... */
} MSG_error_t;

typedef enum {
  MSG_SILENT = 0,
  MSG_SOME,
  MSG_VERBOSE
} MSG_outputmode_t;

/** \deprecated Network sharing mechanism
    \ingroup m_datatypes_management*/
typedef enum {
  MSG_STORE_AND_FORWARD = 1, /* 0 means uninitialized value */
  MSG_TCP
} MSG_sharing_t;

/** \deprecated Link datatype  
 *  \ingroup m_datatypes_management
 *    The notion of <em>link</em> was present in the earliest versions of MSG.  
 *    It was an agglomeration of communicating resources representing a set of
 *    physical network links. This abstraction a disappeared because in real-life,
 *    it is generally not possible to interact directly with a link...
 */
typedef struct m_link *m_link_t;

END_DECL()
#endif
