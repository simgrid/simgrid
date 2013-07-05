/* Copyright (c) 2012. The SimGrid Team.
 * All rights reserved.                                                     */
/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "bittorrent.h"
#include "peer.h"
#include "tracker.h"
#include <msg/msg.h>
#include <simgrid/platf_generator.h>
/**
 * Bittorrent example launcher, using a generated platform
 */

static RngStream rng_stream;

void promoter(context_node_t node);
void labeler(context_edge_t edge);
void create_environment(int node_count);
void dispatch_jobs(double tracker_deadline, double peer_deadline,
                   double seed_percentage);

void promoter(context_node_t node)
{
  s_sg_platf_host_cbarg_t host_parameters;

  if (node->degree == 1) {
    //We promote only the leaf; as we use a star topology, all the nodes
    //will be promoted except the first one, which will be a router with
    //every hosts connected on.
    host_parameters.id = NULL;

    //Power from 3,000,000 to 10,000,000
    host_parameters.power_peak = xbt_dynar_new(sizeof(double), NULL);
    xbt_dynar_push_as(host_parameters.power_peak, double,
    		7000000 * RngStream_RandU01(rng_stream) + 3000000.0);
    host_parameters.core_amount = 1;
    host_parameters.power_scale = 1;
    host_parameters.power_trace = NULL;
    host_parameters.initial_state = SURF_RESOURCE_ON;
    host_parameters.state_trace = NULL;
    host_parameters.coord = NULL;
    host_parameters.properties = NULL;

    platf_graph_promote_to_host(node, &host_parameters);
  }
}

void labeler(context_edge_t edge)
{

  s_sg_platf_link_cbarg_t link_parameters;
  link_parameters.id = NULL;

  //bandwidth from 3,000,000 to 10,000,000
  link_parameters.bandwidth = 7000000 * RngStream_RandU01(rng_stream) + 3000000;
  link_parameters.bandwidth_trace = NULL;

  //Latency from 0ms to 100ms
  link_parameters.latency = RngStream_RandU01(rng_stream) / 10.0;
  link_parameters.latency_trace = NULL;
  link_parameters.state = SURF_RESOURCE_ON;
  link_parameters.state_trace = NULL;
  link_parameters.policy = SURF_LINK_SHARED;
  link_parameters.properties = NULL;

  platf_graph_link_label(edge, &link_parameters);
}

void create_environment(int node_count)
{

  platf_graph_uniform(node_count);

  //every nodes are connected to the first one
  platf_graph_interconnect_star();
  //No need to check if the graph is connected, the star topology implies it.

  //register promoter and labeler
  platf_graph_promoter(promoter);
  platf_graph_labeler(labeler);

  //promoting and labeling
  platf_do_promote();
  platf_do_label();

  //Put the platform into the simulator
  platf_generate();
}

void dispatch_jobs(double tracker_deadline, double peer_deadline,
                   double seed_percentage)
{

  xbt_dynar_t available_nodes = MSG_hosts_as_dynar();
  msg_host_t host;
  unsigned int i;

  char **arguments_tracker;
  char **arguments_peer;

  unsigned int seed_count =
      (seed_percentage / 100.0) * xbt_dynar_length(available_nodes);

  xbt_dynar_foreach(available_nodes, i, host) {
    if (i == 0) {
      //The fisrt node is the tracker
      arguments_tracker = xbt_malloc0(sizeof(char *) * 2);
      arguments_tracker[0] = xbt_strdup("tracker");
      arguments_tracker[1] = bprintf("%f", tracker_deadline);
      MSG_process_create_with_arguments("tracker", tracker, NULL, host, 2,
                                        arguments_tracker);
    } else {
      //Other nodes are peers
      int argument_size;
      arguments_peer = xbt_malloc0(sizeof(char *) * 4);
      arguments_peer[0] = xbt_strdup("peer");
      arguments_peer[1] = bprintf("%d", i);
      arguments_peer[2] = bprintf("%f", peer_deadline);

      //The first peers will be seeders
      if (seed_count > 0) {
        seed_count--;
        arguments_peer[3] = xbt_strdup("1");
        argument_size = 4;
      } else {
        //Other ars leechers
        arguments_peer[3] = NULL;
        argument_size = 3;
      }
      MSG_process_create_with_arguments("peer", peer, NULL, host,
                                        argument_size, arguments_peer);
    }
  }
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);

  rng_stream = RngStream_CreateStream(NULL);

  //Maybe these parameters should be set from the command line...
  //create_environment(<node_count>)
  create_environment(20);

  //dispatch_jobs(<tracker_deadline>, <peer_deadline>, <seed_percentage>)
  dispatch_jobs(2000, 2000, 10);

  MSG_main();

  return 0;
}
