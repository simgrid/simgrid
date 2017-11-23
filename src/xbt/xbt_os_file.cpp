/* xbt_os_file.cpp -- portable interface to file-related functions            */

/* Copyright (c) 2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/file.hpp" /* this module */

#ifdef _WIN32
#include <windows.h>
#endif

#include <cstring>
#include <libgen.h> /* POSIX dirname */

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
