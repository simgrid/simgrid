/*
 * surfxml_parse_values.h
 *
 *  Created on: 24 oct. 2011
 *      Author: navarrop
 */

#ifndef SURFXML_PARSE_VALUES_H_
#define SURFXML_PARSE_VALUES_H_

struct V_peer {
	char* V_peer_id;
	double V_peer_power;
	double V_peer_bw_in;
	double V_peer_bw_out;
	double V_peer_lat;
	char* V_peer_coord;
	tmgr_trace_t V_peer_availability_trace;
	tmgr_trace_t V_peer_state_trace;
};

struct V_link {
	char* V_link_id;
	double V_link_bandwidth;
	tmgr_trace_t V_link_bandwidth_file;
	double V_link_latency;
	tmgr_trace_t V_link_latency_file;
	e_surf_resource_state_t V_link_state;
	tmgr_trace_t V_link_state_file;
	int V_link_sharing_policy;
};

struct V_cluster {
	char* V_cluster_id;
	char* V_cluster_prefix;
	char* V_cluster_suffix;
	char* V_cluster_radical;
	double V_cluster_power;
	int V_cluster_core;
	double V_cluster_bw;
	double V_cluster_lat;
	double V_cluster_bb_bw;
	double V_cluster_bb_lat;
	char * V_cluster_router_id;
	int V_cluster_sharing_policy;
	int V_cluster_bb_sharing_policy;
};

struct V_router {
	char* V_router_id;
	char* V_router_coord;
};

struct V_host {
	char* V_host_id;													//id
	double V_host_power_peak;											//power
	int V_host_core;													//core
	double V_host_power_scale;											//availability
	tmgr_trace_t V_host_power_trace;									//availability file
	e_surf_resource_state_t V_host_state_initial;						//state
	tmgr_trace_t V_host_state_trace;									//state file
	char* V_host_coord;
};

struct V_host s_host;
struct V_router s_router;
struct V_cluster s_cluster;
struct V_peer s_peer;
struct V_link s_link;

#endif /* SURFXML_PARSE_VALUES_H_ */
