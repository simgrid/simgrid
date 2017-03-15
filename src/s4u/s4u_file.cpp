/* Copyright (c) 2015. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "src/msg/msg_private.h"

#include "simgrid/s4u/Actor.hpp"
#include "simgrid/s4u/comm.hpp"
#include "simgrid/s4u/host.hpp"
#include "simgrid/s4u/Mailbox.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_file,"S4U files");

#include "simgrid/s4u/file.hpp"
#include "simgrid/s4u/host.hpp"
#include "simgrid/simix.h"

namespace simgrid {
namespace s4u {

File::File(const char* fullpath, void* userdata) : path_(fullpath)
{
  // this cannot fail because we get a xbt_die if the mountpoint does not exist
  pimpl_ = simcall_file_open(fullpath, Host::current());
}

File::~File() {
  simcall_file_close(pimpl_, Host::current());
}

sg_size_t File::read(sg_size_t size) {
  return simcall_file_read(pimpl_, size, Host::current());
}
sg_size_t File::write(sg_size_t size) {
  return simcall_file_write(pimpl_,size, Host::current());
}
sg_size_t File::size() {
  return simcall_file_get_size(pimpl_);
}

void File::seek(sg_size_t pos) {
  simcall_file_seek(pimpl_,pos,SEEK_SET);
}
sg_size_t File::tell() {
  return simcall_file_tell(pimpl_);
}
void File::move(const char*fullpath) {
  simcall_file_move(pimpl_,fullpath);
}
void File::unlink() {
  sg_host_t attached = Host::current(); // FIXME: we should check where this file is attached
  simcall_file_unlink(pimpl_,attached);
}

}} // namespace simgrid::s4u
