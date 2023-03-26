/* Copyright (c) 2015-2023. The SimGrid Team.
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

namespace simgrid::mc {

Channel::~Channel()
{
  if (this->socket_ >= 0)
    close(this->socket_);
}

/** @brief Send a message; returns 0 on success or errno on failure */
int Channel::send(const void* message, size_t size) const
{
  if (size >= sizeof(int) && is_valid_MessageType(*static_cast<const int*>(message))) {
    XBT_DEBUG("Sending %s (%zu bytes sent)", to_c_str(*static_cast<const MessageType*>(message)), size);
  } else {
    XBT_DEBUG("Sending bytes directly (from address %p) (%zu bytes sent)", message, size);
    if (size == 0)
      XBT_WARN("Request to send a 0-sized message! Proceeding anyway.");
  }

  while (::send(this->socket_, message, size, 0) == -1) {
    if (errno != EINTR) {
      XBT_ERROR("Channel::send failure: %s", strerror(errno));
      return errno;
    }
  }
  return 0;
}

ssize_t Channel::receive(void* message, size_t size) const
{
  ssize_t res = recv(this->socket_, message, size, 0);
  xbt_assert(res != -1, "Channel::receive failure: %s", strerror(errno));
  if (static_cast<size_t>(res) >= sizeof(int) && is_valid_MessageType(*static_cast<const int*>(message))) {
    XBT_DEBUG("Receive %s (requested %zu; received %zd at %p)", to_c_str(*static_cast<const MessageType*>(message)),
              size, res, message);
  } else {
    XBT_DEBUG("Receive %zd bytes", res);
  }
  return res;
}
} // namespace simgrid::mc
