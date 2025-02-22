/* Copyright (c) 2015-2024. The SimGrid Team.  All rights reserved.         */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/remote/Channel.hpp"
#include "src/mc/remote/mc_protocol.h"
#include "xbt/asserts.h"
#include "xbt/asserts.hpp"
#include "xbt/backtrace.hpp"
#include <utility>
#include <xbt/log.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_channel, mc, "MC interprocess communication");

namespace simgrid::mc {
Channel::Channel(int sock, Channel const& other) : socket_(sock), buffer_size_(other.buffer_size_)
{
  XBT_DEBUG("%d Adopt %zu bytes buffered by father channel.", getpid(), buffer_size_);
  if (buffer_size_ > 0)
    memcpy(buffer_, other.buffer_ + other.buffer_next_, buffer_size_);
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
    XBT_DEBUG("%d sends msg %s (%zu bytes from %p) buffsize: %lu next:%lu", getpid(),
              to_c_str(*static_cast<const MessageType*>(message)), size, message, buffer_size_, buffer_next_);
  } else {
    XBT_DEBUG("%d sends %zu bytes from %p (not a message) buffsize: %lu next:%lu", getpid(), size, message,
              buffer_size_, buffer_next_);
    xbt_assert(size > 0, "Request to send a 0-sized message! Please fix your code.");
  }

  while (::send(this->socket_, message, size, 0) == -1) {
    if (errno != EINTR) {
      XBT_ERROR("Channel::send failure: %s", strerror(errno));
      return errno;
    }
  }
  return 0;
}
void Channel::debug()
{
  XBT_DEBUG("%d debugs the channel; buffer size:%lu; buffer next: %lu", getpid(), buffer_size_, buffer_next_);
  if (XBT_LOG_ISENABLED(mc_channel, xbt_log_priority_debug))
    xbt_backtrace_display_current();
}

std::pair<bool, void*> Channel::receive(size_t size)
{
  XBT_DEBUG("%d wants to receive %lu bytes; buffer size: %lu; buffer next: %lu", getpid(), size, buffer_size_,
            buffer_next_);
  void* answer;
  if (buffer_size_ < size) { // We need more data from the network
    auto [more, got] = peek(size);
    if (not more) {
      XBT_DEBUG("%d has no more data to consume", getpid());
      return std::make_pair(more, nullptr);
    }
    answer = got;
  } else {
    answer = buffer_ + buffer_next_;
  }
  /* Consume the data */
  XBT_DEBUG("%d consumes %s (data size:%lu, previous value of next: %lu, of bufsize: %lu)", getpid(),
            to_c_str(*((const MessageType*)buffer_ + buffer_next_)), size, buffer_next_, buffer_size_);
  buffer_next_ += size;
  buffer_size_ -= size;
  if (buffer_size_ == 0)
    buffer_next_ = 0;

  return std::make_pair(true, answer);
}
void* Channel::expect_message(size_t size, MessageType type, const char* error_msg)
{
  auto [more_data, got] = receive(size);
  xbt_assert(more_data, "%s", error_msg);

  auto* answer = static_cast<s_mc_message_t*>(got);
  xbt_assert(answer->type == type, "Received unexpected message %s (%i); expected %s (%i)", to_c_str(answer->type),
             (int)answer->type, to_c_str(type), (int)type);

  return got;
}

std::pair<bool, void*> Channel::peek(size_t size)
{
  XBT_DEBUG("%d peeks %lu bytes; buffer size: %lu; buffer next: %lu", getpid(), size, buffer_size_, buffer_next_);
  if (MC_MESSAGE_LENGTH - buffer_next_ < size) {
    XBT_DEBUG("Move the data before receiving the rest");
    memmove(buffer_, buffer_ + buffer_next_, buffer_size_);
    buffer_next_ = 0;
  }
  while (buffer_size_ < size) {
    /* Receive as much data as we can (filling MC_MESSAGE_LENGTH bytes in the buffer) to save some recv syscalls */
    int avail = MC_MESSAGE_LENGTH - (buffer_next_ + buffer_size_);
    int got   = recv(this->socket_, buffer_ + buffer_next_ + buffer_size_, avail, 0);
    xbt_assert(got != -1 || errno == EAGAIN, "Channel::receive failure: %s", strerror(errno));
    if (got == 0) {
      XBT_DEBUG("%d: Connection closed :(", getpid());
      return std::make_pair(false, nullptr);
    }
    buffer_size_ += got;
    XBT_DEBUG("%d receives %d bytes from the network (avail was %d). New size: %lu", getpid(), got, avail,
              buffer_size_);
  }
  XBT_DEBUG("%d peeks msg/type %s", getpid(), to_c_str(*((const MessageType*)buffer_ + buffer_next_)));

  return std::make_pair(true, buffer_ + buffer_next_);
}

std::pair<bool, MessageType> Channel::peek_message_type()
{
  auto [more, got] = peek(sizeof(s_mc_message_t));
  if (not more)
    return std::make_pair(false, MessageType::NONE);
  auto type = ((s_mc_message_t*)got)->type;

  return std::make_pair(true, type);
}

void Channel::reinject(const char* data, size_t size)
{
  xbt_assert(size > 0, "Cannot reinject less than one char (size: %lu)", size);
  xbt_assert(size + buffer_size_ < MC_MESSAGE_LENGTH,
             "Reinjecting these data would make the buffer to overflow. Please increase MC_MESSAGE_LENGTH in the code "
             "(or fix your code).");
  XBT_DEBUG("%d Reinject %zu bytes on top of %zu pre-existing bytes", getpid(), size, buffer_size_);
  memcpy(buffer_ + buffer_next_ + buffer_size_, data, size);
  buffer_size_ += size;
}
} // namespace simgrid::mc
