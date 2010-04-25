/* Copyright (c) 2006, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras.h"

/* Global private data */
typedef struct{
	gras_socket_t sock; /* server socket on which I'm listening */
	int xId;
	int yId;
	char host[1024]; /* my host name */
	int port; /* port on which I'm listening FIXME */

	int x1; // Xmin
	int x2; // Xmax
	int y1; // Ymin
	int y2; // Ymax

	char north_host[1024];
	int north_port;
	char south_host[1024];
	int south_port;
	char east_host[1024];
	int east_port;
	char west_host[1024];
	int west_port;

        int version;
}node_data_t;
