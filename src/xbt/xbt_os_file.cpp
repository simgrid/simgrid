/* xbt_os_file.cpp -- portable interface to file-related functions          */

/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"
#include "xbt/asserts.h"
#include "xbt/file.hpp" /* this module */

#ifdef _WIN32
#include <windows.h>
#endif

#if HAVE_UNISTD_H
#include <array>
#include <cerrno>
#include <unistd.h>
#endif

#include <cstring>
#include <libgen.h> /* POSIX dirname */

simgrid::xbt::Path::Path()
{
#if HAVE_UNISTD_H
  std::array<char, 2048> buffer;
  const char* cwd = getcwd(buffer.data(), 2048);
  xbt_assert(cwd != nullptr, "Error during getcwd: %s", strerror(errno));
  path_ = std::string(cwd);
#else
  path_ = std::string(".");
#endif
}

std::string simgrid::xbt::Path::get_dir_name() const
{
  std::string p(path_);
  const char* res = dirname(&p[0]);
  return std::string(res, strlen(res));
}

std::string simgrid::xbt::Path::get_base_name() const
{
  std::string p(path_);
  const char* res = basename(&p[0]);
  return std::string(res, strlen(res));
}
