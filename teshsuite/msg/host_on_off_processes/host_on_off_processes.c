/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"            /* Yeah! If you want to use msg, you need to include simgrid/msg.h */

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

xbt_dynar_t tests;
int tasks_done = 0;

static int process_daemon(int argc, char *argv[])
{
  msg_task_t task = NULL;
  XBT_INFO("  Start daemon on %s (%f)",  MSG_host_get_name(MSG_host_self()), MSG_get_host_speed(MSG_host_self()));
  for(;;){
    task = MSG_task_create("daemon", MSG_get_host_speed(MSG_host_self()), 0, NULL);
    XBT_INFO("  Execute daemon");
    MSG_task_execute(task);
    MSG_task_destroy(task);
    tasks_done ++;
  }
  XBT_INFO("  daemon done. See you!");
  return 0;
}

static int process_sleep(int argc, char *argv[])
{
  for(;;){
    XBT_INFO("  I'm alive but I should sleep");
    MSG_process_sleep(10);
  }
  XBT_INFO("  I'm done. See you!");
  return 0;
}

static int commTX(int argc, char *argv[])
{
  msg_task_t task = NULL;
  char mailbox[80];
  sprintf(mailbox, "comm");
  XBT_INFO("  Start TX");
  task = MSG_task_create("COMM", 0, 100000000, NULL);
  MSG_task_isend(task, mailbox);
  // We should wait a bit (if not the process will end before the communication, hence an exception on the other side).
  MSG_process_sleep(30);
  XBT_INFO("  TX done");
  return 0;
}

static int commRX(int argc, char *argv[])
{
  msg_task_t task = NULL;
  char mailbox[80];
  sprintf(mailbox, "comm");
  XBT_INFO("  Start RX");
  msg_error_t error = MSG_task_receive(&(task), mailbox);
  if (error==MSG_OK) {
    XBT_INFO("  Receive message: %s", task->name);
  } else if (error==MSG_HOST_FAILURE) {
    XBT_INFO("  Receive message: HOST_FAILURE");
  } else if (error==MSG_TRANSFER_FAILURE) {
    XBT_INFO("  Receive message: TRANSFERT_FAILURE");
  } else {
    XBT_INFO("  Receive message: %d", error);
  }
  XBT_INFO("  RX Done");
  return 0;
}

static int test_launcher(int argc, char *argv[])
{
  int test = 0;
  char **argvF;
  argvF = xbt_new(char*, 2);
  argvF[0] = xbt_strdup("process_daemon");
  msg_host_t jupiter = MSG_host_by_name("Jupiter");

  test = 1;
  // Create a process running a simple task on a host and turn the host off during the execution of the process.
  if (xbt_dynar_search_or_negative(tests, &test)!=-1){
    XBT_INFO("Test 1:");
    XBT_INFO("  Create a process on Jupiter");
    argvF = xbt_new(char*, 2);
    argvF[0] = xbt_strdup("process_daemon");
    MSG_process_create_with_arguments("process_daemon", process_daemon, NULL, jupiter, 1, argvF);
    MSG_process_sleep(3);
    XBT_INFO("  Turn off Jupiter");
    MSG_host_off(jupiter);
    MSG_process_sleep(10);
    XBT_INFO("Test 1 seems ok, cool !(#Processes: %d, it should be 1; #tasks: %d)", MSG_process_get_number(), tasks_done);
  }

  test = 2;
  // Create a process that on a host that is turned off (this should not be possible)
  if (xbt_dynar_search_or_negative(tests, &test)!=-1){
    XBT_INFO("Test 2:");
    XBT_INFO("  Turn off Jupiter");
    // adsein: Jupiter is already, hence nothing should happen
    // adsein: This can be one additional test, to check that you cannot shutdown twice a host
    MSG_host_off(jupiter);
    argvF = xbt_new(char*, 2);
    argvF[0] = xbt_strdup("process_daemon");
    MSG_process_create_with_arguments("process_daemon", process_daemon, NULL, jupiter, 1, argvF);
    MSG_process_sleep(10);
    XBT_INFO("  Test 2 does not crash, WTF ?!(number of Process : %d, it should be 1)", MSG_process_get_number());
    XBT_INFO("  Ok so let's turn on/off the node to see whether the process is correctly bound to Jupiter");
    MSG_host_on(jupiter);
    XBT_INFO("  Turn off");
    MSG_host_off(jupiter);
    XBT_INFO("  sleep");
    MSG_process_sleep(10);
    XBT_INFO("number of Process : %d it should be 1. The daemon that has been created for test2 has been correctly "
             "destroyed....ok at least it looks rigorous, cool ! You just have to disallow the possibility to create "
             "a new process on a node when the node is off.)", MSG_process_get_number());
  }

   test = 3;
  // Create a process running sucessive sleeps on a host and turn the host off during the execution of the process.
  if (xbt_dynar_search_or_negative(tests, &test)!=-1){
    XBT_INFO("Test 3:");
    MSG_host_on(jupiter);
    argvF = xbt_new(char*, 2);
    argvF[0] = xbt_strdup("process_sleep");
    MSG_process_create_with_arguments("process_sleep", process_sleep, NULL, jupiter, 1, argvF);
    MSG_process_sleep(100);
    XBT_INFO("  Turn off");
    MSG_host_off(jupiter);
    XBT_INFO("  sleep for 10 seconds");
    MSG_process_sleep(10000);
    XBT_INFO("number of Process : %d it should be 1 (i.e. the Test one))", MSG_process_get_number());
  }

  test = 4;
  if (xbt_dynar_search_or_negative(tests, &test)!=-1){
    XBT_INFO("Test 4 (turn off src during a communication) : Create a Process/task to make a communication between "
             "Jupiter and Tremblay and turn off Jupiter during the communication");
    MSG_host_on(jupiter);
    MSG_process_sleep(10);
    argvF = xbt_new(char*, 2);
    argvF[0] = xbt_strdup("commRX");
    MSG_process_create_with_arguments("commRX", commRX, NULL, MSG_host_by_name("Tremblay"), 1, argvF);
    argvF = xbt_new(char*, 2);
    argvF[0] = xbt_strdup("commTX");
    MSG_process_create_with_arguments("commTX", commTX, NULL, jupiter, 1, argvF);
    XBT_INFO("  number of processes: %d", MSG_process_get_number());
    MSG_process_sleep(10);
    XBT_INFO("  Turn Jupiter off");
    MSG_host_off(jupiter);
    XBT_INFO("Test 4 seems ok  (number of Process : %d, it should be 1 or 2 if RX has not been satisfied) cool, you "
             "can now turn off a node that has a process paused by a sleep call", MSG_process_get_number());
  }

  test = 5;
  if (xbt_dynar_search_or_negative(tests, &test)!=-1){
    XBT_INFO("Test 5 (turn off dest during a communication : Create a Process/task to make a communication between "
             "Tremblay and Jupiter and turn off Jupiter during the communication");
    MSG_host_on(jupiter);
    MSG_process_sleep(10);
    argvF = xbt_new(char*, 2);
    argvF[0] = xbt_strdup("commRX");
    MSG_process_create_with_arguments("commRX", commRX, NULL, jupiter, 1, argvF);
    argvF = xbt_new(char*, 2);
    argvF[0] = xbt_strdup("commTX");
    MSG_process_create_with_arguments("commTX", commTX, NULL, MSG_host_by_name("Tremblay"), 1, argvF);
    XBT_INFO("  number of processes: %d", MSG_process_get_number());
    MSG_process_sleep(10);
    XBT_INFO("  Turn Jupiter off");
    MSG_host_off(jupiter);
    XBT_INFO("Test 5 seems ok, cool !(number of Process : %d, it should be 2", MSG_process_get_number());
  }

  test =6;
  if (xbt_dynar_search_or_negative(tests, &test)!=-1){
    XBT_INFO("Test 6: Turn on Jupiter, assign a VM on Jupiter, launch a process inside the VM, and turn off the node");

    // Create VM0
    int dpRate = 70;
    msg_vm_t vm0;
    msg_process_t daemon;

    vm0 = MSG_vm_create (jupiter, "vm0", 1, 2048, 125, NULL, -1, 125, dpRate);
    MSG_vm_start(vm0);

    argvF = xbt_new(char*, 2);
    argvF[0] = xbt_strdup("process_daemon");
    daemon = MSG_process_create_with_arguments("process_daemon", process_daemon, NULL, vm0, 1, argvF);

    argvF = xbt_new(char*, 2);
    argvF[0] = xbt_strdup("process_daemonJUPI");
    MSG_process_create_with_arguments("process_daemonJUPI", process_daemon, NULL, jupiter, 1, argvF);

    MSG_process_suspend(daemon);
    MSG_vm_set_bound(vm0, 90);
    MSG_process_resume(daemon);

    MSG_process_sleep(10);

    XBT_INFO("  Turn Jupiter off");
    MSG_host_off(jupiter);
    XBT_INFO("  Shutdown vm0");
    MSG_vm_shutdown(vm0);
    XBT_INFO("  Destroy vm0");
    MSG_vm_destroy(vm0);
    XBT_INFO("Test 6 is also weird: when the node Jupiter is turned off once again, the VM and its daemon are not "
             "killed. However, the issue regarding the shutdown of hosted VMs can be seen a feature not a bug ;)");
  }

  XBT_INFO("  Test done. See you!");
  return 0;
}

int main(int argc, char *argv[])
{
  msg_error_t res;

  MSG_init(&argc, argv);
  xbt_assert(argc == 3,"Usage: %s platform_file test_number\n\tExample: %s msg_platform.xml 1\n", argv[0], argv[0]);

  unsigned int iter;
  char *groups;
  xbt_dynar_t s_tests = xbt_str_split(argv[2], ",");
  int tmp_test = 0;
  tests = xbt_dynar_new(sizeof(int), NULL);
  xbt_dynar_foreach(s_tests, iter, groups) {
     sscanf(xbt_dynar_get_as(s_tests, iter, char *), "%d", &tmp_test);
     xbt_dynar_set_as(tests, iter, int, tmp_test);
  }
  xbt_dynar_free(&s_tests);

  MSG_create_environment(argv[1]);

  MSG_process_create("test_launcher", test_launcher, NULL, MSG_get_host_by_name("Tremblay"));

  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
