/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/log.h>
#include <xbt/fifo.h>
#include <xbt/sysdep.h>
#include <simgrid/modelchecker.h>

#include "src/mc/mc_record.h"
#include "src/mc/mc_private.h"
#include "src/mc/mc_ignore.h"
#include "src/mc/mc_protocol.h"
#include "src/mc/mc_client.h"
#include "src/mc/ModelChecker.hpp"

/** \file mc_client_api.cpp
 *
 *  This is the implementation of the API used by the user simulated program to
 *  communicate with the MC (declared in modelchecker.h).
 */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_client_api, mc,
  "Public API for the model-checked application");

// MC_random() is in mc_base.cpp

void MC_assert(int prop)
{
  if (MC_is_active() && !prop) {
    MC_client_send_simple_message(MC_MESSAGE_ASSERTION_FAILED);
    MC_client_handle_messages();
  }
}

void MC_cut(void)
{
  user_max_depth_reached = 1;
}

void MC_ignore(void* addr, size_t size)
{
  xbt_assert(mc_mode != MC_MODE_SERVER);
  if (mc_mode != MC_MODE_CLIENT)
    return;

  s_mc_ignore_memory_message_t message;
  message.type = MC_MESSAGE_IGNORE_MEMORY;
  message.addr = (std::uintptr_t) addr;
  message.size = size;
  MC_client_send_message(&message, sizeof(message));
}

void MC_automaton_new_propositional_symbol(const char *id, int(*fct)(void))
{
  xbt_assert(mc_mode != MC_MODE_SERVER);
  if (mc_mode != MC_MODE_CLIENT)
    return;

  xbt_die("Support for client-side function proposition is not implemented: "
    "use MC_automaton_new_propositional_symbol_pointer instead."
  );
}

void MC_automaton_new_propositional_symbol_pointer(const char *name, int* value)
{
  xbt_assert(mc_mode != MC_MODE_SERVER);
  if (mc_mode != MC_MODE_CLIENT)
    return;

  s_mc_register_symbol_message_t message;
  message.type = MC_MESSAGE_REGISTER_SYMBOL;
  if (strlen(name) + 1 > sizeof(message.name))
    xbt_die("Symbol is too long");
  strncpy(message.name, name, sizeof(message.name));
  message.callback = nullptr;
  message.data = value;
  MC_client_send_message(&message, sizeof(message));
}
