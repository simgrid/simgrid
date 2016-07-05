/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_FILE_HPP
#define SIMGRID_S4U_FILE_HPP

#include <xbt/base.h>

#include <simgrid/simix.h>

namespace simgrid {
namespace s4u {

class Actor;
class Storage;

/** @brief A simulated file
 *
 * Used to simulate the time it takes to access to a file, but does not really store any information.
 *
 * They are located on @link{simgrid::s4u::Storage}, that are accessed from a given @link{simgrid::s4u::Host} through mountpoints.
 * For now, you cannot change the mountpoints programatically, and must declare them from your platform file.
 */
XBT_PUBLIC_CLASS File {
public:
  File(const char *fullpath, void* userdata);
  ~File();

  /** Retrieves the path to the file */
  const char *path() { return path_;}

  /** Simulates a read action. Returns the size of data actually read
   *
   *  FIXME: reading from a remotely mounted disk is not implemented yet.
   *  Any storage is considered as local, and no network communication ever occur.
   */
  sg_size_t read(sg_size_t size);
  /** Simulates a write action. Returns the size of data actually written.
   *
   *  FIXME: reading from a remotely mounted disk is not implemented yet.
   *  Any storage is considered as local, and no network communication ever occur.
   */
  sg_size_t write(sg_size_t size);

  /** Allows to store user data on that host */
  void setUserdata(void *data) {userdata_ = data;}
  /** Retrieves the previously stored data */
  void* userdata() {return userdata_;}

  /** Retrieve the datasize */
  sg_size_t size();

  /** Sets the file head to the given position. */
  void seek(sg_size_t pos);
  /** Retrieves the current file position */
  sg_size_t tell();

  /** Rename a file
   *
   * WARNING: It is forbidden to move the file to another mount point */
  void move(const char*fullpath);

  /** Remove a file from disk */
  void unlink();

  /* FIXME: add these to the S4U API:
  XBT_PUBLIC(const char *) MSG_file_get_name(msg_file_t file);
  XBT_PUBLIC(msg_error_t) MSG_file_rcopy(msg_file_t fd, msg_host_t host, const char* fullpath);
  XBT_PUBLIC(msg_error_t) MSG_file_rmove(msg_file_t fd, msg_host_t host, const char* fullpath);
  */

private:
  smx_file_t pimpl_ = nullptr;
  const char *path_ = nullptr;
  void *userdata_ = nullptr;
};

}} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_HOST_HPP */
