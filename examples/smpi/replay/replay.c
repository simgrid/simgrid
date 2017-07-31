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

action_fun previous_send;
static void overriding_send(const char* const* args)
{
  (*previous_send)(args); // Just call the overriden symbol. That's a toy example.
}

int main(int argc, char *argv[]) {
  /* Setup things and register default actions */
  smpi_replay_init(&argc, &argv);

  /* Connect your callback function to the "blah" event in the trace files */
  xbt_replay_action_register("blah", action_blah);

  /* The send action is an override, so we have to first save its previous value in a global */
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    previous_send = xbt_replay_action_get("send");
    xbt_replay_action_register("send", overriding_send);
  }
  /* The regular run of the replayer */
  smpi_replay_main(&argc, &argv);
  return 0;
}
