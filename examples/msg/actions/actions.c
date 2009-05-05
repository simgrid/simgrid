/* 	$Id$	 */

/* Copyright (c) 2009. The SimGrid team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "msg/msg.h" /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "xbt.h" /* calloc, printf */

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,"Messages specific for this msg example");

/* My actions */
static void send(xbt_dynar_t action) {
	INFO1("Send: %s",xbt_str_join(action," "));
}
static void recv(xbt_dynar_t action) {
	INFO1("Recv: %s",xbt_str_join(action," "));
}
static void sleep(xbt_dynar_t action) {
	INFO1("Recv: %s",xbt_str_join(action," "));
}
static void compute(xbt_dynar_t action) {
	INFO1("compute: %s",xbt_str_join(action," "));
}

/** Main function */
int main(int argc, char *argv[]){
	MSG_error_t res = MSG_OK;

	/* Check the given arguments */
	MSG_global_init(&argc,argv);
	if (argc < 4) {
		printf ("Usage: %s platform_file deployment_file action_files\n",argv[0]);
		printf ("example: %s msg_platform.xml msg_deployment.xml actions\n",argv[0]);
		exit(1);
	}

	/*  Simulation setting */
	MSG_create_environment(argv[1]);

	/* No need to register functions as in classical MSG programs: the actions get started anyway */
	MSG_launch_application(argv[2]);

	/*   Action registration */
	MSG_action_register("send", send);
	MSG_action_register("recv", recv);
	MSG_action_register("sleep", sleep);
	MSG_action_register("compute", sleep);

	/* Actually do the simulation using MSG_action_trace_run */
	res = MSG_action_trace_run(argv[3]);

	INFO1("Simulation time %g",MSG_get_clock());
	MSG_clean();

	if(res==MSG_OK)
		return 0;
	else
		return 1;
} /* end_of_main */
