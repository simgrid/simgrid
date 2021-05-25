/* Copyright (c) 2015-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/remote/AppSide.hpp"
#include "src/internal_config.h"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/mc_base.hpp"
#include "src/mc/remote/RemoteProcess.hpp"
#if HAVE_SMPI
#include "src/smpi/include/private.hpp"
#endif
#include "xbt/coverage.h"
#include "xbt/str.h"
#include "xbt/xbt_modinter.h" /* mmalloc_preinit to get the default mmalloc arena address */
#include <simgrid/modelchecker.h>

#include <cerrno>
#include <cstdio> // setvbuf
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sys/ptrace.h>
#include <sys/socket.h>
#include <sys/types.h>

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

  setvbuf(stdout, nullptr, _IOLBF, 0);

  // Fetch socket from MC_ENV_SOCKET_FD:
  const char* fd_env = std::getenv(MC_ENV_SOCKET_FD);
  int fd = xbt_str_parse_int(fd_env, "Variable '" MC_ENV_SOCKET_FD "' should contain a number but contains '%s'");
  XBT_DEBUG("Model-checked application found socket FD %i", fd);

  // Check the socket type/validity:
  int type;
  socklen_t socklen = sizeof(type);
  xbt_assert(getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &socklen) == 0, "Could not check socket type");
  xbt_assert(type == SOCK_SEQPACKET, "Unexpected socket type %i", type);
  XBT_DEBUG("Model-checked application found expected socket type");

  instance_ = std::make_unique<simgrid::mc::AppSide>(fd);

  // Wait for the model-checker:
  errno = 0;
#if defined __linux__
  ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
#elif defined BSD
  ptrace(PT_TRACE_ME, 0, nullptr, 0);
#else
#error "no ptrace equivalent coded for this platform"
#endif
  xbt_assert(errno == 0 && raise(SIGSTOP) == 0, "Could not wait for the model-checker (errno = %d: %s)", errno,
             strerror(errno));

  s_mc_message_initial_addresses_t message{MessageType::INITIAL_ADDRESSES, mmalloc_preinit(),
                                           kernel::actor::get_maxpid_addr(), simix::simix_global_get_actors_addr(),
                                           simix::simix_global_get_dead_actors_addr()};
  xbt_assert(instance_->channel_.send(message) == 0, "Could not send the initial message with addresses.");

  instance_->handle_messages();
  return instance_.get();
}

void AppSide::handle_deadlock_check(const s_mc_message_t*) const
{
  const auto& actor_list = kernel::EngineImpl::get_instance()->get_actor_list();
  bool deadlock = false;
  if (not actor_list.empty()) {
    deadlock = true;
    for (auto const& kv : actor_list)
      if (mc::actor_is_enabled(kv.second)) {
        deadlock = false;
        break;
      }
  }

  // Send result:
  s_mc_message_int_t answer{MessageType::DEADLOCK_CHECK_REPLY, deadlock};
  xbt_assert(channel_.send(answer) == 0, "Could not send response");
}
void AppSide::handle_simcall_execute(const s_mc_message_simcall_handle_t* message) const
{
  kernel::actor::ActorImpl* actor = kernel::actor::ActorImpl::by_pid(message->aid_);
  xbt_assert(actor != nullptr, "Invalid pid %ld", message->aid_);
  if (actor->simcall_.observer_ != nullptr)
    actor->observer_stack_.push_back(actor->simcall_.observer_->clone());
  actor->simcall_handle(message->times_considered_);
  xbt_assert(channel_.send(MessageType::WAITING) == 0, "Could not send MESSAGE_WAITING to model-checker");
}

void AppSide::handle_actor_enabled(const s_mc_message_actor_enabled_t* msg) const
{
  bool res = mc::actor_is_enabled(kernel::actor::ActorImpl::by_pid(msg->aid));
  s_mc_message_int_t answer{MessageType::ACTOR_ENABLED_REPLY, res};
  channel_.send(answer);
}

#define assert_msg_size(_name_, _type_)                                                                                \
  xbt_assert(received_size == sizeof(_type_), "Unexpected size for " _name_ " (%zd != %zu)", received_size,            \
             sizeof(_type_))

void AppSide::handle_messages() const
{
  while (true) { // Until we get a CONTINUE message
    XBT_DEBUG("Waiting messages from model-checker");

    std::array<char, MC_MESSAGE_LENGTH> message_buffer;
    ssize_t received_size = channel_.receive(message_buffer.data(), message_buffer.size());

    xbt_assert(received_size >= 0, "Could not receive commands from the model-checker");

    const s_mc_message_t* message = (s_mc_message_t*)message_buffer.data();
    switch (message->type) {
      case MessageType::DEADLOCK_CHECK:
        assert_msg_size("DEADLOCK_CHECK", s_mc_message_t);
        handle_deadlock_check(message);
        break;

      case MessageType::CONTINUE:
        assert_msg_size("MESSAGE_CONTINUE", s_mc_message_t);
        return;

      case MessageType::SIMCALL_HANDLE:
        assert_msg_size("SIMCALL_HANDLE", s_mc_message_simcall_handle_t);
        handle_simcall_execute((s_mc_message_simcall_handle_t*)message_buffer.data());
        break;

      case MessageType::SIMCALL_IS_VISIBLE: {
        assert_msg_size("SIMCALL_IS_VISIBLE", s_mc_message_simcall_is_visible_t);
        auto msg_simcall                = (s_mc_message_simcall_is_visible_t*)message_buffer.data();
        const kernel::actor::ActorImpl* actor = kernel::actor::ActorImpl::by_pid(msg_simcall->aid);
        xbt_assert(actor != nullptr, "Invalid pid %ld", msg_simcall->aid);
        xbt_assert(actor->simcall_.observer_, "The transition of %s has no observer", actor->get_cname());
        bool value = actor->simcall_.observer_->is_visible();

        // Send result:
        s_mc_message_simcall_is_visible_answer_t answer{MessageType::SIMCALL_IS_VISIBLE_ANSWER, value};
        xbt_assert(channel_.send(answer) == 0, "Could not send response");
        break;
      }

      case MessageType::SIMCALL_TO_STRING: {
        assert_msg_size("SIMCALL_TO_STRING", s_mc_message_simcall_to_string_t);
        auto msg_simcall                = (s_mc_message_simcall_to_string_t*)message_buffer.data();
        const kernel::actor::ActorImpl* actor = kernel::actor::ActorImpl::by_pid(msg_simcall->aid);
        xbt_assert(actor != nullptr, "Invalid pid %ld", msg_simcall->aid);
        xbt_assert(actor->simcall_.observer_, "The transition of %s has no observer", actor->get_cname());
        std::string value = actor->simcall_.observer_->to_string(msg_simcall->time_considered);

        // Send result:
        s_mc_message_simcall_to_string_answer_t answer{MessageType::SIMCALL_TO_STRING_ANSWER, {0}};
        value.copy(answer.value, (sizeof answer.value) - 1); // last byte was set to '\0' by initialization above
        xbt_assert(channel_.send(answer) == 0, "Could not send response");
        break;
      }

      case MessageType::SIMCALL_DOT_LABEL: {
        assert_msg_size("SIMCALL_DOT_LABEL", s_mc_message_simcall_to_string_t);
        auto msg_simcall                = (s_mc_message_simcall_to_string_t*)message_buffer.data();
        const kernel::actor::ActorImpl* actor = kernel::actor::ActorImpl::by_pid(msg_simcall->aid);
        xbt_assert(actor != nullptr, "Invalid pid %ld", msg_simcall->aid);
        xbt_assert(actor->simcall_.observer_, "The transition of %s has no observer", actor->get_cname());
        std::string value = actor->simcall_.observer_->dot_label();

        // Send result:
        s_mc_message_simcall_to_string_answer_t answer{MessageType::SIMCALL_TO_STRING_ANSWER, {0}};
        value.copy(answer.value, (sizeof answer.value) - 1); // last byte was set to '\0' by initialization above
        xbt_assert(channel_.send(answer) == 0, "Could not send response");
        break;
      }

      case MessageType::ACTOR_ENABLED:
        assert_msg_size("ACTOR_ENABLED", s_mc_message_actor_enabled_t);
        handle_actor_enabled((s_mc_message_actor_enabled_t*)message_buffer.data());
        break;

      case MessageType::FINALIZE: {
        assert_msg_size("FINALIZE", s_mc_message_int_t);
        bool terminate_asap = ((s_mc_message_int_t*)message_buffer.data())->value;
        XBT_DEBUG("Finalize (terminate = %d)", (int)terminate_asap);
        if (not terminate_asap) {
          if (XBT_LOG_ISENABLED(mc_client, xbt_log_priority_debug))
            kernel::EngineImpl::get_instance()->display_all_actor_status();
#if HAVE_SMPI
          XBT_DEBUG("Smpi_enabled: %d", (int)smpi_enabled());
          if (smpi_enabled())
            SMPI_finalize();
#endif
        }
        coverage_checkpoint();
        xbt_assert(channel_.send(MessageType::DEADLOCK_CHECK_REPLY) == 0, // DEADLOCK_CHECK_REPLY, really?
                   "Could not answer to FINALIZE");
        std::fflush(stdout);
        if (terminate_asap)
          ::_Exit(0);
        break;
      }

      default:
        xbt_die("Received unexpected message %s (%i)", to_c_str(message->type), static_cast<int>(message->type));
        break;
    }
  }
}

void AppSide::main_loop() const
{
  coverage_checkpoint();
  while (true) {
    simgrid::mc::execute_actors();
    xbt_assert(channel_.send(MessageType::WAITING) == 0, "Could not send WAITING message to model-checker");
    this->handle_messages();
  }
}

void AppSide::report_assertion_failure() const
{
  xbt_assert(channel_.send(MessageType::ASSERTION_FAILED) == 0, "Could not send assertion to model-checker");
  this->handle_messages();
}

void AppSide::ignore_memory(void* addr, std::size_t size) const
{
  s_mc_message_ignore_memory_t message;
  message.type = MessageType::IGNORE_MEMORY;
  message.addr = (std::uintptr_t)addr;
  message.size = size;
  xbt_assert(channel_.send(message) == 0, "Could not send IGNORE_MEMORY message to model-checker");
}

void AppSide::ignore_heap(void* address, std::size_t size) const
{
  const s_xbt_mheap_t* heap = mmalloc_get_current_heap();

  s_mc_message_ignore_heap_t message;
  message.type    = MessageType::IGNORE_HEAP;
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

  xbt_assert(channel_.send(message) == 0, "Could not send ignored region to MCer");
}

void AppSide::unignore_heap(void* address, std::size_t size) const
{
  s_mc_message_ignore_memory_t message;
  message.type = MessageType::UNIGNORE_HEAP;
  message.addr = (std::uintptr_t)address;
  message.size = size;
  xbt_assert(channel_.send(message) == 0, "Could not send IGNORE_HEAP message to model-checker");
}

void AppSide::declare_symbol(const char* name, int* value) const
{
  s_mc_message_register_symbol_t message;
  message.type = MessageType::REGISTER_SYMBOL;
  xbt_assert(strlen(name) + 1 <= message.name.size(), "Symbol is too long");
  strncpy(message.name.data(), name, message.name.size());
  message.callback = nullptr;
  message.data     = value;
  xbt_assert(channel_.send(message) == 0, "Could send REGISTER_SYMBOL message to model-checker");
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
  message.type         = MessageType::STACK_REGION;
  message.stack_region = region;
  xbt_assert(channel_.send(message) == 0, "Could not send STACK_REGION to model-checker");
}
} // namespace mc
} // namespace simgrid
