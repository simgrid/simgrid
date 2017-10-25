/* xbt_os_file.c -- portable interface to file-related functions            */

/* Copyright (c) 2007-2010, 2012-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/file.h"    /* this module */
#include "xbt/log.h"
#include "src/internal_config.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include "libgen.h" /* POSIX dirname */

/** @brief Returns the directory component of a path (reimplementation of POSIX dirname)
 *
 * The argument is never modified, and the returned value must be freed after use.
 */
char *xbt_dirname(const char *path) {
  char *tmp = xbt_strdup(path);
  char *res = xbt_strdup(dirname(tmp));
  free(tmp);
  return res;
}

/** @brief Returns the file component of a path (reimplementation of POSIX basename)
 *
 * The argument is never modified, and the returned value must be freed after use.
 */
char *xbt_basename(const char *path) {
  char *tmp = xbt_strdup(path);
  char *res = xbt_strdup(basename(tmp));
  free(tmp);
  return res;
}
