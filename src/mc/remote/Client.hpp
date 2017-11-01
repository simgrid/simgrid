/* Copyright (c) 2015-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_CLIENT_H
#define SIMGRID_MC_CLIENT_H

#include "src/internal_config.h"

#include <cstddef>
#include <memory>

#include <xbt/base.h>

#include <simgrid/simix.h>

#include "src/mc/remote/Channel.hpp"
#include "src/mc/remote/mc_protocol.h"

namespace simgrid {
namespace mc {

/** Model-checked-side of the communication protocol
 *
 *  Send messages to the model-checker and handles message from it.
 */
class XBT_PUBLIC() Client {
private:
  Channel channel_;
  static std::unique_ptr<Client> instance_;

public:
  Client();
  explicit Client(int fd) : channel_(fd) {}
  void handleMessages();

private:
  void handleDeadlockCheck(s_mc_message_t* msg);
  void handleContinue(s_mc_message_t* msg);
  void handleSimcall(s_mc_message_simcall_handle_t* message);
  void handleRestore(s_mc_message_restore_t* msg);
  void handleActorEnabled(s_mc_message_actor_enabled_t* msg);

public:
  Channel const& getChannel() const { return channel_; }
  Channel& getChannel() { return channel_; }
  void mainLoop();
  void reportAssertionFailure(const char* description = nullptr);
  void ignoreMemory(void* addr, std::size_t size);
  void ignoreHeap(void* addr, std::size_t size);
  void unignoreHeap(void* addr, std::size_t size);
  void declareSymbol(const char* name, int* value);
#if HAVE_UCONTEXT_H
  void declareStack(void* stack, size_t size, smx_actor_t process, ucontext_t* context);
#endif

  // Singleton :/
  // TODO, remove the singleton antipattern.
  static Client* initialize();
  static Client* get() { return instance_.get(); }
};
}
}

#endif
