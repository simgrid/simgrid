/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_semaphore_example, "Messages specific for this msg example");

msg_sem_t sem;

static int peer(int argc, char* argv[]){
  int i = 0; 
  while(i < argc) {
    double wait_time = xbt_str_parse_double(argv[i++],"Invalid wait time: %s");
    MSG_process_sleep(wait_time);
    XBT_INFO("Trying to acquire %d", i);
    MSG_sem_acquire(sem);
    XBT_INFO("Acquired %d", i);

    wait_time = xbt_str_parse_double(argv[i++], "Invalid wait time: %s");
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

  xbt_dynar_t hosts = MSG_hosts_as_dynar();
  msg_host_t h = xbt_dynar_get_as(hosts,0,msg_host_t);

  sem = MSG_sem_init(1);
  char** aliceTimes = xbt_new(char*, 9);
  int nbAlice = 0;
  aliceTimes[nbAlice++] = xbt_strdup("0"); 
  aliceTimes[nbAlice++] = xbt_strdup("1");
  aliceTimes[nbAlice++] = xbt_strdup("3");
  aliceTimes[nbAlice++] = xbt_strdup("5");
  aliceTimes[nbAlice++] = xbt_strdup("1");
  aliceTimes[nbAlice++] = xbt_strdup("2");
  aliceTimes[nbAlice++] = xbt_strdup("5");
  aliceTimes[nbAlice++] = xbt_strdup("0");
  aliceTimes[nbAlice++] = NULL;

  char** bobTimes = xbt_new(char*, 9);
  int nbBob = 0;
  bobTimes[nbBob++] = xbt_strdup("0.9"); 
  bobTimes[nbBob++] = xbt_strdup("1");
  bobTimes[nbBob++] = xbt_strdup("1");
  bobTimes[nbBob++] = xbt_strdup("2");
  bobTimes[nbBob++] = xbt_strdup("2");
  bobTimes[nbBob++] = xbt_strdup("0");
  bobTimes[nbBob++] = xbt_strdup("0");
  bobTimes[nbBob++] = xbt_strdup("5");
  bobTimes[nbBob++] = NULL;

  MSG_process_create_with_arguments(xbt_strdup("Alice"), peer, NULL, h, 8, aliceTimes);
  MSG_process_create_with_arguments(xbt_strdup("Bob"), peer, NULL, h, 8, bobTimes);

  msg_error_t res = MSG_main();
  XBT_INFO("Finished\n");
  return (res != MSG_OK);
}
