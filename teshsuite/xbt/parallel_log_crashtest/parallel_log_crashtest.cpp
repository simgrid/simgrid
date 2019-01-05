/* synchro_crashtest -- tries to crash the logging mechanism by doing parallel logs*/

/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include <thread>

XBT_LOG_NEW_DEFAULT_CATEGORY(synchro_crashtest, "Logs of this example");

const int test_amount    = 99; /* Up to 99 to not break the logs (and thus the testing mechanism) */
const int crasher_amount = 99; /* Up to 99 to not break the logs (and thus the testing mechanism) */

int more_info = 0; /* SET IT TO TRUE TO GET MORE INFO */

/* Code ran by each thread */
static void* crasher_thread(void* arg)
{
  int id = *(int*)arg;

  for (int i = 0; i < test_amount; i++) {
    if (more_info)
      XBT_INFO("%03d (%02d|%02d|%02d|%02d|%02d|%02d|%02d|%02d|%02d)", test_amount - i, id, id, id, id, id, id, id, id,
               id);
    else
      XBT_INFO("XXX (XX|XX|XX|XX|XX|XX|XX|XX|XX)");
  }
  return NULL;
}

int main(int argc, char* argv[])
{
  MSG_init(&argc, argv);

  int id[crasher_amount];
  std::thread crashers[crasher_amount];

  /* spawn threads */
  for (int i = 0; i < crasher_amount; i++) {
    id[i]       = i;
    crashers[i] = std::thread(crasher_thread, &id[i]);
  }

  /* wait for them */
  for (int i = 0; i < crasher_amount; i++)
    crashers[i].join();

  return 0;
}
