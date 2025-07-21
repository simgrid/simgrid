/* Copyright (c) 2015-2025. The SimGrid Team.  All rights reserved.         */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/remote/Channel.hpp"
#include "src/mc/remote/mc_protocol.h"
#include "xbt/asserts.h"
#include "xbt/asserts.hpp"
#include "xbt/backtrace.hpp"
#include "xbt/log.h"
#include "xbt/sysdep.h"
#include "xbt/thread.hpp"

#if __linux__
#include <alloca.h>
#endif

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_channel, mc, "MC interprocess communication");

// #define CHANNEL_TRACE_MSG_COUNT
#ifdef CHANNEL_TRACE_MSG_COUNT
int channel_count         = 0; // count the amount of open channels, to report only when the last one closes
const int msg_type_count  = 1 + (int)simgrid::mc::MessageType::GO_ONE_WAY;
int sends[msg_type_count] = {0};
int recvs[msg_type_count] = {0};
#endif

namespace simgrid::mc {
Channel::Channel(int sock, Channel const& other) : socket_(sock), buffer_in_size_(other.buffer_in_size_)
{
  XBT_DEBUG("%s Adopt %zu bytes buffered by father channel.", xbt::gettid().c_str(), buffer_in_size_);
  if (buffer_in_size_ > 0)
    memcpy(buffer_in_, other.buffer_in_ + other.buffer_in_next_, buffer_in_size_);
#ifdef CHANNEL_TRACE_MSG_COUNT
  channel_count++;
#endif
}

Channel::~Channel()
{
  if (this->socket_ >= 0)
    close(this->socket_);
#ifdef CHANNEL_TRACE_MSG_COUNT
  channel_count--;
  if (channel_count > 0)
    return;
  XBT_CRITICAL("-----------------------------------");
  int total_sends = 0, total_recvs = 0;
  for (int i = 0; i < msg_type_count; i++) {
    XBT_CRITICAL("%s send:%d recv: %d", to_c_str((MessageType)i), sends[i], recvs[i]);
    total_sends += sends[i];
    total_recvs += recvs[i];
  }
  XBT_CRITICAL("Total send:%d recv: %d", total_sends, total_recvs);
#endif
}
template <> void Channel::pack<std::string>(std::string str)
{
  XBT_DEBUG("Pack string (size: %lu; ctn: '%s')", str.length(), str.c_str());
  pack<unsigned short>((unsigned short)str.length());
  pack(str.data(), str.length() + 1);
}
template <> std::string Channel::unpack<std::string>(std::function<void(void)> cb)
{
  unsigned short len = unpack<unsigned short>(cb);
  auto [more, got]   = receive(len + 1);
  if (not more) {
    if (cb != nullptr)
      cb();
    else
      xbt_die("Remote died and no error handling callback provided");
    return std::string();
  }
  std::string res(xbt_strdup((char*)got));
  XBT_DEBUG("Received std::string %s", res.c_str());
  return res;
}

void Channel::pack(const void* message, size_t size)
{
  memcpy(buffer_out_ + buffer_out_size_, message, size);
  buffer_out_size_ += size;
}
int Channel::send()
{
  xbt_assert(buffer_out_size_ > 0, "No data was packed for later emission");
  send(buffer_out_, buffer_out_size_);
  buffer_out_size_ = 0;
  return 0;
}

/** @brief Send a message; returns 0 on success or errno on failure */
int Channel::send(const void* message, size_t size)
{
  xbt_assert(buffer_out_size_ == 0 || message == buffer_out_,
             "Cannot send directly data while some other data is packed for later emission");

  if (size >= sizeof(int) && is_valid_MessageType(*static_cast<const int*>(message))) {
    XBT_DEBUG("%s sends msg %s (%zu bytes from %p) buffsize: %lu next:%lu", xbt::gettid().c_str(),
              to_c_str(*static_cast<const MessageType*>(message)), size, message, buffer_in_size_, buffer_in_next_);
#ifdef CHANNEL_TRACE_MSG_COUNT
    if (message != buffer_out_)
      sends[(int)*static_cast<const MessageType*>(message)]++;
#endif
  } else {
    XBT_DEBUG("%s sends %zu bytes from %p (not a message) buffsize: %lu next:%lu", xbt::gettid().c_str(), size, message,
              buffer_in_size_, buffer_in_next_);
    xbt_assert(size > 0, "Request to send a 0-sized message! Please fix your code.");
  }

  int todo  = size;
  char* pos = (char*)message;
  while (todo > 0) {
    int done = ::send(this->socket_, pos, todo, 0);
    if (done == -1) {
      if (errno != EINTR) {
        XBT_ERROR("Channel::send failure: %s", strerror(errno));
        return errno;
      }
      continue;
    }
    todo -= done;
    pos += done;
  }

  return 0;
}

std::pair<bool, void*> Channel::receive(size_t size)
{
  XBT_DEBUG("%s wants to receive %lu bytes; buffer size: %lu; buffer next: %lu", xbt::gettid().c_str(), size,
            buffer_in_size_, buffer_in_next_);
  void* answer;
  if (buffer_in_size_ < size) { // We need more data from the network
    auto [more, got] = peek(size);
    if (not more) {
      XBT_DEBUG("%s has no more data to consume", xbt::gettid().c_str());
      return std::make_pair(more, nullptr);
    }
    answer = got;
  } else {
    answer = buffer_in_ + buffer_in_next_;
  }
  /* Consume the data */
  XBT_DEBUG("%s consumes %s (data size:%lu, previous value of next: %lu, of bufsize: %lu)", xbt::gettid().c_str(),
            to_c_str(*((const MessageType*)(buffer_in_ + buffer_in_next_))), size, buffer_in_next_, buffer_in_size_);
  buffer_in_next_ += size;
  buffer_in_size_ -= size;
  if (buffer_in_size_ == 0)
    buffer_in_next_ = 0;

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
  XBT_DEBUG("%s peeks %lu bytes; buffer size: %lu; buffer next: %lu", xbt::gettid().c_str(), size, buffer_in_size_,
            buffer_in_next_);
  if (MC_MESSAGE_LENGTH - buffer_in_next_ < size) {
    XBT_DEBUG("Move the data before receiving the rest");
    memmove(buffer_in_, buffer_in_ + buffer_in_next_, buffer_in_size_);
    buffer_in_next_ = 0;
  }
#ifdef CHANNEL_TRACE_MSG_COUNT
  if (buffer_in_size_ < size && is_valid_MessageType(*(int*)(buffer_in_ + buffer_in_next_)))
    recvs[*(int*)(buffer_in_ + buffer_in_next_)]++;
#endif

  while (buffer_in_size_ < size) {
    /* Receive as much data as we can (filling MC_MESSAGE_LENGTH bytes in the buffer) to save some recv syscalls */
    int avail = MC_MESSAGE_LENGTH - (buffer_in_next_ + buffer_in_size_);
    int got   = recv(this->socket_, buffer_in_ + buffer_in_next_ + buffer_in_size_, avail, 0);
    xbt_enforce(got != -1 || errno == EAGAIN, "[tid:%s] Channel::receive failure: %s", simgrid::xbt::gettid().c_str(),
                strerror(errno));
    if (got == 0) {
      XBT_DEBUG("%s: Connection closed :(", xbt::gettid().c_str());
      return std::make_pair(false, nullptr);
    }
    buffer_in_size_ += got;
    XBT_DEBUG("%s receives %d bytes from the network (avail was %d). New size: %lu", xbt::gettid().c_str(), got, avail,
              buffer_in_size_);
  }
  XBT_DEBUG("%s peeks msg/type %s", xbt::gettid().c_str(),
            to_c_str(*((const MessageType*)(buffer_in_ + buffer_in_next_))));

  return std::make_pair(true, buffer_in_ + buffer_in_next_);
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
  xbt_assert(size + buffer_in_size_ < MC_MESSAGE_LENGTH,
             "Reinjecting these data would make the buffer to overflow. Please increase MC_MESSAGE_LENGTH in the code "
             "(or fix your code).");
  XBT_DEBUG("%s Reinject %zu bytes on top of %zu pre-existing bytes", xbt::gettid().c_str(), size, buffer_in_size_);
  memcpy(buffer_in_ + buffer_in_next_ + buffer_in_size_, data, size);
  buffer_in_size_ += size;
}
} // namespace simgrid::mc
