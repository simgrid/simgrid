/* spawn - demo of the gras_actor_spawn function                            */

/* Copyright (c) 2007 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SPAWN_COMMON_H
#define SPAWN_COMMON_H

#include "gras.h"

/* register messages which may be sent (common to client and server) */
void spawn_register_messages(void);

/* Function prototypes */
int father(int argc, char *argv[]);
int child(int argc, char *argv[]);

#endif
