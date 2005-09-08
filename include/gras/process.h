/* $Id$ */

/* gras/process.h - Manipulating data related to an host.                   */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_PROCESS_H
#define GRAS_PROCESS_H

#include "xbt/misc.h"  /* SG_BEGIN_DECL */

SG_BEGIN_DECL()

/* **************************************************************************
 * Initializing the processes
 * **************************************************************************/
/**
 * gras_process_init:
 * 
 * Perform the various intialisations needed by gras. Each process must run it
 */
void gras_process_init(void);

/**
 * gras_process_exit:
 * 
 * Frees the memory allocated by gras. Processes should run it
 */
void gras_process_exit(void);

/****************************************************************************/
/* Manipulating User Data                                                   */
/****************************************************************************/

/** \addtogroup GRAS_globals
 *  \brief Handling global variables so that it works on simulator (Virtualization).
 * 
 * In GRAS, using globals is forbidden since the "processes" will
 * sometimes run as a thread inside the same process (namely, in
 * simulation mode). So, you have to put all globals in a structure, and
 * let GRAS handle it.
 * 
 * Use the \ref gras_userdata_new macro to create a new user data (or malloc it
 * and use \ref gras_userdata_set yourself), and \ref gras_userdata_get to
 * retrive a reference to it. 
 */
/* @{ */

/**
 * \brief Get the data associated with the current process.
 * \ingroup GRAS_globals
 */
void *gras_userdata_get(void);

/**
 * \brief Set the data associated with the current process.
 * \ingroup GRAS_globals
 */
void gras_userdata_set(void *ud);

/** \brief Malloc and set the data associated with the current process. */
#define gras_userdata_new(type) (gras_userdata_set(xbt_new0(type,1)),gras_userdata_get())
/* @} */

SG_END_DECL()

#endif /* GRAS_PROCESS_H */

