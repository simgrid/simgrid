/* $Id$                     */

/* gras/virtu.h - public interface to virtualization (cross-OS portability) */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003,2004 da GRAS posse.                                   */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_VIRTU_H
#define GRAS_VIRTU_H

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

#endif /* GRAS_VIRTU_H */

