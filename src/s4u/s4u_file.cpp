/* Copyright (c) 2015-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"

#include "simgrid/s4u/File.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Storage.hpp"
#include "simgrid/simix.hpp"
#include "src/surf/FileImpl.hpp"
#include "src/surf/HostImpl.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_file,"S4U files");

namespace simgrid {
namespace s4u {

File::File(std::string fullpath, void* userdata) : File(fullpath, Host::current(), userdata){};

File::File(std::string fullpath, sg_host_t host, void* userdata) : path_(fullpath), userdata_(userdata)
{
  // this cannot fail because we get a xbt_die if the mountpoint does not exist
  Storage* st                  = nullptr;
  size_t longest_prefix_length = 0;
  std::string path;
  XBT_DEBUG("Search for storage name for '%s' on '%s'", fullpath.c_str(), host->getCname());

  for (auto const& mnt : host->getMountedStorages()) {
    XBT_DEBUG("See '%s'", mnt.first.c_str());
    mount_point = fullpath.substr(0, mnt.first.length());

    if (mount_point == mnt.first && mnt.first.length() > longest_prefix_length) {
      /* The current mount name is found in the full path and is bigger than the previous*/
      longest_prefix_length = mnt.first.length();
      st                    = mnt.second;
    }
  }
  if (longest_prefix_length > 0) { /* Mount point found, split fullpath into mount_name and path+filename*/
    mount_point = fullpath.substr(0, longest_prefix_length);
    path        = fullpath.substr(longest_prefix_length, fullpath.length());
  } else
    xbt_die("Can't find mount point for '%s' on '%s'", fullpath.c_str(), host->getCname());

  pimpl_ =
      simgrid::simix::kernelImmediate([this, st, path] { return new simgrid::surf::FileImpl(st, path, mount_point); });
  localStorage = st;
}

File::~File()
{
  simgrid::simix::kernelImmediate([this] { delete pimpl_; });
}

sg_size_t File::read(sg_size_t size)
{
  XBT_DEBUG("READ %s on disk '%s'", getPath(), localStorage->getCname());
  // if the current position is close to the end of the file, we may not be able to read the requested size
  sg_size_t read_size = localStorage->read(std::min(size, this->size() - this->tell()));
  pimpl_->incrPosition(read_size);
  return read_size;
}

sg_size_t File::write(sg_size_t size)
{
  XBT_DEBUG("WRITE %s on disk '%s'. size '%llu/%llu'", getPath(), localStorage->getCname(), size, this->size());
  // If the storage is full before even starting to write
  if (localStorage->getSizeUsed() >= localStorage->getSize())
    return 0;
  /* Substract the part of the file that might disappear from the used sized on the storage element */
  localStorage->decrUsedSize(this->size() - this->tell());

  sg_size_t write_size = localStorage->write(size);
  pimpl_->incrPosition(write_size);
  pimpl_->setSize(this->tell());

  localStorage->getContent()->erase(pimpl_->getName());
  localStorage->getContent()->insert({pimpl_->getName(), this->size()});

  return write_size;
}

sg_size_t File::size()
{
  return simgrid::simix::kernelImmediate([this] { return pimpl_->size(); });
}

void File::seek(sg_offset_t pos)
{
  simgrid::simix::kernelImmediate([this, pos] { pimpl_->seek(pos, SEEK_SET); });
}

void File::seek(sg_offset_t pos, int origin)
{
  simgrid::simix::kernelImmediate([this, pos, origin] { pimpl_->seek(pos, origin); });
}

sg_size_t File::tell()
{
  return simgrid::simix::kernelImmediate([this] { return pimpl_->tell(); });
}

void File::move(std::string fullpath)
{
  simgrid::simix::kernelImmediate([this, fullpath] { pimpl_->move(fullpath); });
}

int File::unlink()
{
  return simgrid::simix::kernelImmediate([this] { return pimpl_->unlink(); });
}

}} // namespace simgrid::s4u
