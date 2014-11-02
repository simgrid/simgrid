/* MC interface: definitions that non-MC modules must see, but not the user */

/* Copyright (c) 2007-2014. The SimGrid Team.  All rights reserved.         */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_INTERFACE_H
#define MC_INTERFACE_H
#include "mc/mc.h"

SG_BEGIN_DECL()

typedef struct s_mc_snapshot *mc_snapshot_t;

SG_END_DECL()

#endif
