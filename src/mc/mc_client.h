/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_CLIENT_H
#define SIMGRID_MC_CLIENT_H

#include <xbt/misc.h>
#include "mc_protocol.h"

SG_BEGIN_DECL()

typedef struct s_mc_client {
  int active;
  int fd;
} s_mc_client_t, *mc_client_t;

extern XBT_PRIVATE mc_client_t mc_client;

XBT_PRIVATE void MC_client_init(void);
XBT_PRIVATE void MC_client_hello(void);
XBT_PRIVATE void MC_client_handle_messages(void);
XBT_PRIVATE void MC_client_send_message(void* message, size_t size);
XBT_PRIVATE void MC_client_send_simple_message(e_mc_message_type type);

#ifdef HAVE_MC
void MC_ignore(void* addr, size_t size);
#endif

void MC_client_main_loop(void);

SG_END_DECL()

#endif
