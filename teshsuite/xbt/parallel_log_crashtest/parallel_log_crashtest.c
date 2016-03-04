/* synchro_crashtest -- tries to crash the logging mecanism by doing // logs*/

/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(synchro_crashtest, "Logs of this example");

int test_amount = 99;           /* Up to 999 to not break the logs (and thus the testing mecanism) */
int crasher_amount = 99;        /* Up to 99  to not break the logs (and thus the testing mecanism) */
int *id;                        /* to pass a pointer to the threads without race condition */

int more_info = 0;              /* SET IT TO TRUE TO GET MORE INFO */

/* Code ran by each thread */
static void* crasher_thread(void *arg)
{
  int id = *(int *) arg;

  for (int i = 0; i < test_amount; i++) {
    if (more_info)
      XBT_INFO("%03d (%02d|%02d|%02d|%02d|%02d|%02d|%02d|%02d|%02d)",
             test_amount - i, id, id, id, id, id, id, id, id, id);
    else
      XBT_INFO("XXX (XX|XX|XX|XX|XX|XX|XX|XX|XX)");
  }
  return NULL;
}

static int crasher(int argc, char *argv[])
{
  int i;
  xbt_os_thread_t *crashers;

  xbt_init(&argc, argv);

  /* initializations of the philosopher mecanisms */
  id = xbt_new0(int, crasher_amount);
  crashers = xbt_new(xbt_os_thread_t, crasher_amount);

  for (i = 0; i < crasher_amount; i++)
    id[i] = i;

  /* spawn threads */
  for (i = 0; i < crasher_amount; i++) {
    char *name = bprintf("thread %d", i);
    crashers[i] = xbt_os_thread_create(name, &crasher_thread, &id[i], NULL );
    free(name);
  }

  /* wait for them */
  for (i = 0; i < crasher_amount; i++)
    xbt_os_thread_join(crashers[i],NULL);

  xbt_free(crashers);
  xbt_free(id);

  return 0;
}

int main(int argc, char *argv[])
{
  return crasher(argc, argv);
}
