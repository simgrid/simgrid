/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/replay.hpp"
#include "smpi/smpi.h"

/* This shows how to extend the trace format by adding a new kind of events.
   This function is registered through xbt_replay_action_register() below. */
static void action_blah(const char* const* args)
{
  /* Add your answer to the blah event here.
     args is a strings array containing the blank-separated parameters found in the trace for this event instance. */
}

int main(int argc, char *argv[]) {
  /* Connect your callback function to the "blah" event in the trace files */
  xbt_replay_action_register("blah", action_blah);

  /* The regular run of the replayer */
  smpi_replay_run(&argc, &argv);
  return 0;
}
