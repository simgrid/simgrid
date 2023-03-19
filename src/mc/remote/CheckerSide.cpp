/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/remote/CheckerSide.hpp"
#include "src/mc/ModelChecker.hpp"
#include "src/mc/sosp/RemoteProcessMemory.hpp"
#include "xbt/system_error.hpp"
#include <csignal>
#include <sys/wait.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_checkerside, mc, "MC communication with the application");

namespace simgrid::mc {
CheckerSide::CheckerSide(int sockfd, ModelChecker* mc) : channel_(sockfd)
{
  auto* base = event_base_new();
  base_.reset(base);

  auto* socket_event = event_new(
      base, get_channel().get_socket(), EV_READ | EV_PERSIST,
      [](evutil_socket_t sig, short events, void* arg) {
        auto checker = static_cast<simgrid::mc::CheckerSide*>(arg);
        if (events == EV_READ) {
          std::array<char, MC_MESSAGE_LENGTH> buffer;
          ssize_t size = recv(checker->get_channel().get_socket(), buffer.data(), buffer.size(), MSG_DONTWAIT);
          if (size == -1) {
            XBT_ERROR("Channel::receive failure: %s", strerror(errno));
            if (errno != EAGAIN)
              throw simgrid::xbt::errno_error();
          }

          if (not mc_model_checker->handle_message(buffer.data(), size))
            checker->break_loop();
        } else {
          xbt_die("Unexpected event");
        }
      },
      this);
  event_add(socket_event, nullptr);
  socket_event_.reset(socket_event);

  auto* signal_event = event_new(
      base, SIGCHLD, EV_SIGNAL | EV_PERSIST,
      [](evutil_socket_t sig, short events, void* arg) {
        auto mc = static_cast<simgrid::mc::ModelChecker*>(arg);
        if (events == EV_SIGNAL) {
          if (sig == SIGCHLD)
            mc->handle_waitpid(mc->get_remote_process_memory().pid());
          else
            xbt_die("Unexpected signal: %d", sig);
        } else {
          xbt_die("Unexpected event");
        }
      },
      mc);
  event_add(signal_event, nullptr);
  signal_event_.reset(signal_event);
}

void CheckerSide::dispatch_events() const
{
  event_base_dispatch(base_.get());
}

void CheckerSide::break_loop() const
{
  event_base_loopbreak(base_.get());
}

} // namespace simgrid::mc
