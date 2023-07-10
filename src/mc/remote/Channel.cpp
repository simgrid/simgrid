/* Copyright (c) 2015-2023. The SimGrid Team.  All rights reserved.         */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/remote/Channel.hpp"
#include <xbt/log.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_channel, mc, "MC interprocess communication");

namespace simgrid::mc {
Channel::Channel(int sock, Channel const& other) : socket_(sock), buffer_(other.buffer_)
{
  XBT_DEBUG("Adopt %zu bytes buffered by father channel.", buffer_.size());
}

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

ssize_t Channel::receive(void* message, size_t size, int flags)
{
  size_t bufsize = buffer_.size();
  ssize_t copied = 0;
  auto* whereto  = static_cast<char*>(message);
  size_t todo    = size;
  if (bufsize > 0) {
    XBT_DEBUG("%d %zu bytes (of %zu expected) are already in buffer", getpid(), bufsize, size);
    copied = std::min(size, bufsize);
    std::copy_n(begin(buffer_), copied, whereto);
    buffer_.erase(begin(buffer_), begin(buffer_) + copied);
    todo -= copied;
    whereto += copied;
  }
  ssize_t res = 0;
  if (todo > 0) {
    errno = 0;
    res   = recv(this->socket_, whereto, todo, flags);
    xbt_assert(res != -1 || errno == EAGAIN, "Channel::receive failure: %s", strerror(errno));
    if (res == -1) {
      res = 0;
    }
  }
  XBT_DEBUG("%d Wanted %zu; Got %zd from buffer and %zd from network", getpid(), size, copied, res);
  res += copied;
  if (static_cast<size_t>(res) >= sizeof(int) && is_valid_MessageType(*static_cast<const int*>(message))) {
    XBT_DEBUG("%d Receive %s (requested %zu; received %zd at %p)", getpid(),
              to_c_str(*static_cast<const MessageType*>(message)), size, res, message);
  } else {
    XBT_DEBUG("Receive %zd bytes", res);
  }
  return res;
}

void Channel::reinject(const char* data, size_t size)
{
  xbt_assert(size > 0, "Cannot reinject less than one char (size: %lu)", size);
  XBT_DEBUG("%d Reinject %zu bytes on top of %zu pre-existing bytes", getpid(), size, buffer_.size());
  buffer_.insert(end(buffer_), data, data + size);
}
} // namespace simgrid::mc
