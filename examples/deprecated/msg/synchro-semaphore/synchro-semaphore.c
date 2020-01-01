/* Copyright (c) 2013-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_semaphore_example, "Messages specific for this msg example");

msg_sem_t sem;

static int peer(int argc, char* argv[]){
  int i = 0;
  while(i < argc) {
    double wait_time = xbt_str_parse_double(argv[i],"Invalid wait time: %s");
    i++;
    MSG_process_sleep(wait_time);
    XBT_INFO("Trying to acquire %d", i);
    MSG_sem_acquire(sem);
    XBT_INFO("Acquired %d", i);

    wait_time = xbt_str_parse_double(argv[i], "Invalid wait time: %s");
    i++;
    MSG_process_sleep(wait_time);
    XBT_INFO("Releasing %d", i);
    MSG_sem_release(sem);
    XBT_INFO("Released %d", i);
  }
  MSG_process_sleep(50);
  XBT_INFO("Done");

  return 0;
}

int main(int argc, char* argv[])
{
  MSG_init(&argc, argv);
  MSG_create_environment(argv[1]);

  msg_host_t h = MSG_host_by_name("Fafard");

  sem = MSG_sem_init(1);
  char** aliceTimes = xbt_new(char*, 9);
  aliceTimes[0] = xbt_strdup("0");
  aliceTimes[1] = xbt_strdup("1");
  aliceTimes[2] = xbt_strdup("3");
  aliceTimes[3] = xbt_strdup("5");
  aliceTimes[4] = xbt_strdup("1");
  aliceTimes[5] = xbt_strdup("2");
  aliceTimes[6] = xbt_strdup("5");
  aliceTimes[7] = xbt_strdup("0");
  aliceTimes[8] = NULL;

  char** bobTimes = xbt_new(char*, 9);
  bobTimes[0] = xbt_strdup("0.9");
  bobTimes[1] = xbt_strdup("1");
  bobTimes[2] = xbt_strdup("1");
  bobTimes[3] = xbt_strdup("2");
  bobTimes[4] = xbt_strdup("2");
  bobTimes[5] = xbt_strdup("0");
  bobTimes[6] = xbt_strdup("0");
  bobTimes[7] = xbt_strdup("5");
  bobTimes[8] = NULL;

  MSG_process_create_with_arguments("Alice", peer, NULL, h, 8, aliceTimes);
  MSG_process_create_with_arguments("Bob", peer, NULL, h, 8, bobTimes);

  msg_error_t res = MSG_main();
  MSG_sem_destroy(sem);
  XBT_INFO("Finished\n");
  return (res != MSG_OK);
}
