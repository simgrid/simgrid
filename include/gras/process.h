/* $Id$ */

/* gras/process.h - Manipulating data related to an host.                   */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_PROCESS_H
#define GRAS_PROCESS_H

#include "xbt/misc.h" /* BEGIN_DECL */

BEGIN_DECL

/* **************************************************************************
 * Initializing the processes
 * **************************************************************************/
/**
 * gras_process_init:
 * 
 * Perform the various intialisations needed by gras. Each process must run it
 */
xbt_error_t gras_process_init(void);

/**
 * gras_process_exit:
 * 
 * Frees the memory allocated by gras. Processes should run it
 */
xbt_error_t gras_process_exit(void);

/****************************************************************************/
/* Manipulating User Data                                                   */
/****************************************************************************/
/**
 * gras_userdata_get:
 *
 * Get the data associated with the current process.
 */
void *gras_userdata_get(void);

/**
 * gras_userdata_set:
 *
 * Set the data associated with the current process.
 */
void gras_userdata_set(void *ud);

/**
 * gras_userdata_new:
 *
 * Malloc and set the data associated with the current process.
 */

#define gras_userdata_new(type) (gras_userdata_set(xbt_new0(type,1)),gras_userdata_get())

END_DECL

#endif /* GRAS_PROCESS_H */

