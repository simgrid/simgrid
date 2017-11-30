/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_FILE_HPP
#define SIMGRID_S4U_FILE_HPP

#include "simgrid/plugins/file_system.h"
#include <xbt/Extendable.hpp>
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
  const char* getPath() { return fullpath_.c_str(); }

  /** Simulates a local read action. Returns the size of data actually read */
  sg_size_t read(sg_size_t size);

  /** Simulates a write action. Returns the size of data actually written. */
  sg_size_t write(sg_size_t size);

  /** Allows to store user data on that host */
  void setUserdata(void* data) { userdata_ = data; }
  /** Retrieves the previously stored data */
  void* getUserdata() { return userdata_; }

  sg_size_t size();
  void seek(sg_offset_t pos);             /** Sets the file head to the given position. */
  void seek(sg_offset_t pos, int origin); /** Sets the file head to the given position from a given origin. */
  sg_size_t tell();                       /** Retrieves the current file position */

  /** Rename a file. WARNING: It is forbidden to move the file to another mount point */
  void move(std::string fullpath);
  int remoteCopy(sg_host_t host, const char* fullpath);
  int remoteMove(sg_host_t host, const char* fullpath);

  int unlink(); /** Remove a file from the contents of a disk */
  void dump();

  int desc_id = 0;
  Storage* localStorage;
  std::string mount_point_;

private:
  sg_size_t size_;
  std::string path_;
  std::string fullpath_;
  sg_size_t current_position_ = SEEK_SET;
  void* userdata_             = nullptr;
};

XBT_PUBLIC_CLASS FileSystemStorageExt {
public:
  static simgrid::xbt::Extension<Storage, FileSystemStorageExt> EXTENSION_ID;
  explicit FileSystemStorageExt(Storage* ptr);
  ~FileSystemStorageExt();
  std::map<std::string, sg_size_t>* parseContent(std::string filename);
  std::map<std::string, sg_size_t>* getContent() { return content_; }
  sg_size_t getSize() { return size_; }
  sg_size_t getUsedSize() { return usedSize_; }
  void decrUsedSize(sg_size_t size) { usedSize_ -= size; }
  void incrUsedSize(sg_size_t size) { usedSize_ += size; }
private:
  std::map<std::string, sg_size_t>* content_;
  sg_size_t usedSize_ = 0;
  sg_size_t size_     = 0;
};

XBT_PUBLIC_CLASS FileDescriptorHostExt
{
public:
  static simgrid::xbt::Extension<Host, FileDescriptorHostExt> EXTENSION_ID;
  FileDescriptorHostExt() = default;
  ~FileDescriptorHostExt() { delete file_descriptor_table; }
  std::vector<int>* file_descriptor_table = nullptr; // Created lazily on need
};
}
} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_HOST_HPP */
