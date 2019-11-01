/* Copyright (c) 2017-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLUGINS_FILE_SYSTEM_H_
#define SIMGRID_PLUGINS_FILE_SYSTEM_H_

#include <simgrid/forward.h>
#include <xbt/base.h>
#include <xbt/dict.h>

#ifdef __cplusplus
#include <xbt/Extendable.hpp>

#include <map>
#include <memory>
#include <string>
#endif

// C interface
////////////////
typedef sg_file_t msg_file_t; // MSG backwards compatibility

SG_BEGIN_DECL
XBT_PUBLIC void sg_storage_file_system_init();
XBT_PUBLIC sg_file_t sg_file_open(const char* fullpath, void* data);
XBT_PUBLIC sg_size_t sg_file_read(sg_file_t fd, sg_size_t size);
XBT_PUBLIC sg_size_t sg_file_write(sg_file_t fd, sg_size_t size);
XBT_PUBLIC void sg_file_close(sg_file_t fd);

XBT_PUBLIC const char* sg_file_get_name(sg_file_t fd);
XBT_PUBLIC sg_size_t sg_file_get_size(sg_file_t fd);
XBT_PUBLIC void sg_file_dump(sg_file_t fd);
XBT_PUBLIC void* sg_file_get_data(sg_file_t fd);
XBT_PUBLIC void sg_file_set_data(sg_file_t fd, void* data);
XBT_PUBLIC void sg_file_seek(sg_file_t fd, sg_offset_t offset, int origin);
XBT_PUBLIC sg_size_t sg_file_tell(sg_file_t fd);
XBT_PUBLIC void sg_file_move(sg_file_t fd, const char* fullpath);
XBT_PUBLIC void sg_file_unlink(sg_file_t fd);
XBT_PUBLIC int sg_file_rcopy(sg_file_t file, sg_host_t host, const char* fullpath);
XBT_PUBLIC int sg_file_rmove(sg_file_t file, sg_host_t host, const char* fullpath);

XBT_PUBLIC sg_size_t sg_disk_get_size_free(sg_disk_t d);
XBT_PUBLIC sg_size_t sg_disk_get_size_used(sg_disk_t d);
XBT_PUBLIC sg_size_t sg_disk_get_size(sg_disk_t d);
XBT_PUBLIC const char* sg_disk_get_mount_point(sg_disk_t d);

XBT_PUBLIC sg_size_t sg_storage_get_size_free(sg_storage_t st);
XBT_PUBLIC sg_size_t sg_storage_get_size_used(sg_storage_t st);
XBT_PUBLIC sg_size_t sg_storage_get_size(sg_storage_t st);
XBT_PUBLIC xbt_dict_t sg_storage_get_content(sg_storage_t storage);

XBT_PUBLIC xbt_dict_t sg_host_get_storage_content(sg_host_t host);

#define MSG_file_open(fullpath, data) sg_file_open((fullpath), (data))
#define MSG_file_read(fd, size) sg_file_read((fd), (size))
#define MSG_file_write(fd, size) sg_file_write((fd), (size))
#define MSG_file_close(fd) sg_file_close(fd)
#define MSG_file_get_name(fd) sg_file_get_name(fd)
#define MSG_file_get_size(fd) sg_file_get_size(fd)
#define MSG_file_dump(fd) sg_file_dump(fd)
#define MSG_file_get_data(fd) sg_file_get_data(fd)
#define MSG_file_set_data(fd, data) sg_file_set_data((fd), (data))
#define MSG_file_seek(fd, offset, origin) sg_file_seek((fd), (offset), (origin))
#define MSG_file_tell(fd) sg_file_tell(fd)
#define MSG_file_move(fd, fullpath) sg_file_get_size((fd), (fullpath))
#define MSG_file_unlink(fd) sg_file_unlink(fd)
#define MSG_file_rcopy(file, host, fullpath) sg_file_rcopy((file), (host), (fullpath))
#define MSG_file_rmove(file, host, fullpath) sg_file_rmove((file), (host), (fullpath))

#define MSG_storage_file_system_init() sg_storage_file_system_init()
#define MSG_storage_get_free_size(st) sg_storage_get_size_free(st)
#define MSG_storage_get_used_size(st) sg_storage_get_size_used(st)
#define MSG_storage_get_size(st) sg_storage_get_size(st)
#define MSG_storage_get_content(st) sg_storage_get_content(st)

#define MSG_host_get_storage_content(st) sg_host_get_storage_content(st)

SG_END_DECL

// C++ interface
//////////////////

#ifdef __cplusplus

namespace simgrid {
namespace s4u {

/** @brief A simulated file
 *
 * Used to simulate the time it takes to access to a file, but does not really store any information.
 *
 * They are located on @ref simgrid::s4u::Storage that are accessed from a given @ref simgrid::s4u::Host through
 * mountpoints.
 * For now, you cannot change the mountpoints programmatically, and must declare them from your platform file.
 */
class XBT_PUBLIC File {
public:
  File(const std::string& fullpath, void* userdata);
  File(const std::string& fullpath, sg_host_t host, void* userdata);
  File(const File&) = delete;
  File& operator=(const File&) = delete;
  ~File();

  /** Retrieves the path to the file */
  const char* get_path() { return fullpath_.c_str(); }

  /** Simulates a local read action. Returns the size of data actually read */
  sg_size_t read(sg_size_t size);

  /** Simulates a write action. Returns the size of data actually written. */
  sg_size_t write(sg_size_t size, int write_inside=0);

  /** Allows to store user data on that host */
  void set_userdata(void* data) { userdata_ = data; }
  /** Retrieves the previously stored data */
  void* get_userdata() { return userdata_; }

  sg_size_t size();
  void seek(sg_offset_t pos);             /** Sets the file head to the given position. */
  void seek(sg_offset_t pos, int origin); /** Sets the file head to the given position from a given origin. */
  sg_size_t tell();                       /** Retrieves the current file position */

  /** Rename a file. WARNING: It is forbidden to move the file to another mount point */
  void move(const std::string& fullpath);
  int remote_copy(sg_host_t host, const char* fullpath);
  int remote_move(sg_host_t host, const char* fullpath);

  int unlink(); /** Remove a file from the contents of a disk */
  void dump();

  int desc_id = 0;
  Disk* local_disk_       = nullptr;
  Storage* local_storage_ = nullptr;
  std::string mount_point_;

private:
  sg_size_t size_;
  std::string path_;
  std::string fullpath_;
  sg_size_t current_position_ = SEEK_SET;
  void* userdata_             = nullptr;
};

class XBT_PUBLIC FileSystemDiskExt {
public:
  static simgrid::xbt::Extension<Disk, FileSystemDiskExt> EXTENSION_ID;
  explicit FileSystemDiskExt(Disk* ptr);
  FileSystemDiskExt(const FileSystemDiskExt&) = delete;
  FileSystemDiskExt& operator=(const FileSystemDiskExt&) = delete;
  std::map<std::string, sg_size_t>* parse_content(const std::string& filename);
  std::map<std::string, sg_size_t>* get_content() const { return content_.get(); }
  const char* get_mount_point() { return mount_point_.c_str(); }
  const char* get_mount_point(s4u::Host* remote_host) { return remote_mount_points_[remote_host].c_str(); }
  void add_remote_mount(Host* host, const std::string& mount_point)
  {
    remote_mount_points_.insert({host, mount_point});
  }
  sg_size_t get_size() const { return size_; }
  sg_size_t get_used_size() const { return used_size_; }
  void decr_used_size(sg_size_t size) { used_size_ -= size; }
  void incr_used_size(sg_size_t size) { used_size_ += size; }

private:
  std::unique_ptr<std::map<std::string, sg_size_t>> content_;
  std::map<Host*, std::string> remote_mount_points_;
  std::string mount_point_;
  sg_size_t used_size_ = 0;
  sg_size_t size_      = static_cast<sg_size_t>(500 * 1024) * 1024 * 1024;
};

class XBT_PUBLIC FileSystemStorageExt {
public:
  static simgrid::xbt::Extension<Storage, FileSystemStorageExt> EXTENSION_ID;
  explicit FileSystemStorageExt(Storage* ptr);
  FileSystemStorageExt(const FileSystemStorageExt&) = delete;
  FileSystemStorageExt& operator=(const FileSystemStorageExt&) = delete;
  std::map<std::string, sg_size_t>* parse_content(const std::string& filename);
  std::map<std::string, sg_size_t>* get_content() { return content_.get(); }
  sg_size_t get_size() { return size_; }
  sg_size_t get_used_size() { return used_size_; }
  void decr_used_size(sg_size_t size) { used_size_ -= size; }
  void incr_used_size(sg_size_t size) { used_size_ += size; }

private:
  std::unique_ptr<std::map<std::string, sg_size_t>> content_;
  sg_size_t used_size_ = 0;
  sg_size_t size_     = 0;
};

class XBT_PUBLIC FileDescriptorHostExt {
public:
  static simgrid::xbt::Extension<Host, FileDescriptorHostExt> EXTENSION_ID;
  FileDescriptorHostExt() = default;
  FileDescriptorHostExt(const FileDescriptorHostExt&) = delete;
  FileDescriptorHostExt& operator=(const FileDescriptorHostExt&) = delete;
  std::unique_ptr<std::vector<int>> file_descriptor_table        = nullptr; // Created lazily on need
};
} // namespace s4u
} // namespace simgrid
#endif
#endif
