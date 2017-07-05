/* Copyright (c) 2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/FileImpl.hpp"
#include "src/surf/HostImpl.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_file, surf, "Logging specific to the SURF file module");
namespace simgrid {
namespace surf {

int FileImpl::seek(sg_offset_t offset, int origin)
{
  switch (origin) {
    case SEEK_SET:
      current_position_ = offset;
      return 0;
    case SEEK_CUR:
      current_position_ += offset;
      return 0;
    case SEEK_END:
      current_position_ = size_ + offset;
      return 0;
    default:
      return -1;
  }
}

int FileImpl::unlink(sg_host_t host)
{
  simgrid::surf::StorageImpl* st = host->pimpl_->findStorageOnMountList(mount_point_.c_str());
  /* Check if the file is on this storage */
  if (st->content_->find(path_) == st->content_->end()) {
    XBT_WARN("File %s is not on disk %s. Impossible to unlink", cname(), st->cname());
    return -1;
  } else {
    XBT_DEBUG("UNLINK %s on disk '%s'", cname(), st->cname());
    st->usedSize_ -= size_;

    // Remove the file from storage
    st->content_->erase(path_);

    return 0;
  }
}

void FileImpl::move(sg_host_t host, const char* fullpath)
{
  /* Check if the new full path is on the same mount point */
  if (not strncmp(mount_point_.c_str(), fullpath, mount_point_.size())) {
    std::map<std::string, sg_size_t>* content = host->pimpl_->findStorageOnMountList(mount_point_.c_str())->content_;
    if (content->find(path_) != content->end()) { // src file exists
      sg_size_t new_size = content->at(path_);
      content->erase(path_);
      std::string path = std::string(fullpath).substr(mount_point_.size(), strlen(fullpath));
      content->insert({path.c_str(), new_size});
      XBT_DEBUG("Move file from %s to %s, size '%llu'", path_.c_str(), fullpath, new_size);
    } else {
      XBT_WARN("File %s doesn't exist", path_.c_str());
    }
  } else {
    XBT_WARN("New full path %s is not on the same mount point: %s.", fullpath, mount_point_.c_str());
  }
}
}
}
