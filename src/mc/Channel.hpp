/* Copyright (c) 2015-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_CHANNEL_HPP
#define SIMGRID_MC_CHANNEL_HPP

#include <unistd.h>

#include <type_traits>

#include "src/mc/mc_protocol.h"

namespace simgrid {
namespace mc {

class Channel {
  int socket_ = -1;
  template<class M>
  static constexpr bool messageType()
  {
    return std::is_class<M>::value && std::is_trivial<M>::value;
  }
public:

  Channel() {}
  Channel(int sock) : socket_(sock) {}
  ~Channel();

  // No copy:
  Channel(Channel const&) = delete;
  Channel& operator=(Channel const&) = delete;

  // Move:
  Channel(Channel&& that) : socket_(that.socket_)
  {
    that.socket_ = -1;
  }
  Channel& operator=(Channel&& that)
  {
    this->socket_ = that.socket_;
    that.socket_ = -1;
    return *this;
  }

  // Send
  int send(const void* message, size_t size) const;
  int send(e_mc_message_type type) const
  {
    s_mc_message message = { type };
    return this->send(&message, sizeof(message));
  }
  template<class M>
  typename std::enable_if< messageType<M>(), int >::type
  send(M const& m) const
  {
    return this->send(&m, sizeof(M));
  }

  // Receive
  ssize_t receive(void* message, size_t size, bool block = true) const;
  template<class M>
  typename std::enable_if< messageType<M>(), ssize_t >::type
  receive(M& m) const
  {
    return this->receive(&m, sizeof(M));
  }

  int getSocket() const
  {
    return socket_;
  }

};

}
}

#endif
