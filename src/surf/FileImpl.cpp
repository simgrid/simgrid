/* Copyright (c) 2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/FileImpl.hpp"
#include "src/surf/StorageImpl.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_file, surf, "Logging specific to the SURF file module");
namespace simgrid {
namespace surf {

FileImpl::FileImpl(sg_storage_t st, std::string path, std::string mount) : path_(path), mount_point_(mount)
{
  XBT_DEBUG("\tOpen file '%s'", path.c_str());
  location_ = st->getImpl();
  std::map<std::string, sg_size_t>* content = location_->getContent();
  // if file does not exist create an empty file
  auto sz = content->find(path);
  if (sz != content->end()) {
    size_ = sz->second;
  } else {
    size_ = 0;
    content->insert({path, size_});
    XBT_DEBUG("File '%s' was not found, file created.", path.c_str());
  }
}

Action* FileImpl::read(sg_size_t size)
{
  XBT_DEBUG("READ %s on disk '%s'", cname(), location_->cname());
  if (current_position_ + size > size_) {
    if (current_position_ > size_) {
      size = 0;
    } else {
      size = size_ - current_position_;
    }
    current_position_ = size_;
  } else
    current_position_ += size;

  return location_->read(size);
}

Action* FileImpl::write(sg_size_t size)
{
  XBT_DEBUG("WRITE %s on disk '%s'. size '%llu/%llu'", cname(), location_->cname(), size, size_);

  StorageAction* action = location_->write(size);
  action->file_         = this;
  /* Substract the part of the file that might disappear from the used sized on the storage element */
  location_->usedSize_ -= (size_ - current_position_);
  // If the storage is full before even starting to write
  if (location_->usedSize_ >= location_->getSize()) {
    action->setState(Action::State::failed);
  }
  return action;
}

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

int FileImpl::unlink()
{
  /* Check if the file is on this storage */
  if (location_->getContent()->find(path_) == location_->getContent()->end()) {
    XBT_WARN("File %s is not on disk %s. Impossible to unlink", cname(), location_->cname());
    return -1;
  } else {
    XBT_DEBUG("UNLINK %s on disk '%s'", cname(), location_->cname());
    location_->usedSize_ -= size_;

    // Remove the file from storage
    location_->getContent()->erase(path_);

    return 0;
  }
}

void FileImpl::move(const char* fullpath)
{
  /* Check if the new full path is on the same mount point */
  if (not strncmp(mount_point_.c_str(), fullpath, mount_point_.size())) {
    std::map<std::string, sg_size_t>* content = location_->getContent();
    auto sz = content->find(path_);
    if (sz != content->end()) { // src file exists
      sg_size_t new_size = sz->second;
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
