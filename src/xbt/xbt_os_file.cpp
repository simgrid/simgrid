/* xbt_os_file.cpp -- portable interface to file-related functions            */

/* Copyright (c) 2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/file.hpp" /* this module */

#include "xbt/file.h"
#include "xbt/sysdep.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <cstring>
#include <libgen.h> /* POSIX dirname */

/** @brief Returns the directory component of a path (reimplementation of POSIX dirname)
 *
 * The argument is never modified, and the returned value must be freed after use.
 */
char *xbt_dirname(const char *path)
{
  return xbt_strdup(simgrid::xbt::Path(path).getDirname().c_str());
}

/** @brief Returns the file component of a path (reimplementation of POSIX basename)
 *
 * The argument is never modified, and the returned value must be freed after use.
 */
char *xbt_basename(const char *path)
{
  return xbt_strdup(simgrid::xbt::Path(path).getBasename().c_str());
}

std::string simgrid::xbt::Path::getDirname()
{
  std::string p(path_);
  char *res = dirname(&p[0]);
  return std::string(res, strlen(res));
}

std::string simgrid::xbt::Path::getBasename()
{
  std::string p(path_);
  char *res = basename(&p[0]);
  return std::string(res, strlen(res));
}
