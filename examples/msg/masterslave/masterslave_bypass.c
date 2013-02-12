/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "surf/surfxml_parse.h" /* to override surf_parse and bypass the parser */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");
#define FINALIZE ((void*)221297)        /* a magic number to tell people to stop working */

static int surf_parse_bypass_platform(void)
{
  static int AX_ptr = 0;
  static int surfxml_bufferstack_size = 2048;
  static int surfxml_buffer_stack_stack_ptr = 0;
  static int surfxml_buffer_stack_stack[1024];
  /* allocating memory for the buffer, I think 2kB should be enough */
  surfxml_bufferstack = xbt_new0(char, surfxml_bufferstack_size);

  XBT_DEBUG("<platform>");
  SURFXML_BUFFER_SET(platform_version, "3");
  SURFXML_START_TAG(platform);

  XBT_DEBUG("<AS>");
  SURFXML_BUFFER_SET(AS_id, "AS0");
  A_surfxml_AS_routing = A_surfxml_AS_routing_Full;
  SURFXML_START_TAG(AS);

  XBT_DEBUG("<host id=\"host A\" power=\"100000000.00\"/>");
  SURFXML_BUFFER_SET(host_id, "host A");
  SURFXML_BUFFER_SET(host_power, "100000000.00");
  SURFXML_BUFFER_SET(host_availability, "1.0");
  SURFXML_BUFFER_SET(host_availability___file, "");
  SURFXML_BUFFER_SET(host_core, "1");
  A_surfxml_host_state = A_surfxml_host_state_ON;
  SURFXML_BUFFER_SET(host_state___file, "");
  SURFXML_BUFFER_SET(host_coordinates, "");
  SURFXML_START_TAG(host);
  SURFXML_END_TAG(host);

  XBT_DEBUG("<host id=\"host B\" power=\"100000000.00\"/>");
  SURFXML_BUFFER_SET(host_id, "host B");
  SURFXML_BUFFER_SET(host_power, "100000000.00");
  SURFXML_BUFFER_SET(host_availability, "1.0");
  SURFXML_BUFFER_SET(host_availability___file, "");
  SURFXML_BUFFER_SET(host_core, "1");
  A_surfxml_host_state = A_surfxml_host_state_ON;
  SURFXML_BUFFER_SET(host_state___file, "");
  SURFXML_BUFFER_SET(host_coordinates, "");
  SURFXML_START_TAG(host);
  SURFXML_END_TAG(host);

  XBT_DEBUG("<link id=\"LinkA\" bandwidth=\"10000000.0\" latency=\"0.2\"/>");
  SURFXML_BUFFER_SET(link_id, "LinkA");
  SURFXML_BUFFER_SET(link_bandwidth, "10000000.0");
  SURFXML_BUFFER_SET(link_bandwidth___file, "");
  SURFXML_BUFFER_SET(link_latency, "0.2");
  SURFXML_BUFFER_SET(link_latency___file, "");
  A_surfxml_link_state = A_surfxml_link_state_ON;
  SURFXML_BUFFER_SET(link_state___file, "");
  A_surfxml_link_sharing___policy = A_surfxml_link_sharing___policy_SHARED;
  SURFXML_START_TAG(link);
  SURFXML_END_TAG(link);

  XBT_DEBUG("<route src=\"host A\" dst=\"host B\">");
  SURFXML_BUFFER_SET(route_src, "host A");
  SURFXML_BUFFER_SET(route_dst, "host B");
  A_surfxml_route_symmetrical = A_surfxml_route_symmetrical_YES;
  SURFXML_START_TAG(route);
  XBT_DEBUG("  <link:ctn id=\"LinkA\"/>");
  SURFXML_BUFFER_SET(link___ctn_id, "LinkA");
  A_surfxml_link___ctn_direction = A_surfxml_link___ctn_direction_NONE;
  SURFXML_START_TAG(link___ctn);
  SURFXML_END_TAG(link___ctn);
  XBT_DEBUG("</route>");
  SURFXML_END_TAG(route);

  XBT_DEBUG("</AS>");
  SURFXML_END_TAG(AS);
  XBT_DEBUG("</platfrom>");
  SURFXML_END_TAG(platform);

  free(surfxml_bufferstack);
  return 0;
}

static int surf_parse_bypass_application(void)
{
  static int AX_ptr;
  static int surfxml_bufferstack_size = 2048;
  static int surfxml_buffer_stack_stack_ptr = 0;
  static int surfxml_buffer_stack_stack[1024];
  /* allocating memory to the buffer, I think 2MB should be enough */
  surfxml_bufferstack = xbt_new0(char, surfxml_bufferstack_size);

  /* <platform> */
  SURFXML_BUFFER_SET(platform_version, "3");

  SURFXML_START_TAG(platform);

/*   <process host="host A" function="master"> */
  SURFXML_BUFFER_SET(process_host, "host A");
  SURFXML_BUFFER_SET(process_function, "master");
  SURFXML_BUFFER_SET(process_start___time, "-1.0");
  SURFXML_BUFFER_SET(process_kill___time, "-1.0");
  SURFXML_START_TAG(process);

/*      <argument value="20"/> */
  SURFXML_BUFFER_SET(argument_value, "20");
  SURFXML_START_TAG(argument);
  SURFXML_END_TAG(argument);

/*      <argument value="5000000"/> */
  SURFXML_BUFFER_SET(argument_value, "5000000");
  SURFXML_START_TAG(argument);
  SURFXML_END_TAG(argument);

/*      <argument value="100000"/> */
  SURFXML_BUFFER_SET(argument_value, "100000");
  SURFXML_START_TAG(argument);
  SURFXML_END_TAG(argument);

/*      <argument value="host B"/> */
  SURFXML_BUFFER_SET(argument_value, "host B");
  SURFXML_START_TAG(argument);
  SURFXML_END_TAG(argument);

/*   </process> */
  SURFXML_END_TAG(process);

/*   <process host="host B" function="slave"/> */
  SURFXML_BUFFER_SET(process_host, "host B");
  SURFXML_BUFFER_SET(process_function, "slave");
  SURFXML_BUFFER_SET(process_start___time, "-1.0");
  SURFXML_BUFFER_SET(process_kill___time, "-1.0");
  SURFXML_START_TAG(process);
  SURFXML_END_TAG(process);

/* </platform> */
  SURFXML_END_TAG(platform);

  free(surfxml_bufferstack);
  return 0;
}

int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);
msg_error_t test_all(void);

/** Emitter function  */
int master(int argc, char *argv[])
{
  int slaves_count = 0;
  msg_host_t *slaves = NULL;
  msg_task_t *todo = NULL;
  int number_of_tasks = 0;
  double task_comp_size = 0;
  double task_comm_size = 0;
  int i;
  _XBT_GNUC_UNUSED int read;

  read = sscanf(argv[1], "%d", &number_of_tasks);
  xbt_assert(read, "Invalid argument %s\n", argv[1]);
  read = sscanf(argv[2], "%lg", &task_comp_size);
  xbt_assert(read, "Invalid argument %s\n", argv[2]);
  read = sscanf(argv[3], "%lg", &task_comm_size);
  xbt_assert(read, "Invalid argument %s\n", argv[3]);

  {                             /*  Task creation */
    char sprintf_buffer[64];

    todo = xbt_new0(msg_task_t, number_of_tasks);

    for (i = 0; i < number_of_tasks; i++) {
      sprintf(sprintf_buffer, "Task_%d", i);
      todo[i] =
          MSG_task_create(sprintf_buffer, task_comp_size, task_comm_size,
                          NULL);
    }
  }

  {                             /* Process organisation */
    slaves_count = argc - 4;
    slaves = xbt_new0(msg_host_t, slaves_count);

    for (i = 4; i < argc; i++) {
      slaves[i - 4] = MSG_get_host_by_name(argv[i]);
      if (slaves[i - 4] == NULL) {
        XBT_INFO("Unknown host %s. Stopping Now! ", argv[i]);
        abort();
      }
    }
  }

  XBT_INFO("Got %d slave(s) :", slaves_count);
  for (i = 0; i < slaves_count; i++)
    XBT_INFO("\t %s", MSG_host_get_name(slaves[i]));

  XBT_INFO("Got %d task to process :", number_of_tasks);

  for (i = 0; i < number_of_tasks; i++)
    XBT_INFO("\t\"%s\"", todo[i]->name);

  for (i = 0; i < number_of_tasks; i++) {
    XBT_INFO("Sending \"%s\" to \"%s\"",
          todo[i]->name, MSG_host_get_name(slaves[i % slaves_count]));
    if (MSG_host_self() == slaves[i % slaves_count]) {
      XBT_INFO("Hey ! It's me ! :)");
    }
    MSG_task_send(todo[i], MSG_host_get_name(slaves[i % slaves_count]));
    XBT_INFO("Send completed");
  }

  XBT_INFO
      ("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (i = 0; i < slaves_count; i++)
    MSG_task_send(MSG_task_create("finalize", 0, 0, FINALIZE),
        MSG_host_get_name(slaves[i]));

  XBT_INFO("Goodbye now!");
  free(slaves);
  free(todo);
  return 0;
}                               /* end_of_master */

/** Receiver function  */
int slave(int argc, char *argv[])
{
  XBT_INFO("I'm a slave");
  while (1) {
    msg_task_t task = NULL;
    int a;
    a = MSG_task_receive(&task, MSG_host_get_name(MSG_host_self()));
    if (a == MSG_OK) {
      XBT_INFO("Received \"%s\" ", MSG_task_get_name(task));
      if (MSG_task_get_data(task) == FINALIZE) {
        MSG_task_destroy(task);
        break;
      }
      XBT_INFO("Processing \"%s\" ", MSG_task_get_name(task));
      MSG_task_execute(task);
      XBT_INFO("\"%s\" done ", MSG_task_get_name(task));
      MSG_task_destroy(task);
    } else {
      XBT_INFO("Hey ?! What's up ? ");
      xbt_die( "Unexpected behavior");
    }
  }
  XBT_INFO("I'm done. See you!");
  return 0;
}                               /* end_of_slave */

/** Test function */
msg_error_t test_all(void)
{
  msg_error_t res = MSG_OK;

  /*  Simulation setting */
  surf_parse = surf_parse_bypass_platform;
  MSG_create_environment(NULL);

  /*   Application deployment */
  MSG_function_register("master", master);
  MSG_function_register("slave", slave);
  surf_parse = surf_parse_bypass_application;
  MSG_launch_application(NULL);

  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());
  return res;
}                               /* end_of_test_all */

/** Main function */
int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

  MSG_init(&argc, argv);
  res = test_all();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
