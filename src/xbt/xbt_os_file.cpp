/* xbt_os_file.cpp -- portable interface to file-related functions          */

/* Copyright (c) 2017-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"
#include "xbt/file.hpp" /* this module */

#ifdef _WIN32
#include <windows.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cstring>
#include <libgen.h> /* POSIX dirname */

simgrid::xbt::Path::Path()
{
#if HAVE_UNISTD_H
  char buffer[2048];
  getcwd(buffer, 2048);
  path_ = std::string(buffer);
#else
  path_ = std::string(".");
#endif
}

std::string simgrid::xbt::Path::get_dir_name()
{
  std::string p(path_);
  char *res = dirname(&p[0]);
  return std::string(res, strlen(res));
}

std::string simgrid::xbt::Path::get_base_name()
{
  std::string p(path_);
  char *res = basename(&p[0]);
  return std::string(res, strlen(res));
}
