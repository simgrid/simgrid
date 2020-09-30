/* Copyright (c) 2015-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/remote/AppSide.hpp"
#include "src/internal_config.h"
#include <simgrid/modelchecker.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sys/ptrace.h>
#include <sys/socket.h>
#include <sys/types.h>

// We won't need those once the separation MCer/MCed is complete:
#include "src/mc/mc_smx.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_client, mc, "MC client logic");

namespace simgrid {
namespace mc {

std::unique_ptr<AppSide> AppSide::instance_;

AppSide* AppSide::initialize()
{
  if (not std::getenv(MC_ENV_SOCKET_FD)) // We are not in MC mode: don't initialize the MC world
    return nullptr;

  // Do not break if we are called multiple times:
  if (instance_)
    return instance_.get();

  _sg_do_model_check = 1;

  // Fetch socket from MC_ENV_SOCKET_FD:
  const char* fd_env = std::getenv(MC_ENV_SOCKET_FD);
  int fd = xbt_str_parse_int(fd_env, "Variable '" MC_ENV_SOCKET_FD "' should contain a number but contains '%s'");
  XBT_DEBUG("Model-checked application found socket FD %i", fd);

  // Check the socket type/validity:
  int type;
  socklen_t socklen = sizeof(type);
  if (getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &socklen) != 0)
    xbt_die("Could not check socket type");
  xbt_assert(type == SOCK_SEQPACKET, "Unexpected socket type %i", type);
  XBT_DEBUG("Model-checked application found expected socket type");

  instance_.reset(new simgrid::mc::AppSide(fd));

  // Wait for the model-checker:
  errno = 0;
#if defined __linux__
  ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
#elif defined BSD
  ptrace(PT_TRACE_ME, 0, nullptr, 0);
#else
#error "no ptrace equivalent coded for this platform"
#endif
  if (errno != 0 || raise(SIGSTOP) != 0)
    xbt_die("Could not wait for the model-checker (errno = %d: %s)", errno, strerror(errno));

  instance_->handle_messages();
  return instance_.get();
}

void AppSide::handle_deadlock_check(const s_mc_message_t*) const
{
  bool deadlock = false;
  if (not simix_global->process_list.empty()) {
    deadlock = true;
    for (auto const& kv : simix_global->process_list)
      if (simgrid::mc::actor_is_enabled(kv.second)) {
        deadlock = false;
        break;
      }
  }

  // Send result:
  s_mc_message_int_t answer{MC_MESSAGE_DEADLOCK_CHECK_REPLY, deadlock};
  xbt_assert(channel_.send(answer) == 0, "Could not send response");
}
void AppSide::handle_continue(const s_mc_message_t*) const
{
  /* Nothing to do */
}
void AppSide::handle_simcall(const s_mc_message_simcall_handle_t* message) const
{
  smx_actor_t process = SIMIX_process_from_PID(message->pid);
  xbt_assert(process != nullptr, "Invalid pid %lu", message->pid);
  process->simcall_handle(message->value);
  if (channel_.send(MC_MESSAGE_WAITING))
    xbt_die("Could not send MESSAGE_WAITING to model-checker");
}

void AppSide::handle_actor_enabled(const s_mc_message_actor_enabled_t* msg) const
{
  bool res = simgrid::mc::actor_is_enabled(SIMIX_process_from_PID(msg->aid));
  s_mc_message_int_t answer{MC_MESSAGE_ACTOR_ENABLED_REPLY, res};
  channel_.send(answer);
}

#define assert_msg_size(_name_, _type_)                                                                                \
  xbt_assert(received_size == sizeof(_type_), "Unexpected size for " _name_ " (%zd != %zu)", received_size,            \
             sizeof(_type_))

void AppSide::handle_messages() const
{
  while (true) {
    XBT_DEBUG("Waiting messages from model-checker");

    char message_buffer[MC_MESSAGE_LENGTH];
    ssize_t received_size = channel_.receive(&message_buffer, sizeof(message_buffer));

    xbt_assert(received_size >= 0, "Could not receive commands from the model-checker");

    const s_mc_message_t* message = (s_mc_message_t*)message_buffer;
    switch (message->type) {
      case MC_MESSAGE_DEADLOCK_CHECK:
        assert_msg_size("DEADLOCK_CHECK", s_mc_message_t);
        handle_deadlock_check(message);
        break;

      case MC_MESSAGE_CONTINUE:
        assert_msg_size("MESSAGE_CONTINUE", s_mc_message_t);
        handle_continue(message);
        return;

      case MC_MESSAGE_SIMCALL_HANDLE:
        assert_msg_size("SIMCALL_HANDLE", s_mc_message_simcall_handle_t);
        handle_simcall((s_mc_message_simcall_handle_t*)message_buffer);
        break;

      case MC_MESSAGE_ACTOR_ENABLED:
        assert_msg_size("ACTOR_ENABLED", s_mc_message_actor_enabled_t);
        handle_actor_enabled((s_mc_message_actor_enabled_t*)message_buffer);
        break;

      default:
        xbt_die("Received unexpected message %s (%i)", MC_message_type_name(message->type), message->type);
        break;
    }
  }
}

void AppSide::main_loop() const
{
  while (true) {
    simgrid::mc::wait_for_requests();
    xbt_assert(channel_.send(MC_MESSAGE_WAITING) == 0, "Could not send WAITING message to model-checker");
    this->handle_messages();
  }
}

void AppSide::report_assertion_failure() const
{
  if (channel_.send(MC_MESSAGE_ASSERTION_FAILED))
    xbt_die("Could not send assertion to model-checker");
  this->handle_messages();
}

void AppSide::ignore_memory(void* addr, std::size_t size) const
{
  s_mc_message_ignore_memory_t message;
  message.type = MC_MESSAGE_IGNORE_MEMORY;
  message.addr = (std::uintptr_t)addr;
  message.size = size;
  if (channel_.send(message))
    xbt_die("Could not send IGNORE_MEMORY message to model-checker");
}

void AppSide::ignore_heap(void* address, std::size_t size) const
{
  const s_xbt_mheap_t* heap = mmalloc_get_current_heap();

  s_mc_message_ignore_heap_t message;
  message.type    = MC_MESSAGE_IGNORE_HEAP;
  message.address = address;
  message.size    = size;
  message.block   = ((char*)address - (char*)heap->heapbase) / BLOCKSIZE + 1;
  if (heap->heapinfo[message.block].type == 0) {
    message.fragment = -1;
    heap->heapinfo[message.block].busy_block.ignore++;
  } else {
    message.fragment = (ADDR2UINT(address) % BLOCKSIZE) >> heap->heapinfo[message.block].type;
    heap->heapinfo[message.block].busy_frag.ignore[message.fragment]++;
  }

  if (channel_.send(message))
    xbt_die("Could not send ignored region to MCer");
}

void AppSide::unignore_heap(void* address, std::size_t size) const
{
  s_mc_message_ignore_memory_t message;
  message.type = MC_MESSAGE_UNIGNORE_HEAP;
  message.addr = (std::uintptr_t)address;
  message.size = size;
  if (channel_.send(message))
    xbt_die("Could not send IGNORE_HEAP message to model-checker");
}

void AppSide::declare_symbol(const char* name, int* value) const
{
  s_mc_message_register_symbol_t message;
  message.type = MC_MESSAGE_REGISTER_SYMBOL;
  if (strlen(name) + 1 > sizeof(message.name))
    xbt_die("Symbol is too long");
  strncpy(message.name, name, sizeof(message.name));
  message.callback = nullptr;
  message.data     = value;
  if (channel_.send(message))
    xbt_die("Could send REGISTER_SYMBOL message to model-checker");
}

void AppSide::declare_stack(void* stack, size_t size, ucontext_t* context) const
{
  const s_xbt_mheap_t* heap = mmalloc_get_current_heap();

  s_stack_region_t region;
  memset(&region, 0, sizeof(region));
  region.address = stack;
  region.context = context;
  region.size    = size;
  region.block   = ((char*)stack - (char*)heap->heapbase) / BLOCKSIZE + 1;

  s_mc_message_stack_region_t message;
  message.type         = MC_MESSAGE_STACK_REGION;
  message.stack_region = region;
  if (channel_.send(message))
    xbt_die("Could not send STACK_REGION to model-checker");
}
} // namespace mc
} // namespace simgrid
