/* xbt_os_file.cpp -- portable interface to file-related functions          */

/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"
#include "xbt/asserts.h"
#include "xbt/file.hpp" /* this module */

#if HAVE_UNISTD_H
#include <array>
#include <cerrno>
#include <unistd.h>
#endif

#include <boost/algorithm/string.hpp>
#include <cstring>
#include <fstream>
#include <libgen.h> /* POSIX dirname */

static std::vector<std::string> file_path;

void simgrid::xbt::path_push(std::string const& str)
{
  file_path.push_back(str);
}
void simgrid::xbt::path_pop()
{
  file_path.pop_back();
}
std::string simgrid::xbt::path_to_string()
{
  return boost::join(file_path, ":");
}
FILE* simgrid::xbt::path_fopen(const std::string& name, const char* mode)
{
  if (name[0] == '/') // don't mess with absolute file names
    return fopen(name.c_str(), mode);

  /* search relative files in the path */
  for (auto const& path_elm : file_path) {
    std::string buff = path_elm + "/" + name;
    FILE* file       = fopen(buff.c_str(), mode);

    if (file)
      return file;
  }
  return nullptr;
}

std::ifstream* simgrid::xbt::path_ifsopen(const std::string& name)
{
  xbt_assert(not name.empty());

  auto* fs = new std::ifstream();
  if (name[0] == '/') // don't mess with absolute file names
    fs->open(name.c_str(), std::ifstream::in);

  /* search relative files in the path */
  for (auto const& path_elm : file_path) {
    std::string buff = path_elm + "/" + name;
    fs->open(buff.c_str(), std::ifstream::in);

    if (not fs->fail())
      return fs;
  }

  return fs;
}

simgrid::xbt::Path::Path()
{
#if HAVE_UNISTD_H
  std::array<char, 2048> buffer;
  const char* cwd = getcwd(buffer.data(), 2048);
  xbt_assert(cwd != nullptr, "Error during getcwd: %s", strerror(errno));
  path_ = cwd;
#else
  path_ = ".";
#endif
}

std::string simgrid::xbt::Path::get_dir_name() const
{
  std::string p(path_);
  return dirname(&p[0]);
}

std::string simgrid::xbt::Path::get_base_name() const
{
  std::string p(path_);
  return basename(&p[0]);
}
