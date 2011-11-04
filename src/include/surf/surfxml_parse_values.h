/* Copyright (c) 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURFXML_PARSE_VALUES_H_
#define SURFXML_PARSE_VALUES_H_

typedef struct s_surf_parsing_peer_arg *surf_parsing_peer_arg_t;
typedef struct s_surf_parsing_peer_arg {
	char* V_peer_id;
	char* V_peer_power;
	char* V_peer_bw_in;
	char* V_peer_bw_out;
	char* V_peer_lat;
	char* V_peer_coord;
	char* V_peer_availability_trace;
	char* V_peer_state_trace;
} s_surf_parsing_peer_arg_t;

typedef struct s_surf_parsing_link_arg *surf_parsing_link_arg_t;
typedef struct s_surf_parsing_link_arg {
	char* V_link_id;
	double V_link_bandwidth;
	tmgr_trace_t V_link_bandwidth_file;
	double V_link_latency;
	tmgr_trace_t V_link_latency_file;
	e_surf_resource_state_t V_link_state;
	tmgr_trace_t V_link_state_file;
	int V_link_sharing_policy;
	int V_policy_initial_link;
} s_surf_parsing_link_arg_t;

typedef struct s_surf_parsing_cluster_arg *surf_parsing_cluster_arg_t;
typedef struct s_surf_parsing_cluster_arg {
	char* V_cluster_id;
	char* V_cluster_prefix;
	char* V_cluster_suffix;
	char* V_cluster_radical;
	double S_cluster_power;
	int S_cluster_core;
	double S_cluster_bw;
	double S_cluster_lat;
	double S_cluster_bb_bw;
	double S_cluster_bb_lat;
	char* S_cluster_router_id;
	int V_cluster_sharing_policy;
	int V_cluster_bb_sharing_policy;
	char* V_cluster_availability_file;
	char* V_cluster_state_file;
} s_surf_parsing_cluster_arg_t;

typedef struct s_surf_parsing_router_arg *surf_parsing_router_arg_t;
typedef struct s_surf_parsing_router_arg {
	char* V_router_id;
	char* V_router_coord;
} s_surf_parsing_router_arg_t;

typedef struct s_surf_parsing_host_arg *surf_parsing_host_arg_t;
typedef struct s_surf_parsing_host_arg {
	char* V_host_id;													//id
	double V_host_power_peak;											//power
	int V_host_core;													//core
	double V_host_power_scale;											//availability
	tmgr_trace_t V_host_power_trace;									//availability file
	e_surf_resource_state_t V_host_state_initial;						//state
	tmgr_trace_t V_host_state_trace;									//state file
	const char* V_host_coord;
} s_surf_parsing_host_arg_t;

extern surf_parsing_host_arg_t struct_host;
extern surf_parsing_router_arg_t struct_router;
extern surf_parsing_cluster_arg_t struct_cluster;
extern surf_parsing_peer_arg_t struct_peer;
extern surf_parsing_link_arg_t struct_lnk;

void surf_parse_host(void);
void surf_parse_link(void);


#endif /* SURFXML_PARSE_VALUES_H_ */
