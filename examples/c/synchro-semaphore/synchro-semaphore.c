/* Copyright (c) 2013-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"
#include "simgrid/host.h"
#include "simgrid/semaphore.h"

#include "xbt/log.h"
#include "xbt/str.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(semaphores, "Messages specific for this example");

sg_sem_t sem;

static void peer(int argc, char* argv[])
{
  int i = 0;
  while (i < argc) {
    double wait_time = xbt_str_parse_double(argv[i], "Invalid wait time: %s");
    i++;
    sg_actor_sleep_for(wait_time);
    XBT_INFO("Trying to acquire %d", i);
    sg_sem_acquire(sem);
    XBT_INFO("Acquired %d", i);

    wait_time = xbt_str_parse_double(argv[i], "Invalid wait time: %s");
    i++;
    sg_actor_sleep_for(wait_time);
    XBT_INFO("Releasing %d", i);
    sg_sem_release(sem);
    XBT_INFO("Released %d", i);
  }
  sg_actor_sleep_for(50);
  XBT_INFO("Done");
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  simgrid_load_platform(argv[1]);

  sg_host_t h = sg_host_by_name("Fafard");

  sem                      = sg_sem_init(1);
  const char* aliceTimes[] = {"0", "1", "3", "5", "1", "2", "5", "0"};
  const char* bobTimes[]   = {"0.9", "1", "1", "2", "2", "0", "0", "5"};

  sg_actor_create("Alice", h, peer, 8, aliceTimes);
  sg_actor_create("Bob", h, peer, 8, bobTimes);

  simgrid_run();
  sg_sem_destroy(sem);
  XBT_INFO("Finished\n");
  return 0;
}
