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
     typedef struct s_smx_mutex *smx_mutex_t;
     typedef struct s_smx_cond *smx_cond_t;

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

SG_END_DECL()
#endif
