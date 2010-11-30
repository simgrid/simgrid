/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "surf/surfxml_parse.h" /* to override surf_parse and bypass the parser */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

#define FINALIZE ((void*)221297)        /* a magic number to tell people to stop working */

MSG_error_t test_all(const char *platform_file);


int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);

typedef enum {
  PORT_22 = 0,
  MAX_CHANNEL
} channel_t;

/** Emitter function  */
int master(int argc, char *argv[])
{
  int slaves_count = 0;
  m_host_t *slaves = NULL;
  m_task_t *todo = NULL;
  int number_of_tasks = 0;
  double task_comp_size = 0;
  double task_comm_size = 0;


  int i;

  xbt_assert1(sscanf(argv[1], "%d", &number_of_tasks),
              "Invalid argument %s\n", argv[1]);
  xbt_assert1(sscanf(argv[2], "%lg", &task_comp_size),
              "Invalid argument %s\n", argv[2]);
  xbt_assert1(sscanf(argv[3], "%lg", &task_comm_size),
              "Invalid argument %s\n", argv[3]);

  {                             /*  Task creation */
    char sprintf_buffer[64];

    todo = xbt_new0(m_task_t, number_of_tasks);

    for (i = 0; i < number_of_tasks; i++) {
      sprintf(sprintf_buffer, "Task_%d", i);
      todo[i] =
          MSG_task_create(sprintf_buffer, task_comp_size, task_comm_size,
                          NULL);
    }
  }

  {                             /* Process organisation */
    slaves_count = argc - 4;
    slaves = xbt_new0(m_host_t, slaves_count);

    for (i = 4; i < argc; i++) {
      slaves[i - 4] = MSG_get_host_by_name(argv[i]);
      if (slaves[i - 4] == NULL) {
        INFO1("Unknown host %s. Stopping Now! ", argv[i]);
        abort();
      }
    }
  }

  INFO1("Got %d slave(s) :", slaves_count);
  for (i = 0; i < slaves_count; i++)
    INFO1("\t %s", slaves[i]->name);

  INFO1("Got %d task to process :", number_of_tasks);

  for (i = 0; i < number_of_tasks; i++)
    INFO1("\t\"%s\"", todo[i]->name);

  for (i = 0; i < number_of_tasks; i++) {
    INFO2("Sending \"%s\" to \"%s\"",
          todo[i]->name, slaves[i % slaves_count]->name);
    if (MSG_host_self() == slaves[i % slaves_count]) {
      INFO0("Hey ! It's me ! :)");
    }
    MSG_task_put(todo[i], slaves[i % slaves_count], PORT_22);
    INFO0("Send completed");
  }

  INFO0
      ("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (i = 0; i < slaves_count; i++)
    MSG_task_put(MSG_task_create("finalize", 0, 0, FINALIZE),
                 slaves[i], PORT_22);

  INFO0("Goodbye now!");
  free(slaves);
  free(todo);
  return 0;
}                               /* end_of_master */

/** Receiver function  */
int slave(int argc, char *argv[])
{
  INFO0("I'm a slave");
  while (1) {
    m_task_t task = NULL;
    int a;
    a = MSG_task_get(&(task), PORT_22);
    if (a == MSG_OK) {
      INFO1("Received \"%s\" ", MSG_task_get_name(task));
      if (MSG_task_get_data(task) == FINALIZE) {
        MSG_task_destroy(task);
        break;
      }
      INFO1("Processing \"%s\" ", MSG_task_get_name(task));
      MSG_task_execute(task);
      INFO1("\"%s\" done ", MSG_task_get_name(task));
      MSG_task_destroy(task);
    } else {
      INFO0("Hey ?! What's up ? ");
      xbt_assert0(0, "Unexpected behavior");
    }
  }
  INFO0("I'm done. See you!");
  return 0;
}                               /* end_of_slave */


/** Bypass deployment **/
static int bypass_deployment(void)
{
	int nb_host,i;
	static int AX_ptr;
	static int surfxml_bufferstack_size = 2048;
	static int surfxml_buffer_stack_stack_ptr = 0;
	static int surfxml_buffer_stack_stack[1024];
	/* allocating memory to the buffer, I think 2MB should be enough */
	surfxml_bufferstack = xbt_new0(char, surfxml_bufferstack_size);
	nb_host = MSG_get_host_number();

	/* <platform> */
	SURFXML_BUFFER_SET(platform_version, "3");
	SURFXML_START_TAG(platform);
	DEBUG0("<platform version=\"3\">");

	  DEBUG0("	<process host=\"c-0.me\" function=\"master\">");
	  SURFXML_BUFFER_SET(process_host, "c-0.me");
	  SURFXML_BUFFER_SET(process_function, "master");
	  SURFXML_BUFFER_SET(process_start_time, "-1.0");
	  SURFXML_BUFFER_SET(process_kill_time, "-1.0");
	  SURFXML_START_TAG(process);

	  DEBUG1("		<argument value=\"%s\"/>",bprintf("%d",nb_host-1));
	  SURFXML_BUFFER_SET(argument_value, bprintf("%d",nb_host-1));
	  SURFXML_START_TAG(argument);
	  SURFXML_END_TAG(argument);

	  DEBUG0("		<argument value=\"5000000\"/>");
	  SURFXML_BUFFER_SET(argument_value, "5000000");
	  SURFXML_START_TAG(argument);
	  SURFXML_END_TAG(argument);

	  DEBUG0("		<argument value=\"100000\"/>");
	  SURFXML_BUFFER_SET(argument_value, "100000");
	  SURFXML_START_TAG(argument);
	  SURFXML_END_TAG(argument);

	for(i=1 ; i<nb_host ; i++)
	{
	  DEBUG1("		<argument value=\"%s.me\"/>",bprintf("c-%d",i));
	  SURFXML_BUFFER_SET(argument_value, bprintf("c-%d.me",i));
	  SURFXML_START_TAG(argument);
	  SURFXML_END_TAG(argument);
	}
	DEBUG0("	</process>");
	SURFXML_END_TAG(process);

	for(i=1 ; i<nb_host ; i++)
	{
	  DEBUG1("	<process host=\"%s.me\" function=\"slave\"/>",bprintf("c-%d",i));
	  SURFXML_BUFFER_SET(process_host, bprintf("c-%d.me",i));
	  SURFXML_BUFFER_SET(process_function, "slave");
	  SURFXML_BUFFER_SET(process_start_time, "-1.0");
	  SURFXML_BUFFER_SET(process_kill_time, "-1.0");
	  SURFXML_START_TAG(process);
	  SURFXML_END_TAG(process);
	}

	DEBUG0("</platform>");
	SURFXML_END_TAG(platform);

	free(surfxml_bufferstack);
	return 0;
}

/** Test function */
MSG_error_t test_all(const char *platform_file)
{
	MSG_error_t res = MSG_OK;
	MSG_set_channel_number(MAX_CHANNEL);
	MSG_create_environment(platform_file);
	MSG_function_register("master", master);
	MSG_function_register("slave", slave);
	surf_parse = bypass_deployment;
	MSG_launch_application(NULL);

	res = MSG_main();

	INFO1("Simulation time %g", MSG_get_clock());
	return res;
}                               /* end_of_test_all */


/** Main function */
int main(int argc, char *argv[])
{
	MSG_error_t res = MSG_OK;

	MSG_global_init(&argc, argv);
	res = test_all(argv[1]);
	MSG_clean();

	if (res == MSG_OK)
		return 0;
	else
		return 1;
}                               /* end_of_main */
