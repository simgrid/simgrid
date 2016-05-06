/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/simix/SynchroRaw.hpp"
#include "src/surf/surf_interface.hpp"

void simgrid::simix::Raw::suspend() {
  /* The suspension of raw synchros is delayed to when the process is rescheduled. */
}

void simgrid::simix::Raw::resume() {
  /* I cannot resume raw synchros directly. This is delayed to when the process is rescheduled at
   * the end of the synchro. */
}
