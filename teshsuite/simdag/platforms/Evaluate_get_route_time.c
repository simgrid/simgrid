/* Copyright (c) 2008, 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

//for i in $(seq 1 100); do teshsuite/simdag/platforms/evaluate_get_route_time ../examples/platforms/One_cluster.xml 1 2> /tmp/null ; done


#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"
#include "surf/surf_private.h"
#include "xbt/xbt_os_time.h"

#define BILLION  1000000000L;
extern routing_global_t global_routing;

int main(int argc, char **argv)
{
	SD_workstation_t w1, w2;
	const SD_workstation_t *workstations;
	int i, j;
	int list_size;
	xbt_os_timer_t timer = xbt_os_timer_new();

	/* initialisation of SD */
	SD_init(&argc, argv);

	/* creation of the environment */
	SD_create_environment(argv[1]);

	workstations = SD_workstation_get_list();
	list_size = SD_workstation_get_number();

	/* Random number initialization */
	srand( (int) (xbt_os_time()*1000) );

	do{
		i = rand()%list_size;
		j = rand()%list_size;
	}while(i==j);

	w1 = workstations[i];
	w2 = workstations[j];
	printf("%d\tand\t%d\t\t",i,j);

	xbt_os_timer_start(timer);
	SD_route_get_list(w1, w2);
  xbt_os_timer_stop(timer);

	printf("%lf\n", xbt_os_timer_elapsed(timer) );

	SD_exit();

	return 0;
}
