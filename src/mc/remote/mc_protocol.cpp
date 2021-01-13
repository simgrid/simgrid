/* Copyright (c) 2015-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/remote/mc_protocol.h"
#include <array>

const char* MC_message_type_name(simgrid::mc::MessageType type)
{
  constexpr std::array<const char*, 14> names{{"NONE", "CONTINUE", "IGNORE_HEAP", "UNIGNORE_HEAP", "IGNORE_MEMORY",
                                               "STACK_REGION", "REGISTER_SYMBOL", "DEADLOCK_CHECK",
                                               "DEADLOCK_CHECK_REPLY", "WAITING", "SIMCALL_HANDLE", "ASSERTION_FAILED",
                                               "ACTOR_ENABLED", "ACTOR_ENABLED_REPLY"}};
  return names[static_cast<int>(type)];
}
