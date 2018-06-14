/* Copyright (c) 2009-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi/smpi.h"
#include "smpi/sampi.h"
#include <simgrid/s4u.hpp>
#include <simgrid/plugins/load_balancer.h>
#include <simgrid/plugins/load.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(sampi_load_balancer_test, "Messages specific for this sampi example");


int main(int argc, char* argv[])
{
  sg_host_load_plugin_init();
  smpi_replay_init(&argc, &argv);
  sg_load_balancer_plugin_init(); // Must be called after smpi_replay_init as this will overwrite some replay actions

  smpi_replay_main(&argc, &argv);
  return 0;
}
