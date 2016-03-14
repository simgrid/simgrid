/* Copyright (c) 2015. The SimGrid Team.
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

#include "src/mc/mc_protocol.h"
#include "src/mc/Channel.hpp"

namespace simgrid {
namespace mc {

class XBT_PUBLIC() Client {
private:
  bool active_ = false;
  Channel channel_;
  static std::unique_ptr<Client> client_;
public:
  Client();
  Client(int fd) : active_(true), channel_(fd) {}
  void handleMessages();
  Channel const& getChannel() const { return channel_; }
  Channel& getChannel() { return channel_; }
  void mainLoop(void);
  void reportAssertionFailure(const char* description = nullptr);
  void ignoreMemory(void* addr, std::size_t size);
  void ignoreHeap(void* addr, std::size_t size);
  void unignoreHeap(void* addr, std::size_t size);
  void declareSymbol(const char *name, int* value);
#if HAVE_UCONTEXT_H
  void declareStack(void *stack, size_t size, smx_process_t process, ucontext_t* context);
#endif

  // Singleton :/
  // TODO, remove the singleton antipattern.
  static Client* initialize();
  static Client* get()
  {
    return client_.get();
  }
};

}
}

SG_BEGIN_DECL()

#if HAVE_MC
void MC_ignore(void* addr, std::size_t size);
#endif

SG_END_DECL()

#endif
