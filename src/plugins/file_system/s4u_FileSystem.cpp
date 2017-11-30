/* Copyright (c) 2015-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"

#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Storage.hpp"
#include "simgrid/simix.hpp"
#include "src/plugins/file_system/FileSystem.hpp"
#include "src/surf/HostImpl.hpp"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <fstream>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_file, "S4U files");
int sg_storage_max_file_descriptors = 1024;

namespace simgrid {
namespace s4u {
simgrid::xbt::Extension<Storage, FileSystemStorageExt> FileSystemStorageExt::EXTENSION_ID;
simgrid::xbt::Extension<Host, FileDescriptorHostExt> FileDescriptorHostExt::EXTENSION_ID;

File::File(std::string fullpath, void* userdata) : File(fullpath, Host::current(), userdata){};

File::File(std::string fullpath, sg_host_t host, void* userdata) : fullpath_(fullpath), userdata_(userdata)
{
  // this cannot fail because we get a xbt_die if the mountpoint does not exist
  Storage* st                  = nullptr;
  size_t longest_prefix_length = 0;
  XBT_DEBUG("Search for storage name for '%s' on '%s'", fullpath.c_str(), host->getCname());

  for (auto const& mnt : host->getMountedStorages()) {
    XBT_DEBUG("See '%s'", mnt.first.c_str());
    mount_point_ = fullpath.substr(0, mnt.first.length());

    if (mount_point_ == mnt.first && mnt.first.length() > longest_prefix_length) {
      /* The current mount name is found in the full path and is bigger than the previous*/
      longest_prefix_length = mnt.first.length();
      st                    = mnt.second;
    }
  }
  if (longest_prefix_length > 0) { /* Mount point found, split fullpath into mount_name and path+filename*/
    mount_point_ = fullpath.substr(0, longest_prefix_length);
    path_        = fullpath.substr(longest_prefix_length, fullpath.length());
  } else
    xbt_die("Can't find mount point for '%s' on '%s'", fullpath.c_str(), host->getCname());

  localStorage = st;

  // assign a file descriptor id to the newly opened File
  FileDescriptorHostExt* ext = host->extension<simgrid::s4u::FileDescriptorHostExt>();
  if (ext->file_descriptor_table == nullptr) {
    ext->file_descriptor_table = new std::vector<int>(sg_storage_max_file_descriptors);
    std::iota(ext->file_descriptor_table->rbegin(), ext->file_descriptor_table->rend(), 0); // Fill with ..., 1, 0.
  }
  xbt_assert(not ext->file_descriptor_table->empty(), "Too much files are opened! Some have to be closed.");
  desc_id = ext->file_descriptor_table->back();
  ext->file_descriptor_table->pop_back();

  XBT_DEBUG("\tOpen file '%s'", path_.c_str());
  std::map<std::string, sg_size_t>* content = localStorage->extension<FileSystemStorageExt>()->getContent();
  // if file does not exist create an empty file
  auto sz = content->find(path_);
  if (sz != content->end()) {
    size_ = sz->second;
  } else {
    size_ = 0;
    content->insert({path_, size_});
    XBT_DEBUG("File '%s' was not found, file created.", path_.c_str());
  }
}

File::~File()
{
  Host::current()->extension<simgrid::s4u::FileDescriptorHostExt>()->file_descriptor_table->push_back(desc_id);
}

void File::dump()
{
  XBT_INFO("File Descriptor information:\n"
           "\t\tFull path: '%s'\n"
           "\t\tSize: %llu\n"
           "\t\tMount point: '%s'\n"
           "\t\tStorage Id: '%s'\n"
           "\t\tStorage Type: '%s'\n"
           "\t\tFile Descriptor Id: %d",
           getPath(), size_, mount_point_.c_str(), localStorage->getCname(), localStorage->getType(), desc_id);
}

sg_size_t File::read(sg_size_t size)
{
  XBT_DEBUG("READ %s on disk '%s'", getPath(), localStorage->getCname());
  // if the current position is close to the end of the file, we may not be able to read the requested size
  sg_size_t read_size = localStorage->read(std::min(size, size_ - current_position_));
  current_position_ += read_size;
  return read_size;
}

sg_size_t File::write(sg_size_t size)
{
  XBT_DEBUG("WRITE %s on disk '%s'. size '%llu/%llu'", getPath(), localStorage->getCname(), size, size_);
  // If the storage is full before even starting to write
  if (sg_storage_get_size_used(localStorage) >= sg_storage_get_size(localStorage))
    return 0;
  /* Substract the part of the file that might disappear from the used sized on the storage element */
  localStorage->extension<FileSystemStorageExt>()->decrUsedSize(size_ - current_position_);

  sg_size_t write_size = localStorage->write(size);
  localStorage->extension<FileSystemStorageExt>()->incrUsedSize(write_size);

  current_position_ += write_size;
  size_ = current_position_;
  std::map<std::string, sg_size_t>* content = localStorage->extension<FileSystemStorageExt>()->getContent();

  content->erase(path_);
  content->insert({path_, size_});

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

void File::move(std::string fullpath)
{
  /* Check if the new full path is on the same mount point */
  if (not strncmp(mount_point_.c_str(), fullpath.c_str(), mount_point_.length())) {
    std::map<std::string, sg_size_t>* content = localStorage->extension<FileSystemStorageExt>()->getContent();
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
  } else {
    XBT_WARN("New full path %s is not on the same mount point: %s.", fullpath.c_str(), mount_point_.c_str());
  }
}

int File::unlink()
{
  /* Check if the file is on local storage */
  std::map<std::string, sg_size_t>* content = localStorage->extension<FileSystemStorageExt>()->getContent();

  if (content->find(path_) == content->end()) {
    XBT_WARN("File %s is not on disk %s. Impossible to unlink", path_.c_str(), localStorage->getCname());
    return -1;
  } else {
    XBT_DEBUG("UNLINK %s on disk '%s'", path_.c_str(), localStorage->getCname());
    localStorage->extension<FileSystemStorageExt>()->decrUsedSize(size_);

    // Remove the file from storage
    content->erase(fullpath_);

    return 0;
  }
}

FileSystemStorageExt::FileSystemStorageExt(simgrid::s4u::Storage* ptr)
{
  content_ = parseContent(ptr->getImpl()->content_name);
  size_    = ptr->getImpl()->size_;
}

FileSystemStorageExt::~FileSystemStorageExt()
{
  delete content_;
}

std::map<std::string, sg_size_t>* FileSystemStorageExt::parseContent(std::string filename)
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

      usedSize_ += size;
      parse_content->insert({tokens.front(), size});
    }
  } while (not fs->eof());
  delete fs;
  return parse_content;
}
}
}

using simgrid::s4u::FileSystemStorageExt;
using simgrid::s4u::FileDescriptorHostExt;

static void onStorageCreation(simgrid::s4u::Storage& st)
{
  st.extension_set(new FileSystemStorageExt(&st));
}

static void onStorageDestruction(simgrid::s4u::Storage& st)
{
  delete st.extension<FileSystemStorageExt>();
}

static void onHostCreation(simgrid::s4u::Host& host)
{
  host.extension_set<FileDescriptorHostExt>(new FileDescriptorHostExt());
}

/* **************************** Public interface *************************** */
SG_BEGIN_DECL()

void sg_storage_file_system_init()
{
  if (not FileSystemStorageExt::EXTENSION_ID.valid()) {
    FileSystemStorageExt::EXTENSION_ID = simgrid::s4u::Storage::extension_create<FileSystemStorageExt>();
    simgrid::s4u::Storage::onCreation.connect(&onStorageCreation);
    simgrid::s4u::Storage::onDestruction.connect(&onStorageDestruction);
  }

  if (not FileDescriptorHostExt::EXTENSION_ID.valid()) {
    FileDescriptorHostExt::EXTENSION_ID = simgrid::s4u::Host::extension_create<FileDescriptorHostExt>();
    simgrid::s4u::Host::onCreation.connect(&onHostCreation);
  }
}

sg_file_t sg_file_open(const char* fullpath, void* data)
{
  return new simgrid::s4u::File(fullpath, data);
}

void sg_file_close(sg_file_t fd)
{
  delete fd;
}

const char* sg_file_get_name(sg_file_t fd)
{
  xbt_assert((fd != nullptr), "Invalid file descriptor");
  return fd->getPath();
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
  return fd->getUserdata();
}

void sg_file_set_data(sg_file_t fd, void* data)
{
  fd->setUserdata(data);
}

/**
 * \brief Set the file position indicator in the msg_file_t by adding offset bytes
 * to the position specified by origin (either SEEK_SET, SEEK_CUR, or SEEK_END).
 *
 * \param fd : file object that identifies the stream
 * \param offset : number of bytes to offset from origin
 * \param origin : Position used as reference for the offset. It is specified by one of the following constants defined
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

sg_size_t sg_storage_get_size_free(sg_storage_t st)
{
  return st->extension<FileSystemStorageExt>()->getSize() - st->extension<FileSystemStorageExt>()->getUsedSize();
}

sg_size_t sg_storage_get_size_used(sg_storage_t st)
{
  return st->extension<FileSystemStorageExt>()->getUsedSize();
}

sg_size_t sg_storage_get_size(sg_storage_t st)
{
  return st->extension<FileSystemStorageExt>()->getSize();
}

xbt_dict_t sg_storage_get_content(sg_storage_t storage)
{
  std::map<std::string, sg_size_t>* content = storage->extension<simgrid::s4u::FileSystemStorageExt>()->getContent();
  // Note: ::operator delete is ok here (no destructor called) since the dict elements are of POD type sg_size_t.
  xbt_dict_t content_as_dict = xbt_dict_new_homogeneous(::operator delete);

  for (auto const& entry : *content) {
    sg_size_t* psize = new sg_size_t;
    *psize           = entry.second;
    xbt_dict_set(content_as_dict, entry.first.c_str(), psize, nullptr);
  }
  return content_as_dict;
}

xbt_dict_t sg_host_get_storage_content(sg_host_t host)
{
  xbt_assert((host != nullptr), "Invalid parameters");
  xbt_dict_t contents = xbt_dict_new_homogeneous(nullptr);
  for (auto const& elm : host->getMountedStorages())
    xbt_dict_set(contents, elm.first.c_str(), sg_storage_get_content(elm.second), nullptr);

  return contents;
}

SG_END_DECL()
