/* $Id$                     */

/* gras/core.h - Unsorted part of the GRAS public interface                 */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_CORE_H
#define GRAS_CORE_H

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
 * Garbage collection support
 * **************************************************************************/
typedef enum { free_after_use, free_never } e_gras_free_directive_t;

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
 * gras_process_finalize:
 * 
 * Perform the various intialisations needed by gras. Each process must run it
 */
gras_error_t gras_process_finalize(void);

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

/* **************************************************************************
 * Wrappers over OS functions
 * **************************************************************************/

/**
 * gras_get_my_fqdn:
 *
 * Returns the fully-qualified name of the host machine, or NULL if the name
 * cannot be determined.  Always returns the same value, so multiple calls
 * cause no problems.
 */
const char *
gras_get_my_fqdn(void);

/**
 * gras_time:
 * 
 * Get the time in number of second since the Epoch.
 * (00:00:00 UTC, January 1, 1970 in Real Life, and begining of simulation in SG)
 */
double gras_time(void);

/**
 * gras_sleep:
 * @sec: number of seconds to sleep
 * @usec: number of microseconds to sleep
 * 
 * sleeps for the given amount of seconds plus the given amount of microseconds.
 */
void gras_sleep(unsigned long sec, unsigned long usec);

END_DECL

#endif /* GRAS_CORE_H */

