/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donnassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_DATATYPE_H
#define SIMIX_DATATYPE_H
#include "xbt/misc.h"
#include "xbt/swag.h"

SG_BEGIN_DECL()

/* ******************************** Host ************************************ */
/** @defgroup m_datatypes_management_details Details on SIMIX datatypes
    @ingroup  m_datatypes_management*/

typedef struct s_simdata_host *simdata_host_t;
/** @brief Host datatype 
    @ingroup m_datatypes_management_details */
typedef struct s_smx_host {
  char *name;			/**< @brief host name if any */
  simdata_host_t simdata;	/**< @brief simulator data */
  void *data;			/**< @brief user data */
} s_smx_host_t;
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

typedef struct s_smx_mutex *smx_mutex_t;
typedef struct s_smx_cond *smx_cond_t;


/********************************** Action *************************************/
typedef struct s_simdata_action *simdata_action_t;
/** @brief Action datatype 
    @ingroup m_datatypes_management_details */
typedef struct s_smx_action {
  char *name;			/**< @brief action name if any */
  simdata_action_t simdata;	/**< @brief simulator data */
  void *data;			/**< @brief user data */
} s_smx_action_t;

typedef struct s_smx_action *smx_action_t;


/* ****************************** Process *********************************** */
typedef struct s_simdata_process *simdata_process_t;
/** @brief Process datatype 
    @ingroup m_datatypes_management_details @{ */
typedef struct s_smx_process {
  char *name;			/**< @brief process name if any */
  simdata_process_t simdata;	/**< @brief simulator data */
  s_xbt_swag_hookup_t process_hookup;
  s_xbt_swag_hookup_t synchro_hookup;
  s_xbt_swag_hookup_t host_proc_hookup;
  void *data;			/**< @brief user data */
} s_smx_process_t;
/** @} */
/** @brief Agent datatype  
    @ingroup m_datatypes_management 

    An agent may be defined as a <em>code</em>, with some <em>private
    data</em>, executing in a <em>location</em>.
    \see m_process_management
  @{ */
typedef struct s_smx_process *smx_process_t;
/** @} */

/** @brief Agent code
    @ingroup m_datatypes_management 
    The code of an agent is a m_process_code_t, i.e. a function with no arguments 
    returning no value.
    \see m_process_management
  @{ */
typedef int(*smx_process_code_t)(int argc,char *argv[]) ;
/** @} */


/* ***************************** Error handling ***************************** */
/** @brief Error handling 
    @ingroup m_datatypes_management 
    @{
*/
typedef enum {
  SIMIX_OK = 0,  /**< @brief Everything is right. Keep on going this way ! */
  SIMIX_WARNING, /**< @brief Mmmh! Something must be not perfectly clean. But I
      may be a paranoid freak... ! */
  SIMIX_TRANSFER_FAILURE, /**< @brief There has been a problem during you action
      transfer. Either the network is down or the remote host has been
      shutdown. */
  SIMIX_HOST_FAILURE, /**< @brief System shutdown. The host on which you are
      running has just been rebooted. Free your datastructures and
      return now !*/
  SIMIX_ACTION_CANCELLED, /**< @brief Cancelled action. This action has been cancelled 
			by somebody!*/
  SIMIX_FATAL /**< @brief You've done something wrong. You'd better look at it... */
} SIMIX_error_t;
/** @} */

SG_END_DECL()
#endif
