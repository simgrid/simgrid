/* Copyright (c) 2008, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "msg/msg.h" /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "xbt/sysdep.h" /* calloc, printf */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,"Messages specific for this msg example");


typedef enum {
  PORT_22 = 0,
  MAX_CHANNEL
} channel_t;

int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);
int forwarder(int argc, char *argv[]);
MSG_error_t test_all(const char *platform_file, const char *application_file);


#define FINALIZE ((void*)221297) /* a magic number to tell people to stop working */

/** Emitter function  */
int master(int argc, char *argv[])
{
	int alias_count = 0;
	char** aliases = NULL;
	m_task_t *todo = NULL;
	int number_of_tasks = 0;
	double task_comp_size = 0;
	double task_comm_size = 0;


	int i;

	xbt_assert1(sscanf(argv[1],"%d", &number_of_tasks),"Invalid argument %s\n",argv[1]);
	xbt_assert1(sscanf(argv[2],"%lg", &task_comp_size),"Invalid argument %s\n",argv[2]);
	xbt_assert1(sscanf(argv[3],"%lg", &task_comm_size),"Invalid argument %s\n",argv[3]);

	{
		/*  Task creation */
		char sprintf_buffer[64];

		todo = xbt_new0(m_task_t,number_of_tasks);

		for (i = 0; i < number_of_tasks; i++) 
		{
			sprintf(sprintf_buffer, "Task_%d", i);
			todo[i] = MSG_task_create(sprintf_buffer, task_comp_size, task_comm_size, NULL);
		}
	}

	{                  
		/* Process organisation */
		alias_count = argc - 4;
		aliases = xbt_new0(char*,alias_count);

		for(i = 4; i < argc; i++) 
		{
			aliases[i-4] = strdup(argv[i]);
		}
	}

	INFO2("Got %d aliases and %d tasks to process", alias_count,number_of_tasks);
	
	for (i = 0; i < alias_count; i++)
		DEBUG1("%s", aliases[i]);

	for (i = 0; i < number_of_tasks; i++) 
	{
		INFO2("Sending \"%s\" to \"%s\"",todo[i]->name,aliases[i % alias_count]);
		
		if(!strcmp(MSG_host_get_name(MSG_host_self()), aliases[i % alias_count])) 
		{
			INFO0("Hey ! It's me ! :)");
		}

		MSG_task_send(todo[i], aliases[i % alias_count]);
		INFO0("Sent");
	}

	INFO0("All tasks have been dispatched. Let's tell everybody the computation is over.");
	
	for (i = 0; i < alias_count; i++) 
		MSG_task_send(MSG_task_create("finalize", 0, 0, FINALIZE),aliases[i]);

	INFO0("Goodbye now!");
	
	for(i = 0; i < alias_count; i++)
		free(aliases[i]);
		
	free(aliases);
	free(todo);
	return 0;
} /* end_of_master */

/** Receiver function  */
int slave(int argc, char *argv[])
{
	m_task_t task = NULL;
	int res;
	
	while(1) 
	{
		res = MSG_task_receive(&(task), MSG_host_get_name(MSG_host_self()));
		xbt_assert0(res == MSG_OK, "MSG_task_receive failed");
	
		INFO1("Received \"%s\"", MSG_task_get_name(task));
	
		if (!strcmp(MSG_task_get_name(task),"finalize")) 
		{
			MSG_task_destroy(task);
			break;
		}

		INFO1("Processing \"%s\"", MSG_task_get_name(task));
		MSG_task_execute(task);
		INFO1("\"%s\" done", MSG_task_get_name(task));
		MSG_task_destroy(task);
		task = NULL;
	}
	
	INFO0("I'm done. See you!");
	return 0;
} /* end_of_slave */

/** Forwarder function */
int forwarder(int argc, char *argv[])
{
	int i;
	int alias_count;
	char** aliases;
	
	{                  /* Process organisation */
		alias_count = argc - 1;
		aliases = xbt_new0(char*,alias_count);
	
		for (i = 1; i < argc; i++) 
			aliases[i-1] =strdup(argv[i]);
	}

	i=0;
	
	while(1) 
	{
		m_task_t task = NULL;
		int a;
		
		a = MSG_task_receive(&(task),MSG_host_get_name(MSG_host_self()));
		
		if (a == MSG_OK) 
		{
			INFO1("Received \"%s\"", MSG_task_get_name(task));
			
			if(MSG_task_get_data(task)==FINALIZE) 
			{
				INFO0("All tasks have been dispatched. Let's tell everybody the computation is over.");
				
				for (i = 0; i < alias_count; i++) 
					MSG_task_send(MSG_task_create("finalize", 0, 0, FINALIZE),aliases[i]);
				
				MSG_task_destroy(task);
				break;
			}
		
			INFO2("Sending \"%s\" to \"%s\"",MSG_task_get_name(task),aliases[i% alias_count]);
			MSG_task_send(task, aliases[i % alias_count]);
			i++;
		} 
		else 
		{
			INFO0("Hey ?! What's up ? ");
			xbt_assert0(0,"Unexpected behavior");
		}
	}
	
	for(i = 0; i < alias_count; i++)
		free(aliases[i]);

	INFO0("I'm done. See you!");
	return 0;
	
} /* end_of_forwarder */

/** Test function */
MSG_error_t test_all(const char *platform_file,const char *application_file)
{
	MSG_error_t res = MSG_OK;

	{	/*  Simulation setting */
		MSG_set_channel_number(MAX_CHANNEL);
		MSG_paje_output("msg_test.trace");
		MSG_create_environment(platform_file);
	}
	
	{
		/*   Application deployment */
		MSG_function_register("master", master);
		MSG_function_register("slave", slave);
		MSG_function_register("forwarder", forwarder);
		MSG_launch_application(application_file);
	}
	
	res = MSG_main();

	INFO1("Simulation time %g",MSG_get_clock());
	return res;
} /* end_of_test_all */


/** Main function */
int main(int argc, char *argv[])
{
	MSG_error_t res = MSG_OK;
	
	MSG_global_init(&argc,argv);
	
	if (argc < 3) 
	{
		printf ("Usage: %s platform_file deployment_file\n",argv[0]);
		printf ("example: %s msg_platform.xml msg_deployment.xml\n",argv[0]);
		exit(1);
	}
	
	res = test_all(argv[1],argv[2]);
	MSG_clean();
	
	if(res==MSG_OK)
		return 0;
	else
		return 1;
} /* end_of_main */
