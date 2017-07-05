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
}
}
