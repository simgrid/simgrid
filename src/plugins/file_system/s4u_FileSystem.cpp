/* Copyright (c) 2015-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/file_system.h"
#include "simgrid/s4u/Actor.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "src/surf/HostImpl.hpp"
#include "src/surf/xml/platf_private.hpp"
#include "xbt/config.hpp"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <fstream>
#include <numeric>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_file, "S4U files");
int sg_storage_max_file_descriptors = 1024;

namespace simgrid {
namespace s4u {
simgrid::xbt::Extension<Disk, FileSystemDiskExt> FileSystemDiskExt::EXTENSION_ID;
simgrid::xbt::Extension<Storage, FileSystemStorageExt> FileSystemStorageExt::EXTENSION_ID;
simgrid::xbt::Extension<Host, FileDescriptorHostExt> FileDescriptorHostExt::EXTENSION_ID;

File::File(const std::string& fullpath, void* userdata) : File(fullpath, Host::current(), userdata){};

File::File(const std::string& fullpath, sg_host_t host, void* userdata) : fullpath_(fullpath), userdata_(userdata)
{
  // this cannot fail because we get a xbt_die if the mountpoint does not exist
  if (not host->get_mounted_storages().empty()) {
    Storage* st                  = nullptr;
    size_t longest_prefix_length = 0;
    XBT_DEBUG("Search for storage name for '%s' on '%s'", fullpath_.c_str(), host->get_cname());

    for (auto const& mnt : host->get_mounted_storages()) {
      XBT_DEBUG("See '%s'", mnt.first.c_str());
      mount_point_ = fullpath_.substr(0, mnt.first.length());

      if (mount_point_ == mnt.first && mnt.first.length() > longest_prefix_length) {
        /* The current mount name is found in the full path and is bigger than the previous*/
        longest_prefix_length = mnt.first.length();
        st                    = mnt.second;
      }
    }
    if (longest_prefix_length > 0) { /* Mount point found, split fullpath_ into mount_name and path+filename*/
      mount_point_ = fullpath_.substr(0, longest_prefix_length);
      path_        = fullpath_.substr(longest_prefix_length, fullpath_.length());
    } else
      xbt_die("Can't find mount point for '%s' on '%s'", fullpath_.c_str(), host->get_cname());

    local_storage_ = st;
  }
  if (not host->get_disks().empty()) {
    Disk* d                      = nullptr;
    size_t longest_prefix_length = 0;
    for (auto const& disk : host->get_disks()) {
      std::string current_mount;
      if (disk->get_host() != host)
        current_mount = disk->extension<FileSystemDiskExt>()->get_mount_point(disk->get_host());
      else
        current_mount = disk->extension<FileSystemDiskExt>()->get_mount_point();
      mount_point_              = fullpath_.substr(0, current_mount.length());
      if (mount_point_ == current_mount && current_mount.length() > longest_prefix_length) {
        /* The current mount name is found in the full path and is bigger than the previous*/
        longest_prefix_length = current_mount.length();
        d                     = disk;
      }
      if (longest_prefix_length > 0) { /* Mount point found, split fullpath_ into mount_name and path+filename*/
        mount_point_ = fullpath_.substr(0, longest_prefix_length);
        if (mount_point_ == std::string("/"))
          path_ = fullpath_;
        else
          path_ = fullpath_.substr(longest_prefix_length, fullpath_.length());
        XBT_DEBUG("%s + %s", mount_point_.c_str(), path_.c_str());
      } else
        xbt_die("Can't find mount point for '%s' on '%s'", fullpath_.c_str(), host->get_cname());
    }
    local_disk_ = d;
  }

  // assign a file descriptor id to the newly opened File
  FileDescriptorHostExt* ext = host->extension<simgrid::s4u::FileDescriptorHostExt>();
  if (ext->file_descriptor_table == nullptr) {
    ext->file_descriptor_table.reset(new std::vector<int>(sg_storage_max_file_descriptors));
    std::iota(ext->file_descriptor_table->rbegin(), ext->file_descriptor_table->rend(), 0); // Fill with ..., 1, 0.
  }
  xbt_assert(not ext->file_descriptor_table->empty(), "Too much files are opened! Some have to be closed.");
  desc_id = ext->file_descriptor_table->back();
  ext->file_descriptor_table->pop_back();

  XBT_DEBUG("\tOpen file '%s'", path_.c_str());
  std::map<std::string, sg_size_t>* content = nullptr;
  if (local_storage_)
    content = local_storage_->extension<FileSystemStorageExt>()->get_content();

  if (local_disk_)
    content = local_disk_->extension<FileSystemDiskExt>()->get_content();

  // if file does not exist create an empty file
  if (content) {
    auto sz = content->find(path_);
    if (sz != content->end()) {
      size_ = sz->second;
    } else {
      size_ = 0;
      content->insert({path_, size_});
      XBT_DEBUG("File '%s' was not found, file created.", path_.c_str());
    }
  }
}

File::~File()
{
  Host::current()->extension<simgrid::s4u::FileDescriptorHostExt>()->file_descriptor_table->push_back(desc_id);
}

void File::dump()
{
  if (local_storage_)
    XBT_INFO("File Descriptor information:\n"
             "\t\tFull path: '%s'\n"
             "\t\tSize: %llu\n"
             "\t\tMount point: '%s'\n"
             "\t\tStorage Id: '%s'\n"
             "\t\tStorage Type: '%s'\n"
             "\t\tFile Descriptor Id: %d",
             get_path(), size_, mount_point_.c_str(), local_storage_->get_cname(), local_storage_->get_type(), desc_id);
  if (local_disk_)
    XBT_INFO("File Descriptor information:\n"
             "\t\tFull path: '%s'\n"
             "\t\tSize: %llu\n"
             "\t\tMount point: '%s'\n"
             "\t\tDisk Id: '%s'\n"
             "\t\tHost Id: '%s'\n"
             "\t\tFile Descriptor Id: %d",
             get_path(), size_, mount_point_.c_str(), local_disk_->get_cname(), local_disk_->get_host()->get_cname(),
             desc_id);
}

sg_size_t File::read(sg_size_t size)
{
  if (size_ == 0) /* Nothing to read, return */
    return 0;
  sg_size_t read_size = 0;
  Host* host          = nullptr;
  if (local_storage_) {
    /* Find the host where the file is physically located and read it */
    host = local_storage_->get_host();
    XBT_DEBUG("READ %s on disk '%s'", get_path(), local_storage_->get_cname());
    // if the current position is close to the end of the file, we may not be able to read the requested size
    read_size = local_storage_->read(std::min(size, size_ - current_position_));
    current_position_ += read_size;
  }

  if (local_disk_) {
    /* Find the host where the file is physically located and read it */
    host = local_disk_->get_host();
    XBT_DEBUG("READ %s on disk '%s'", get_path(), local_disk_->get_cname());
    // if the current position is close to the end of the file, we may not be able to read the requested size
    read_size = local_disk_->read(std::min(size, size_ - current_position_));
    current_position_ += read_size;
  }

  if (host && host->get_name() != Host::current()->get_name() && read_size > 0) {
    /* the file is hosted on a remote host, initiate a communication between src and dest hosts for data transfer */
    XBT_DEBUG("File is on %s remote host, initiate data transfer of %llu bytes.", host->get_cname(), read_size);
    host->send_to(Host::current(), read_size);
  }

  return read_size;
}

/** @brief Write into a file (local or remote)
 *
 * @param size of the file to write
 * @return the number of bytes successfully write or -1 if an error occurred
 */
sg_size_t File::write(sg_size_t size, int write_inside)
{
  if (size == 0) /* Nothing to write, return */
    return 0;
  sg_size_t write_size = 0;
  Host* host           = nullptr;

  /* Find the host where the file is physically located (remote or local)*/
  if (local_storage_)
    host = local_storage_->get_host();
  if (local_disk_)
    host = local_disk_->get_host();

  if (host && host->get_name() != Host::current()->get_name()) {
    /* the file is hosted on a remote host, initiate a communication between src and dest hosts for data transfer */
    XBT_DEBUG("File is on %s remote host, initiate data transfer of %llu bytes.", host->get_cname(), size);
    Host::current()->send_to(host, size);
  }

  if (local_storage_) {
    XBT_DEBUG("WRITE %s on disk '%s'. size '%llu/%llu' '%llu:%llu'", get_path(), local_storage_->get_cname(), size,
              size_, sg_storage_get_size_used(local_storage_), sg_storage_get_size(local_storage_));
    // If the storage is full before even starting to write
    if (sg_storage_get_size_used(local_storage_) >= sg_storage_get_size(local_storage_))
      return 0;
    if (write_inside == 0) {
      /* Subtract the part of the file that might disappear from the used sized on the storage element */
      local_storage_->extension<FileSystemStorageExt>()->decr_used_size(size_ - current_position_);
      write_size = local_storage_->write(size);
      local_storage_->extension<FileSystemStorageExt>()->incr_used_size(write_size);
      current_position_ += write_size;
      size_ = current_position_;
    } else {
      write_size = local_storage_->write(size);
      current_position_ += write_size;
      if (current_position_ > size_)
        size_ = current_position_;
    }
    std::map<std::string, sg_size_t>* content = local_storage_->extension<FileSystemStorageExt>()->get_content();

    content->erase(path_);
    content->insert({path_, size_});
  }

  if (local_disk_) {
    XBT_DEBUG("WRITE %s on disk '%s'. size '%llu/%llu' '%llu:%llu'", get_path(), local_disk_->get_cname(), size, size_,
              sg_disk_get_size_used(local_disk_), sg_disk_get_size(local_disk_));
    // If the storage is full before even starting to write
    if (sg_disk_get_size_used(local_disk_) >= sg_disk_get_size(local_disk_))
      return 0;
    if (write_inside == 0) {
      /* Subtract the part of the file that might disappear from the used sized on the storage element */
      local_disk_->extension<FileSystemDiskExt>()->decr_used_size(size_ - current_position_);
      write_size = local_disk_->write(size);
      local_disk_->extension<FileSystemDiskExt>()->incr_used_size(write_size);
      current_position_ += write_size;
      size_ = current_position_;
    } else {
      write_size = local_disk_->write(size);
      current_position_ += write_size;
      if (current_position_ > size_)
        size_ = current_position_;
    }
    std::map<std::string, sg_size_t>* content = local_disk_->extension<FileSystemDiskExt>()->get_content();

    content->erase(path_);
    content->insert({path_, size_});
  }
  return write_size;
}

sg_size_t File::size()
{
  return size_;
}

void File::seek(sg_offset_t offset)
{
  current_position_ = offset;
}

void File::seek(sg_offset_t offset, int origin)
{
  switch (origin) {
    case SEEK_SET:
      current_position_ = offset;
      break;
    case SEEK_CUR:
      current_position_ += offset;
      break;
    case SEEK_END:
      current_position_ = size_ + offset;
      break;
    default:
      break;
  }
}

sg_size_t File::tell()
{
  return current_position_;
}

void File::move(const std::string& fullpath)
{
  /* Check if the new full path is on the same mount point */
  if (fullpath.compare(0, mount_point_.length(), mount_point_) == 0) {
    std::map<std::string, sg_size_t>* content = nullptr;
    if (local_storage_)
      content = local_storage_->extension<FileSystemStorageExt>()->get_content();
    if (local_disk_)
      content = local_disk_->extension<FileSystemDiskExt>()->get_content();
    if (content) {
      auto sz = content->find(path_);
      if (sz != content->end()) { // src file exists
        sg_size_t new_size = sz->second;
        content->erase(path_);
        std::string path = fullpath.substr(mount_point_.length(), fullpath.length());
        content->insert({path.c_str(), new_size});
        XBT_DEBUG("Move file from %s to %s, size '%llu'", path_.c_str(), fullpath.c_str(), new_size);
      } else {
        XBT_WARN("File %s doesn't exist", path_.c_str());
      }
    }
  } else {
    XBT_WARN("New full path %s is not on the same mount point: %s.", fullpath.c_str(), mount_point_.c_str());
  }
}

int File::unlink()
{
  /* Check if the file is on local storage */
  std::map<std::string, sg_size_t>* content = nullptr;
  const char* name = "";
  if (local_storage_) {
    content = local_storage_->extension<FileSystemStorageExt>()->get_content();
    name    = local_storage_->get_cname();
  }
  if (local_disk_) {
    content = local_disk_->extension<FileSystemDiskExt>()->get_content();
    name    = local_disk_->get_cname();
  }

  if (not content || content->find(path_) == content->end()) {
    XBT_WARN("File %s is not on disk %s. Impossible to unlink", path_.c_str(), name);
    return -1;
  } else {
    XBT_DEBUG("UNLINK %s on disk '%s'", path_.c_str(), name);

    if (local_storage_)
      local_storage_->extension<FileSystemStorageExt>()->decr_used_size(size_);

    if (local_disk_)
      local_disk_->extension<FileSystemDiskExt>()->decr_used_size(size_);

    // Remove the file from storage
    content->erase(fullpath_);

    return 0;
  }
}

int File::remote_copy(sg_host_t host, const char* fullpath)
{
  /* Find the host where the file is physically located and read it */
  Host* src_host = nullptr;
  if (local_storage_) {
    src_host = local_storage_->get_host();
    XBT_DEBUG("READ %s on disk '%s'", get_path(), local_storage_->get_cname());
  }

  if (local_disk_) {
    src_host = local_disk_->get_host();
    XBT_DEBUG("READ %s on disk '%s'", get_path(), local_disk_->get_cname());
  }

  seek(0, SEEK_SET);
  // if the current position is close to the end of the file, we may not be able to read the requested size
  sg_size_t read_size = 0;
  if (local_storage_)
    read_size = local_storage_->read(size_);
  if (local_disk_)
    read_size = local_disk_->read(size_);

  current_position_ += read_size;

  Host* dst_host = host;
  if (local_storage_) {
    /* Find the host that owns the storage where the file has to be copied */
    Storage* storage_dest        = nullptr;
    size_t longest_prefix_length = 0;

    for (auto const& elm : host->get_mounted_storages()) {
      std::string mount_point = std::string(fullpath).substr(0, elm.first.size());
      if (mount_point == elm.first && elm.first.length() > longest_prefix_length) {
        /* The current mount name is found in the full path and is bigger than the previous*/
        longest_prefix_length = elm.first.length();
        storage_dest          = elm.second;
      }
    }

    if (storage_dest != nullptr) {
      /* Mount point found, retrieve the host the storage is attached to */
      dst_host = storage_dest->get_host();
    } else {
      XBT_WARN("Can't find mount point for '%s' on destination host '%s'", fullpath, host->get_cname());
      return -1;
    }
  }

  if (local_disk_) {
    size_t longest_prefix_length = 0;
    Disk* dst_disk               = nullptr;

    for (auto const& disk : host->get_disks()) {
      std::string current_mount = disk->extension<FileSystemDiskExt>()->get_mount_point();
      std::string mount_point   = std::string(fullpath).substr(0, current_mount.length());
      if (mount_point == current_mount && current_mount.length() > longest_prefix_length) {
        /* The current mount name is found in the full path and is bigger than the previous*/
        longest_prefix_length = current_mount.length();
        dst_disk              = disk;
      }
    }

    if (dst_disk == nullptr) {
      XBT_WARN("Can't find mount point for '%s' on destination host '%s'", fullpath, host->get_cname());
      return -1;
    }
  }

  if (src_host) {
    XBT_DEBUG("Initiate data transfer of %llu bytes between %s and %s.", read_size, src_host->get_cname(),
              dst_host->get_cname());
    src_host->send_to(dst_host, read_size);
  }

  /* Create file on remote host, write it and close it */
  File* fd = new File(fullpath, dst_host, nullptr);
  if (local_storage_) {
    sg_size_t write_size = fd->local_storage_->write(read_size);
    fd->local_storage_->extension<FileSystemStorageExt>()->incr_used_size(write_size);
    (*(fd->local_storage_->extension<FileSystemStorageExt>()->get_content()))[path_] = size_;
  }
  if (local_disk_)
    fd->write(read_size);
  delete fd;
  return 0;
}

int File::remote_move(sg_host_t host, const char* fullpath)
{
  int res = remote_copy(host, fullpath);
  unlink();
  return res;
}

FileSystemDiskExt::FileSystemDiskExt(simgrid::s4u::Disk* ptr)
{
  const char* size_str    = ptr->get_property("size");
  if (size_str)
    size_ = surf_parse_get_size(size_str, "disk size", ptr->get_name());

  const char* current_mount_str = ptr->get_property("mount");
  if (current_mount_str)
    mount_point_ = std::string(current_mount_str);
  else
    mount_point_ = std::string("/");

  const char* content_str = ptr->get_property("content");
  if (content_str)
    content_.reset(parse_content(content_str));
}

FileSystemStorageExt::FileSystemStorageExt(simgrid::s4u::Storage* ptr)
{
  content_.reset(parse_content(ptr->get_impl()->content_name_));
  size_    = ptr->get_impl()->size_;
}

std::map<std::string, sg_size_t>* FileSystemDiskExt::parse_content(const std::string& filename)
{
  if (filename.empty())
    return nullptr;

  std::map<std::string, sg_size_t>* parse_content = new std::map<std::string, sg_size_t>();

  std::ifstream* fs = surf_ifsopen(filename);

  std::string line;
  std::vector<std::string> tokens;
  do {
    std::getline(*fs, line);
    boost::trim(line);
    if (line.length() > 0) {
      boost::split(tokens, line, boost::is_any_of(" \t"), boost::token_compress_on);
      xbt_assert(tokens.size() == 2, "Parse error in %s: %s", filename.c_str(), line.c_str());
      sg_size_t size = std::stoull(tokens.at(1));

      used_size_ += size;
      parse_content->insert({tokens.front(), size});
    }
  } while (not fs->eof());
  delete fs;
  return parse_content;
}

std::map<std::string, sg_size_t>* FileSystemStorageExt::parse_content(const std::string& filename)
{
  if (filename.empty())
    return nullptr;

  std::map<std::string, sg_size_t>* parse_content = new std::map<std::string, sg_size_t>();

  std::ifstream* fs = surf_ifsopen(filename);

  std::string line;
  std::vector<std::string> tokens;
  do {
    std::getline(*fs, line);
    boost::trim(line);
    if (line.length() > 0) {
      boost::split(tokens, line, boost::is_any_of(" \t"), boost::token_compress_on);
      xbt_assert(tokens.size() == 2, "Parse error in %s: %s", filename.c_str(), line.c_str());
      sg_size_t size = std::stoull(tokens.at(1));

      used_size_ += size;
      parse_content->insert({tokens.front(), size});
    }
  } while (not fs->eof());
  delete fs;
  return parse_content;
}
}
}

using simgrid::s4u::FileDescriptorHostExt;
using simgrid::s4u::FileSystemDiskExt;
using simgrid::s4u::FileSystemStorageExt;

static void on_disk_creation(simgrid::s4u::Disk& d)
{
  d.extension_set(new FileSystemDiskExt(&d));
}
static void on_storage_creation(simgrid::s4u::Storage& st)
{
  st.extension_set(new FileSystemStorageExt(&st));
}

static void on_host_creation(simgrid::s4u::Host& host)
{
  host.extension_set<FileDescriptorHostExt>(new FileDescriptorHostExt());
}

static void on_platform_created()
{
  for (auto const& host : simgrid::s4u::Engine::get_instance()->get_all_hosts()) {
    const char* remote_disk_str = host->get_property("remote_disk");
    if (remote_disk_str) {
      std::vector<std::string> tokens;
      boost::split(tokens, remote_disk_str, boost::is_any_of(":"));
      std::string mount_point         = tokens[0];
      simgrid::s4u::Host* remote_host = simgrid::s4u::Host::by_name_or_null(tokens[2]);
      xbt_assert(remote_host, "You're trying to access a host that does not exist. Please check your platform file");

      simgrid::s4u::Disk* disk = nullptr;
      for (auto const& d : remote_host->get_disks())
        if (d->get_name() == tokens[1]) {
          disk = d;
          break;
        }

      xbt_assert(disk, "You're trying to mount a disk that does not exist. Please check your platform file");
      disk->extension<FileSystemDiskExt>()->add_remote_mount(remote_host, mount_point);
      host->add_disk(disk);

      XBT_DEBUG("Host '%s' wants to mount a remote disk: %s of %s mounted on %s", host->get_cname(), disk->get_cname(),
                remote_host->get_cname(), mount_point.c_str());
      XBT_DEBUG("Host '%s' now has %zu disks", host->get_cname(), host->get_disks().size());
    }
  }
}

static void on_simulation_end()
{
  XBT_DEBUG("Simulation is over, time to unregister remote disks if any");
  for (auto const& host : simgrid::s4u::Engine::get_instance()->get_all_hosts()) {
    const char* remote_disk_str = host->get_property("remote_disk");
    if (remote_disk_str) {
      std::vector<std::string> tokens;
      boost::split(tokens, remote_disk_str, boost::is_any_of(":"));
      XBT_DEBUG("Host '%s' wants to unmount a remote disk: %s of %s mounted on %s", host->get_cname(),
                tokens[1].c_str(), tokens[2].c_str(), tokens[0].c_str());
      host->remove_disk(tokens[1]);
      XBT_DEBUG("Host '%s' now has %zu disks", host->get_cname(), host->get_disks().size());
    }
  }
}

/* **************************** Public interface *************************** */
void sg_storage_file_system_init()
{
  sg_storage_max_file_descriptors = 1024;
  simgrid::config::bind_flag(sg_storage_max_file_descriptors, "storage/max_file_descriptors",
                             "Maximum number of concurrently opened files per host. Default is 1024");

  if (not FileSystemStorageExt::EXTENSION_ID.valid()) {
    FileSystemStorageExt::EXTENSION_ID = simgrid::s4u::Storage::extension_create<FileSystemStorageExt>();
    simgrid::s4u::Storage::on_creation.connect(&on_storage_creation);
  }

  if (not FileSystemDiskExt::EXTENSION_ID.valid()) {
    FileSystemDiskExt::EXTENSION_ID = simgrid::s4u::Disk::extension_create<FileSystemDiskExt>();
    simgrid::s4u::Disk::on_creation.connect(&on_disk_creation);
  }

  if (not FileDescriptorHostExt::EXTENSION_ID.valid()) {
    FileDescriptorHostExt::EXTENSION_ID = simgrid::s4u::Host::extension_create<FileDescriptorHostExt>();
    simgrid::s4u::Host::on_creation.connect(&on_host_creation);
  }
  simgrid::s4u::Engine::on_platform_created.connect(&on_platform_created);
  simgrid::s4u::Engine::on_simulation_end.connect(&on_simulation_end);
}

sg_file_t sg_file_open(const char* fullpath, void* data)
{
  return new simgrid::s4u::File(fullpath, data);
}

sg_size_t sg_file_read(sg_file_t fd, sg_size_t size)
{
  return fd->read(size);
}

sg_size_t sg_file_write(sg_file_t fd, sg_size_t size)
{
  return fd->write(size);
}

void sg_file_close(sg_file_t fd)
{
  delete fd;
}

const char* sg_file_get_name(sg_file_t fd)
{
  xbt_assert((fd != nullptr), "Invalid file descriptor");
  return fd->get_path();
}

sg_size_t sg_file_get_size(sg_file_t fd)
{
  return fd->size();
}

void sg_file_dump(sg_file_t fd)
{
  fd->dump();
}

void* sg_file_get_data(sg_file_t fd)
{
  return fd->get_userdata();
}

void sg_file_set_data(sg_file_t fd, void* data)
{
  fd->set_userdata(data);
}

/**
 * @brief Set the file position indicator in the sg_file_t by adding offset bytes
 * to the position specified by origin (either SEEK_SET, SEEK_CUR, or SEEK_END).
 *
 * @param fd : file object that identifies the stream
 * @param offset : number of bytes to offset from origin
 * @param origin : Position used as reference for the offset. It is specified by one of the following constants defined
 *                 in \<stdio.h\> exclusively to be used as arguments for this function (SEEK_SET = beginning of file,
 *                 SEEK_CUR = current position of the file pointer, SEEK_END = end of file)
 */
void sg_file_seek(sg_file_t fd, sg_offset_t offset, int origin)
{
  fd->seek(offset, origin);
}

sg_size_t sg_file_tell(sg_file_t fd)
{
  return fd->tell();
}

void sg_file_move(sg_file_t fd, const char* fullpath)
{
  fd->move(fullpath);
}

void sg_file_unlink(sg_file_t fd)
{
  fd->unlink();
  delete fd;
}

/**
 * @brief Copy a file to another location on a remote host.
 * @param file : the file to move
 * @param host : the remote host where the file has to be copied
 * @param fullpath : the complete path destination on the remote host
 * @return If successful, the function returns 0. Otherwise, it returns -1.
 */
int sg_file_rcopy(sg_file_t file, sg_host_t host, const char* fullpath)
{
  return file->remote_copy(host, fullpath);
}

/**
 * @brief Move a file to another location on a remote host.
 * @param file : the file to move
 * @param host : the remote host where the file has to be moved
 * @param fullpath : the complete path destination on the remote host
 * @return If successful, the function returns 0. Otherwise, it returns -1.
 */
int sg_file_rmove(sg_file_t file, sg_host_t host, const char* fullpath)
{
  return file->remote_move(host, fullpath);
}

sg_size_t sg_disk_get_size_free(sg_disk_t d)
{
  return d->extension<FileSystemDiskExt>()->get_size() - d->extension<FileSystemDiskExt>()->get_used_size();
}

sg_size_t sg_disk_get_size_used(sg_disk_t d)
{
  return d->extension<FileSystemDiskExt>()->get_used_size();
}

sg_size_t sg_disk_get_size(sg_disk_t d)
{
  return d->extension<FileSystemDiskExt>()->get_size();
}

const char* sg_disk_get_mount_point(sg_disk_t d)
{
  return d->extension<FileSystemDiskExt>()->get_mount_point();
}

sg_size_t sg_storage_get_size_free(sg_storage_t st)
{
  return st->extension<FileSystemStorageExt>()->get_size() - st->extension<FileSystemStorageExt>()->get_used_size();
}

sg_size_t sg_storage_get_size_used(sg_storage_t st)
{
  return st->extension<FileSystemStorageExt>()->get_used_size();
}

sg_size_t sg_storage_get_size(sg_storage_t st)
{
  return st->extension<FileSystemStorageExt>()->get_size();
}

xbt_dict_t sg_storage_get_content(sg_storage_t storage)
{
  std::map<std::string, sg_size_t>* content = storage->extension<simgrid::s4u::FileSystemStorageExt>()->get_content();
  // Note: ::operator delete is ok here (no destructor called) since the dict elements are of POD type sg_size_t.
  xbt_dict_t content_as_dict = xbt_dict_new_homogeneous(::operator delete);

  for (auto const& entry : *content) {
    sg_size_t* psize = new sg_size_t;
    *psize           = entry.second;
    xbt_dict_set(content_as_dict, entry.first.c_str(), psize);
  }
  return content_as_dict;
}

xbt_dict_t sg_host_get_storage_content(sg_host_t host)
{
  xbt_assert((host != nullptr), "Invalid parameters");
  xbt_dict_t contents = xbt_dict_new_homogeneous(nullptr);
  for (auto const& elm : host->get_mounted_storages())
    xbt_dict_set(contents, elm.first.c_str(), sg_storage_get_content(elm.second));

  return contents;
}
