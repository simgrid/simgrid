/* Copyright (c) 2015-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/remote/Channel.hpp"
#include <xbt/log.h>

#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_Channel, mc, "MC interprocess communication");

namespace simgrid {
namespace mc {

Channel::~Channel()
{
  if (this->socket_ >= 0)
    close(this->socket_);
}

/** @brief Send a message; returns 0 on success or errno on failure */
int Channel::send(const void* message, size_t size) const
{
  XBT_DEBUG("Send %s", to_c_str(*(MessageType*)message));
  while (::send(this->socket_, message, size, 0) == -1) {
    if (errno != EINTR) {
      XBT_ERROR("Channel::send failure: %s", strerror(errno));
      return errno;
    }
  }
  return 0;
}

ssize_t Channel::receive(void* message, size_t size, bool block) const
{
  ssize_t res = recv(this->socket_, message, size, block ? 0 : MSG_DONTWAIT);
  if (res != -1)
    XBT_DEBUG("Receive %s", to_c_str(*(MessageType*)message));
  else
    XBT_ERROR("Channel::receive failure: %s", strerror(errno));
  return res;
}
}
}
