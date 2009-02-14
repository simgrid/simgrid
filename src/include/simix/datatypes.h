/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donnassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_DATATYPE_H
#define SIMIX_DATATYPE_H
#include "xbt/misc.h"
#include "xbt/swag.h"
#include "xbt/fifo.h"

SG_BEGIN_DECL()

/* ******************************** Host ************************************ */
/** @defgroup m_datatypes_management_details Details on SIMIX datatypes
    @ingroup  m_datatypes_management*/

typedef struct s_smx_simdata_host *smx_simdata_host_t;
/** @brief Host datatype 
    @ingroup m_datatypes_management_details */
typedef struct s_smx_host {
  char *name;			/**< @brief host name if any */
  smx_simdata_host_t simdata;	/**< @brief simulator data */
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
typedef struct s_smx_simdata_action *smx_simdata_action_t;
/** @brief Action datatype 
    @ingroup m_datatypes_management_details */
typedef struct s_smx_action {
  char *name;			/**< @brief action name if any */
  smx_simdata_action_t simdata;	/**< @brief simulator data */
	xbt_fifo_t cond_list;   /*< conditional variables that must be signaled when the action finish. */
  void *data;			/**< @brief user data */
  int refcount;			/**< @brief reference counter */
} s_smx_action_t;

typedef struct s_smx_action *smx_action_t;


/* ****************************** Process *********************************** */
typedef struct s_smx_simdata_process *smx_simdata_process_t;
/** @brief Process datatype 
    @ingroup m_datatypes_management_details @{ */
typedef struct s_smx_process {

   char *name;			/**< @brief process name if any */
   smx_simdata_process_t simdata;	/**< @brief simulator data */
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

SG_END_DECL()
#endif
