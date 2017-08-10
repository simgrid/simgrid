/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_FILE_HPP
#define SIMGRID_S4U_FILE_HPP

#include <xbt/base.h>

#include <simgrid/simix.h>
#include <string>

namespace simgrid {
namespace s4u {

/** @brief A simulated file
 *
 * Used to simulate the time it takes to access to a file, but does not really store any information.
 *
 * They are located on @ref simgrid::s4u::Storage that are accessed from a given @ref simgrid::s4u::Host through
 * mountpoints.
 * For now, you cannot change the mountpoints programatically, and must declare them from your platform file.
 */
XBT_PUBLIC_CLASS File
{
public:
  File(std::string fullpath, void* userdata);
  File(std::string fullpath, sg_host_t host, void* userdata);
  ~File();

  /** Retrieves the path to the file */
  const char* getPath() { return path_.c_str(); }

  /** Simulates a local read action. Returns the size of data actually read */
  sg_size_t read(sg_size_t size);

  /** Simulates a write action. Returns the size of data actually written. */
  sg_size_t write(sg_size_t size);

  /** Allows to store user data on that host */
  void setUserdata(void* data) { userdata_ = data; }
  /** Retrieves the previously stored data */
  void* getUserdata() { return userdata_; }

  /** Retrieve the datasize */
  sg_size_t size();

  /** Sets the file head to the given position. */
  void seek(sg_offset_t pos);
  void seek(sg_offset_t pos, int origin);

  /** Retrieves the current file position */
  sg_size_t tell();

  /** Rename a file. WARNING: It is forbidden to move the file to another mount point */
  void move(std::string fullpath);

  /** Remove a file from disk */
  int unlink();

  std::string storage_type;
  std::string storageId;
  std::string mount_point;
  int desc_id = 0;

private:
  surf_file_t pimpl_ = nullptr;
  std::string path_;
  void* userdata_ = nullptr;
};
}
} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_HOST_HPP */
