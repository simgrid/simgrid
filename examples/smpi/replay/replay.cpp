/* Copyright (c) 2009-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/replay.hpp"
#include "simgrid/s4u/Actor.hpp"
#include "smpi/smpi.h"
#include "xbt/asserts.h"
#include "xbt/str.h"

#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(replay_test, "Messages specific for this msg example");

/* This shows how to extend the trace format by adding a new kind of events.
   This function is registered through xbt_replay_action_register() below. */
static void action_blah(const simgrid::xbt::ReplayAction& /*args*/)
{
  /* Add your answer to the blah event here.
     args is a strings array containing the blank-separated parameters found in the trace for this event instance. */
}

action_fun previous_send;
static void overriding_send(simgrid::xbt::ReplayAction& args)
{
  previous_send(args); // Just call the overridden symbol. That's a toy example.
}

int main(int argc, char* argv[])
{
  const char* instance_id = simgrid::s4u::Actor::self()->get_property("instance_id");
  const int rank =
      static_cast<int>(xbt_str_parse_int(simgrid::s4u::Actor::self()->get_property("rank"), "Cannot parse rank"));
  const char* trace_filename = argv[1];
  double start_delay_flops   = 0;

  if (argc > 2) {
    start_delay_flops = xbt_str_parse_double(argv[2], "Cannot parse start_delay_flops");
  }

  /* Setup things and register default actions */
  smpi_replay_init(instance_id, rank, start_delay_flops);

  /* Connect your callback function to the "blah" event in the trace files */
  xbt_replay_action_register("blah", action_blah);

  /* The send action is an override, so we have to first save its previous value in a global */
  int new_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &new_rank);
  if (new_rank != rank)
    XBT_WARN("Rank inconsistency. Got %d, expected %d", new_rank, rank);
  if (rank == 0) {
    previous_send = xbt_replay_action_get("send");
    xbt_replay_action_register("send", overriding_send);
  }
  /* The regular run of the replayer */
  smpi_replay_main(rank, trace_filename);
  return 0;
}
