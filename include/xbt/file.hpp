/* Copyright (c) 2017-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_FILE_HPP
#define XBT_FILE_HPP

#include <string>
#include <xbt/base.h>

namespace simgrid {
namespace xbt {

class Path {
public:
  /** Build a path from the current working directory (CWD) */
  explicit Path();
  /** Build a path from the provided parameter */
  explicit Path(const char* path): path_(path) {}
  /** Build a path from the provided parameter */
  explicit Path(const std::string& path) : path_(path) {}

  /** @brief Returns the full path name */
  const std::string& get_name() const { return path_; }
  /** @brief Returns the directory component of a path (reimplementation of POSIX dirname) */
  std::string get_dir_name() const;
  /** @brief Returns the file component of a path (reimplementation of POSIX basename) */
  std::string get_base_name() const;

private:
  std::string path_;
};
}}

#endif                          /* XBT_FILE_HPP */
