/* Copyright (c) 2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SRC_SURF_FILEIMPL_HPP_
#define SRC_SURF_FILEIMPL_HPP_

#include "surf/surf.h"
#include <string>

namespace simgrid {
namespace surf {

class FileImpl {
public:
  FileImpl(const char* path, const char* mount, sg_size_t size) : path_(path), mount_point_(mount), size_(size) {}
  ~FileImpl() = default;

  std::string name() { return path_; }
  const char* cname() { return path_.c_str(); }
  const char* mount() { return mount_point_.c_str(); }
  sg_size_t size() { return size_; }
  void setSize(sg_size_t size) { size_ = size; }
  void setPosition(sg_size_t size) { current_position_ = size; }
  void incrPosition(sg_size_t incr) { current_position_ += incr; }
  sg_size_t tell() { return current_position_; }
  int seek(sg_offset_t offset, int origin);
  int unlink(sg_host_t host);
  void move(sg_host_t host, const char* fullpath);

private:
  std::string path_;
  std::string mount_point_;
  sg_size_t size_;
  sg_size_t current_position_ = SEEK_SET;
};
}
}
#endif /* SRC_SURF_FILEIMPL_HPP_ */
