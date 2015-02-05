/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_CLIENT_H
#define MC_CLIENT_H

#include <xbt/misc.h>

SG_BEGIN_DECL()

typedef struct s_mc_client {
  int active;
  int fd;
} s_mc_client_t, mc_client_t;

extern s_mc_client_t mc_client;

void MC_client_init(void);
void MC_client_hello(void);
void MC_client_handle_messages(void);

SG_END_DECL()

#endif
