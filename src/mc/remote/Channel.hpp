/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_CHANNEL_HPP
#define SIMGRID_MC_CHANNEL_HPP

#include "src/mc/remote/mc_protocol.h"
#include "xbt/asserts.h"
#include "xbt/sysdep.h"

#include <cstddef>
#include <string>
#include <type_traits>

namespace simgrid::mc {

/** A channel for exchanging messages between model-checker and model-checked app
 *
 *  This abstracts away the way the messages are transferred. Currently, they
 *  are sent over a (connected) `SOCK_SEQPACKET` socket.
 */
class Channel {
  int socket_ = -1;
  template <class M> static constexpr bool messageType() { return std::is_class_v<M> && std::is_trivial_v<M>; }
  char buffer_in_[MC_MESSAGE_LENGTH];
  size_t buffer_in_next_ = 0;
  size_t buffer_in_size_ = 0;
  char buffer_out_[MC_MESSAGE_LENGTH];
  size_t buffer_out_size_ = 0;

public:
  Channel() = default;
  explicit Channel(int sock) : socket_(sock) {}
  Channel(int sock, Channel const& other);
  ~Channel();

  // No copy:
  Channel(Channel const&) = delete;
  Channel& operator=(Channel const&) = delete;

  // Send
  int send(const void* message, size_t size);
  int send(MessageType type)
  {
    s_mc_message_t message = {type};
    return this->send(&message, sizeof(message));
  }
  /** @brief Send a message; returns 0 on success or errno on failure */
  template <class M> typename std::enable_if_t<messageType<M>(), int> send(M const& m)
  {
    return this->send(&m, sizeof(M));
  }

  // Pack (delayed send)
  template <class T> void pack(T data) { pack(&data, sizeof(T)); }
  void pack(const void* message, size_t size); // queue something for later emission
  int send();                                  // Send everything that was packed

  // Receive
  std::pair<bool, void*> receive(size_t size);
  template <class T> T unpack(std::function<void(void)> cb = nullptr)
  {
    auto [more, got] = receive(sizeof(T));
    if (not more) {
      if (cb != nullptr)
        cb();
      else
        xbt_die("Remote died and no error handling callback provided");
    }
    return *(T*)got;
  }

  void* expect_message(size_t size, MessageType type, const char* error_msg);

  // Peek (receive without consuming)
  std::pair<bool, void*> peek(size_t size);

  // Write the type of the next message in the reference parameter, and return false if no message is to be read (socket
  // closed by peer)
  std::pair<bool, MessageType> peek_message_type();
  void reinject(const char* data, size_t size);
  bool has_pending_data() const { return buffer_in_size_ != 0; }

  // Socket handling
  int get_socket() const { return socket_; }
  void reset_socket(int socket) { socket_ = socket; }
};

template <> void Channel::pack<std::string>(std::string str);
template <> std::string Channel::unpack<std::string>(std::function<void(void)> cb);

} // namespace simgrid::mc

#endif
