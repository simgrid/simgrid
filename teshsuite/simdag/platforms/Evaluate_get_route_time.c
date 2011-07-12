/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

//for i in $(seq 1 100); do teshsuite/simdag/platforms/evaluate_get_route_time ../examples/platforms/One_cluster.xml 1 2> /tmp/null ; done


#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"
#include "surf/surf_private.h"
#include <time.h>

#define BILLION  1000000000L;
extern routing_global_t global_routing;

int main(int argc, char **argv)
{
	SD_workstation_t w1, w2;
	const SD_workstation_t *workstations;
	int i, j;
	int list_size;
	struct timespec start, stop;
	double accum;

	/* initialisation of SD */
	SD_init(&argc, argv);

	/* creation of the environment */
	SD_create_environment(argv[1]);

	workstations = SD_workstation_get_list();
	list_size = SD_workstation_get_number();

	unsigned int seed;
	struct timespec time;
	clock_gettime( CLOCK_REALTIME, &time);
	seed = time.tv_nsec;

	srand(seed);

	do{
		i = rand()%list_size;
		j = rand()%list_size;
	}while(i==j);

	w1 = workstations[i];
	w2 = workstations[j];
	printf("%d\tand\t%d\t\t",i,j);

	if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) {
	perror( "clock gettime" );
	return EXIT_FAILURE;
	}

	SD_route_get_list(w1, w2);

	if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) {
	perror( "clock gettime" );
	return EXIT_FAILURE;
	}

	accum = ( stop.tv_sec - start.tv_sec )
	   + (double)( stop.tv_nsec - start.tv_nsec )
		 / (double)BILLION;
	printf("%lf\n", accum);

	SD_exit();

	return 0;
}
