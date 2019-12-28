/* Copyright (c) 2015-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_CLIENT_H
#define SIMGRID_MC_CLIENT_H

#include "src/mc/remote/Channel.hpp"

#include <memory>

namespace simgrid {
namespace mc {

/** Model-checked-side of the communication protocol
 *
 *  Send messages to the model-checker and handles message from it.
 */
class XBT_PUBLIC Client {
private:
  Channel channel_;
  static std::unique_ptr<Client> instance_;

public:
  Client();
  explicit Client(int fd) : channel_(fd) {}
  void handle_messages();

private:
  void handle_deadlock_check(const s_mc_message_t* msg);
  void handle_continue(const s_mc_message_t* msg);
  void handle_simcall(const s_mc_message_simcall_handle_t* message);
  void handle_actor_enabled(const s_mc_message_actor_enabled_t* msg);

public:
  Channel const& get_channel() const { return channel_; }
  Channel& get_channel() { return channel_; }
  XBT_ATTRIB_NORETURN void main_loop();
  void report_assertion_failure();
  void ignore_memory(void* addr, std::size_t size);
  void ignore_heap(void* addr, std::size_t size);
  void unignore_heap(void* addr, std::size_t size);
  void declare_symbol(const char* name, int* value);
#if HAVE_UCONTEXT_H
  void declare_stack(void* stack, size_t size, ucontext_t* context);
#endif

  // Singleton :/
  // TODO, remove the singleton antipattern.
  static Client* initialize();
  static Client* get() { return instance_.get(); }
};
}
}

#endif
