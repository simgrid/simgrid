/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/simix/Synchro.h"

simgrid::simix::Synchro::Synchro() {
  simcalls = xbt_fifo_new();
}

simgrid::simix::Synchro::~Synchro() {
  xbt_fifo_free(simcalls);
  xbt_free(name);
}
