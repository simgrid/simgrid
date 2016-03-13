/* Copyright (c) 2015-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <xbt/log.h>

#include "src/mc/Channel.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_Channel, mc, "MC interprocess communication");

namespace simgrid {
namespace mc {

Channel::~Channel()
{
  if (this->socket_ >=0)
    close(this->socket_);
}

int Channel::send(const void* message, size_t size) const
{
  XBT_DEBUG("Protocol [%s] send %s",
    MC_mode_name(mc_mode),
    MC_message_type_name(*(e_mc_message_type*) message));

  while (::send(this->socket_, message, size, 0) == -1)
    if (errno == EINTR)
      continue;
    else
      return errno;
  return 0;
}

ssize_t Channel::receive(void* message, size_t size, bool block) const
{
  int res = recv(this->socket_, message, size, block ? 0 : MSG_DONTWAIT);
  if (res != -1)
    XBT_DEBUG("Protocol [%s] received %s",
      MC_mode_name(mc_mode),
      MC_message_type_name(*(e_mc_message_type*) message));
  return res;
}

}
}
