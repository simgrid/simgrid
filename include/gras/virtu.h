/* $Id$                    */

/* gras/virtu.h - public interface to virtualization (cross-OS portability) */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_VIRTU_H
#define GRAS_VIRTU_H

#include "xbt/misc.h" /* BEGIN_DECL */

BEGIN_DECL

/**
 * gras_os_time:
 * @Returns: number of second since the Epoch.
 * (00:00:00 UTC, January 1, 1970 in Real Life, and begining of simulation in SG)
 * 
 * Get the current time.
 */
double gras_os_time(void);

/**
 * gras_os_sleep:
 * @Param1: number of seconds to sleep
 * @Param2: number of microseconds to sleep
 * 
 * sleeps for the given amount of time.
 */
void gras_os_sleep(unsigned long sec, unsigned long usec);

/**
 * gras_get_my_fqdn:
 *
 * Returns the fully-qualified name of the host machine, or NULL if the name
 * cannot be determined.  Always returns the same value, so multiple calls
 * cause no problems.
 */
const char *
gras_get_my_fqdn(void);

END_DECL

#endif /* GRAS_VIRTU_H */

