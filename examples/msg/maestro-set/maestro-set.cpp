/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @addtogroup MSG_examples
 *
 *  - <b>maestro-set/maestro-set.cpp: Switch the system thread hosting our maestro</b>.
 *    That's a very advanced example in which we move the maestro thread to another process.
 *    Not many users need it (maybe only one, actually), but this example is also a regression test.
 *
 *    This example is in C++ because we use C++11 threads to ensure that the feature is working as
 *    expected. You can still use that feature from a C code.
 */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

#include <thread>

std::thread::id root_id;

static void ensure_root_tid() {
  std::thread::id this_id = std::this_thread::get_id();
  xbt_assert(root_id == this_id, "I was supposed to be the main thread");
  XBT_INFO("I am the main thread, as expected");
}
static void ensure_other_tid() {
  std::thread::id this_id = std::this_thread::get_id();
  xbt_assert(this_id != root_id, "I was NOT supposed to be the main thread");
  XBT_INFO("I am not the main thread, as expected");
}



static int sender(int argc, char *argv[])
{
  ensure_root_tid();

  msg_task_t task_la = MSG_task_create("Some task", 0.0, 10e8, NULL);
  MSG_task_send(task_la, "some mailbox");

  return 0;
}

static int receiver(int argc, char *argv[])
{
  ensure_other_tid();

  msg_task_t task_la = NULL;
  xbt_assert(MSG_task_receive(&task_la,"some mailbox") == MSG_OK);
  XBT_INFO("Task received");
  MSG_task_destroy(task_la);

  return 0;
}

static void maestro(void* data)
{
  ensure_other_tid();
  MSG_process_create("receiver",&receiver,NULL,MSG_host_by_name("Jupiter"));
  MSG_main();
}

/** Main function */
int main(int argc, char *argv[])
{
  root_id = std::this_thread::get_id();

  SIMIX_set_maestro(maestro, NULL);
  MSG_init(&argc, argv);

  if (argc != 2) {
    XBT_CRITICAL("Usage: %s platform_file\n", argv[0]);
    xbt_die("example: %s msg_platform.xml\n",argv[0]);
  }

  MSG_create_environment(argv[1]);

  /* Become one of the simulated process.
   *
   * This must be done after the creation of the platform because we are depending attaching to a host.*/
  MSG_process_attach("sender", NULL, MSG_host_by_name("Tremblay"), NULL);
  ensure_root_tid();

  // Execute the sender code:
  const char* subargv[3] = { "sender", "Jupiter", NULL };
  sender(2, (char**) subargv);

  MSG_process_detach(); // Become root thread again
  XBT_INFO("Detached");
  ensure_root_tid();

  return 0;
}
