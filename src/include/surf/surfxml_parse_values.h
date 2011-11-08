/* Copyright (c) 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURFXML_PARSE_VALUES_H_
#define SURFXML_PARSE_VALUES_H_


typedef struct s_surf_parsing_cluster_arg *surf_parsing_cluster_arg_t;
typedef struct s_surf_parsing_cluster_arg {
	const char* id;
	const char* prefix;
	const char* suffix;
	const char* radical;
	double power;
	int core_amount;
	double bw;
	double lat;
	double bb_bw;
	double bb_lat;
	const char* router_id;
	int sharing_policy;
	int bb_sharing_policy;
	const char* availability_trace; //FIXME: convert to tmgr
	const char* state_trace;
} s_surf_parsing_cluster_arg_t;

typedef struct s_surf_parsing_link_up_down *surf_parsing_link_up_down_t;
typedef struct s_surf_parsing_link_up_down {
	void* link_up;
	void* link_down;
} s_surf_parsing_link_up_down_t;


extern surf_parsing_cluster_arg_t struct_cluster;

#endif /* SURFXML_PARSE_VALUES_H_ */
