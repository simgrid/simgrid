/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <errno.h>
#include <string.h>
#include <stdio.h> // perror
#include <cstddef> // std::size_t

#include <sys/types.h>
#include <sys/socket.h>

#include <xbt/log.h>

#include "src/mc/mc_protocol.h"
#include "src/mc/Client.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_protocol, mc, "Generic MC protocol logic");

const char* MC_message_type_name(e_mc_message_type type)
{
  switch(type) {
  case MC_MESSAGE_NONE:
    return "NONE";
  case MC_MESSAGE_CONTINUE:
    return "CONTINUE";
  case MC_MESSAGE_IGNORE_HEAP:
    return "IGNORE_HEAP";
  case MC_MESSAGE_UNIGNORE_HEAP:
    return "UNIGNORE_HEAP";
  case MC_MESSAGE_IGNORE_MEMORY:
    return "IGNORE_MEMORY";
  case MC_MESSAGE_STACK_REGION:
    return "STACK_REGION";
  case MC_MESSAGE_REGISTER_SYMBOL:
    return "REGISTER_SYMBOL";
  case MC_MESSAGE_DEADLOCK_CHECK:
    return "DEADLOCK_CHECK";
  case MC_MESSAGE_DEADLOCK_CHECK_REPLY:
    return "DEADLOCK_CHECK_REPLY";
  case MC_MESSAGE_WAITING:
    return "WAITING";
  case MC_MESSAGE_SIMCALL_HANDLE:
    return "SIMCALL_HANDLE";
  case MC_MESSAGE_ASSERTION_FAILED:
    return "ASSERTION_FAILED";
  default:
    return "?";
  }
}

const char* MC_mode_name(e_mc_mode_t mode)
{
  switch(mode) {
  case MC_MODE_NONE:
    return "NONE";
  case MC_MODE_CLIENT:
    return "CLIENT";
  case MC_MODE_SERVER:
    return "SERVER";
  default:
    return "?";
  }
}
