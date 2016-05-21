/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_FILE_H
#define XBT_FILE_H

#include <stdint.h> /* ssize_t */
#include <stdarg.h>             /* va_* */
#include <stdio.h>  /* FILE */
#include <stdlib.h> /* size_t, ssize_t */
#include "xbt/misc.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"
#include "simgrid_config.h"     /* FILE for getline */

SG_BEGIN_DECL()

/** @defgroup XBT_file File manipulation functions
 *  @ingroup XBT_misc
 *
 * This module redefine some quite classical functions such as xbt_getline() or xbt_dirname() for the platforms
 * lacking them.
 * @{
 */
/* Our own implementation of getline, mainly useful on the platforms not enjoying this function */
XBT_PUBLIC(ssize_t) xbt_getline(char **lineptr, size_t * n, FILE * stream);

/* Our own implementation of dirname, that does not exist on windows */
XBT_PUBLIC(char *) xbt_dirname(const char *path);
XBT_PUBLIC(char *) xbt_basename(const char *path);

/**@}*/

SG_END_DECL()
#endif                          /* XBT_FILE_H */
