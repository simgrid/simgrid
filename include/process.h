/* $Id$                     */

/* gras/core.h - Unsorted part of the GRAS public interface                 */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_PROCESS_H
#define GRAS_PROCESS_H

#include <stddef.h>    /* offsetof() */
#include <sys/types.h>  /* size_t */
#include <stdarg.h>


/*! C++ users need love */
#ifndef BEGIN_DECL
# ifdef __cplusplus
#  define BEGIN_DECL extern "C" {
# else
#  define BEGIN_DECL 
# endif
#endif

/*! C++ users need love */
#ifndef END_DECL
# ifdef __cplusplus
#  define END_DECL }
# else
#  define END_DECL 
# endif
#endif
/* End of cruft for C++ */

BEGIN_DECL

/* **************************************************************************
 * Initializing the processes
 * **************************************************************************/
/**
 * gras_process_init:
 * 
 * Perform the various intialisations needed by gras. Each process must run it
 */
gras_error_t gras_process_init(void);

/**
 * gras_process_exit:
 * 
 * Frees the memory allocated by gras. Processes should run it
 */
gras_error_t gras_process_exit(void);

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
void *gras_userdata_set(void *ud);

/**
 * gras_userdata_new:
 *
 * Malloc and set the data associated with the current process.
 */

#define gras_userdata_new(type) gras_userdata_set(malloc(sizeof(type)))

END_DECL

#endif /* GRAS_PROCESS_H */

