/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "msg/msg.h"
#include "surf/surf_private.h"

extern routing_global_t global_routing;
int totalHosts= 0;
const m_host_t *hosts;

int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);

XBT_LOG_NEW_DEFAULT_CATEGORY(ring,
                             "Messages specific for this msg example");

int master(int argc, char *argv[])
{
	m_task_t task_s = NULL;
	m_task_t task_r = NULL;
	unsigned int task_comp_size = 50000000;
	unsigned int task_comm_size = 1000000;
	char mailbox[80];
	char buffer[20];
	int num = atoi(argv[1]);

    sprintf(mailbox, "host%d", num+1);
    if(num == totalHosts-1)
    	sprintf(mailbox, "host%d", 0);
    sprintf(buffer, "Token");

    task_s = MSG_task_create(buffer,
							task_comp_size,
							task_comm_size,
							NULL);
    MSG_task_send(task_s,mailbox);
    INFO1("Send Data to \"%s\"", mailbox);

	sprintf(mailbox, "host%d", num);
	MSG_task_receive(&(task_r), mailbox);
	INFO1("Received \"%s\"", MSG_task_get_name(task_r));
	return 0;
}

int slave(int argc, char *argv[])
{
	m_task_t task_s = NULL;
	m_task_t task_r = NULL;
	unsigned int task_comp_size = 50000000;
	unsigned int task_comm_size = 1000000;
	char mailbox[80];
	char buffer[20];
	int num = atoi(argv[1]);

	sprintf(mailbox, "host%d", num);
	MSG_task_receive(&(task_r), mailbox);
	INFO1("Received \"%s\"", MSG_task_get_name(task_r));
	sprintf(mailbox, "host%d", num+1);
	if(num == totalHosts-1)
		sprintf(mailbox, "host%d", 0);
	sprintf(buffer, "Token");
	task_s = MSG_task_create(buffer,
							task_comp_size,
							task_comm_size,
							NULL);
	MSG_task_send(task_s, mailbox);
	INFO1("Send Data to \"%s\"", mailbox);

	return 0;
}

static int surf_parse_bypass_application(void)
{
	int i;
	static int AX_ptr;
	static int surfxml_bufferstack_size = 2048;
	static int surfxml_buffer_stack_stack_ptr = 0;
	static int surfxml_buffer_stack_stack[1024];
	/* allocating memory to the buffer, I think 2MB should be enough */
	surfxml_bufferstack = xbt_new0(char, surfxml_bufferstack_size);

	totalHosts = MSG_get_host_number();
	hosts = MSG_get_host_table();

	/* <platform> */
	SURFXML_BUFFER_SET(platform_version, "3");

	SURFXML_START_TAG(platform);

	DEBUG1("process : %s en master",MSG_host_get_name(hosts[0]));
	/*   <process host="host A" function="master"> */
	SURFXML_BUFFER_SET(process_host, MSG_host_get_name(hosts[0]));
	SURFXML_BUFFER_SET(process_function, "master");
	SURFXML_BUFFER_SET(process_start_time, "-1.0");
	SURFXML_BUFFER_SET(process_kill_time, "-1.0");
	SURFXML_START_TAG(process);

	/*      <argument value="0"/> */
	SURFXML_BUFFER_SET(argument_value, "0");
	SURFXML_START_TAG(argument);
	SURFXML_END_TAG(argument);
	SURFXML_END_TAG(process);

	for(i=1;i<totalHosts;i++)
	{
	DEBUG1("process : %s en slave",MSG_host_get_name(hosts[i]));
	/*   <process host="host A" function="slave"> */
	SURFXML_BUFFER_SET(process_host,MSG_host_get_name(hosts[i]) );
	SURFXML_BUFFER_SET(process_function, "slave");
	SURFXML_BUFFER_SET(process_start_time, "-1.0");
	SURFXML_BUFFER_SET(process_kill_time, "-1.0");
	SURFXML_START_TAG(process);

	/*      <argument value="num"/> */
	SURFXML_BUFFER_SET(argument_value, bprintf("%d",i));
	SURFXML_START_TAG(argument);
	SURFXML_END_TAG(argument);
	SURFXML_END_TAG(process);
	}
	/* </platform> */
	SURFXML_END_TAG(platform);

	free(surfxml_bufferstack);
	return 0;
}

typedef enum {
  PORT_22 = 20,
  MAX_CHANNEL
} channel_t;

int main(int argc, char **argv)
{
	int res;
  MSG_global_init(&argc, argv);
  MSG_set_channel_number(MAX_CHANNEL);
  MSG_create_environment(argv[1]);

  MSG_function_register("master", master);
  MSG_function_register("slave", slave);
  surf_parse = surf_parse_bypass_application;
  MSG_launch_application(NULL);

  res = MSG_main();

  INFO1("Simulation time %g", MSG_get_clock());

  MSG_clean();

  if (res == MSG_OK)
    return 0;
  else
    return 1;

}
