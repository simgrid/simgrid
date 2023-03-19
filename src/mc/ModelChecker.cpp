/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/ModelChecker.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/explo/LivenessChecker.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/sosp/RemoteProcessMemory.hpp"
#include "src/mc/transition/TransitionComm.hpp"
#include "xbt/automaton.hpp"
#include "xbt/system_error.hpp"

#include <array>
#include <csignal>
#include <sys/ptrace.h>
#include <sys/wait.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_ModelChecker, mc, "ModelChecker");

::simgrid::mc::ModelChecker* mc_model_checker = nullptr;

namespace simgrid::mc {

ModelChecker::ModelChecker(std::unique_ptr<RemoteProcessMemory> remote_memory)
    : remote_process_memory_(std::move(remote_memory))
{
}

bool ModelChecker::handle_message(const char* buffer, ssize_t size)
{
  s_mc_message_t base_message;
  xbt_assert(size >= (ssize_t)sizeof(base_message), "Broken message");
  memcpy(&base_message, buffer, sizeof(base_message));

  switch(base_message.type) {
    case MessageType::INITIAL_ADDRESSES: {
      s_mc_message_initial_addresses_t message;
      xbt_assert(size == sizeof(message), "Broken message. Got %d bytes instead of %d.", (int)size, (int)sizeof(message));
      memcpy(&message, buffer, sizeof(message));

      get_remote_process_memory().init(message.mmalloc_default_mdp);
      break;
    }

    case MessageType::IGNORE_HEAP: {
      s_mc_message_ignore_heap_t message;
      xbt_assert(size == sizeof(message), "Broken message");
      memcpy(&message, buffer, sizeof(message));

      IgnoredHeapRegion region;
      region.block    = message.block;
      region.fragment = message.fragment;
      region.address  = message.address;
      region.size     = message.size;
      get_remote_process_memory().ignore_heap(region);
      break;
    }

    case MessageType::UNIGNORE_HEAP: {
      s_mc_message_ignore_memory_t message;
      xbt_assert(size == sizeof(message), "Broken message");
      memcpy(&message, buffer, sizeof(message));
      get_remote_process_memory().unignore_heap((void*)(std::uintptr_t)message.addr, message.size);
      break;
    }

    case MessageType::IGNORE_MEMORY: {
      s_mc_message_ignore_memory_t message;
      xbt_assert(size == sizeof(message), "Broken message");
      memcpy(&message, buffer, sizeof(message));
      this->get_remote_process_memory().ignore_region(message.addr, message.size);
      break;
    }

    case MessageType::STACK_REGION: {
      s_mc_message_stack_region_t message;
      xbt_assert(size == sizeof(message), "Broken message");
      memcpy(&message, buffer, sizeof(message));
      this->get_remote_process_memory().stack_areas().push_back(message.stack_region);
    } break;

    case MessageType::REGISTER_SYMBOL: {
      s_mc_message_register_symbol_t message;
      xbt_assert(size == sizeof(message), "Broken message");
      memcpy(&message, buffer, sizeof(message));
      xbt_assert(not message.callback, "Support for client-side function proposition is not implemented.");
      XBT_DEBUG("Received symbol: %s", message.name.data());

      LivenessChecker::automaton_register_symbol(get_remote_process_memory(), message.name.data(),
                                                 remote((int*)message.data));
      break;
    }

    case MessageType::WAITING:
      return false;

    case MessageType::ASSERTION_FAILED:
      Exploration::get_instance()->report_assertion_failure();
      break;

    default:
      xbt_die("Unexpected message from model-checked application");
  }
  return true;
}

} // namespace simgrid::mc
