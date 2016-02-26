/* Copyright (c) 2015. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "src/msg/msg_private.h"

#include "simgrid/s4u/actor.hpp"
#include "simgrid/s4u/comm.hpp"
#include "simgrid/s4u/host.hpp"
#include "simgrid/s4u/mailbox.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_file,"S4U files");

#include "simgrid/s4u/file.hpp"
#include "simgrid/s4u/host.hpp"
#include "simgrid/simix.h"

namespace simgrid {
namespace s4u {

File::File(const char*fullpath, void *userdata) {
  // this cannot fail because we get a xbt_die if the mountpoint does not exist
  inferior_ = simcall_file_open(fullpath, Host::current());
  path_ = fullpath;
}

File::~File() {
  simcall_file_close(inferior_, Host::current());
}

sg_size_t File::read(sg_size_t size) {
  return simcall_file_read(inferior_, size, Host::current());
}
sg_size_t File::write(sg_size_t size) {
  return simcall_file_write(inferior_,size, Host::current());
}
sg_size_t File::size() {
  return simcall_file_get_size(inferior_);
}

void File::seek(sg_size_t pos) {
  simcall_file_seek(inferior_,pos,SEEK_SET);
}
sg_size_t File::tell() {
  return simcall_file_tell(inferior_);
}
void File::move(const char*fullpath) {
  simcall_file_move(inferior_,fullpath);
}
void File::unlink() {
  sg_host_t attached = Host::current(); // FIXME: we should check where this file is attached
  simcall_file_unlink(inferior_,attached);
}

}} // namespace simgrid::s4u
