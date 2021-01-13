/* synchro_crashtest -- tries to crash the logging mechanism by doing parallel logs*/

/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Engine.hpp"
#include "xbt/log.h"
#include <array>
#include <thread>

XBT_LOG_NEW_DEFAULT_CATEGORY(synchro_crashtest, "Logs of this example");

constexpr int test_amount    = 99; /* Up to 99 to not break the logs (and thus the testing mechanism) */
constexpr int crasher_amount = 99; /* Up to 99 to not break the logs (and thus the testing mechanism) */

constexpr bool more_info = false; /* SET IT TO TRUE TO GET MORE INFO */

/* Code ran by each thread */
static void crasher_thread(int id)
{
  for (int i = 0; i < test_amount; i++) {
    if (more_info)
      XBT_INFO("%03d (%02d|%02d|%02d|%02d|%02d|%02d|%02d|%02d|%02d)", test_amount - i, id, id, id, id, id, id, id, id,
               id);
    else
      XBT_INFO("XXX (XX|XX|XX|XX|XX|XX|XX|XX|XX)");
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  std::array<std::thread, crasher_amount> crashers;

  /* spawn threads */
  int id = 0;
  for (std::thread& thr : crashers)
    thr = std::thread(crasher_thread, id++);

  /* wait for them */
  for (std::thread& thr : crashers)
    thr.join();

  return 0;
}
