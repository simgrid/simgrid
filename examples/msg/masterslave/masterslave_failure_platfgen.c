/* Copyright (c) 2007-2010, 2012-2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "xbt/sysdep.h"         /* calloc, printf */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"

/* we are going to use a generated platform */
#include "simgrid/platf_generator.h"
#include "simgrid/platf_interface.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);
void promoter_1(context_node_t node);
void labeler_1(context_edge_t edge);

#define FINALIZE ((void*)221297)        /* a magic number to tell people to stop working */

#define TASK_COUNT_PER_HOST 5           /* Number of tasks each slave has to do */
#define TASK_COMP_SIZE 15000000         /* computation cost of each task */
#define TASK_COMM_SIZE 1200000          /* communication time of task */

/** Promoter function
 * Just promote each node into a host, with fixed power
 * Set one node as master
 * Add a state trace on other nodes
 */
void promoter_1(context_node_t node) {

  s_sg_platf_host_cbarg_t host_parameters;
  static int master_choosen = FALSE;

  host_parameters.id = NULL;
  host_parameters.power_peak = xbt_dynar_new(sizeof(double), NULL);
  xbt_dynar_push_as(host_parameters.power_peak, double, 25000000.0);
  host_parameters.core_amount = 1;
  host_parameters.power_scale = 1;
  host_parameters.power_trace = NULL;
  host_parameters.initial_state = SURF_RESOURCE_ON;
  host_parameters.state_trace = NULL;
  host_parameters.coord = NULL;
  host_parameters.properties = NULL;

  if(!master_choosen) {
    master_choosen = TRUE;
    host_parameters.id = "host_master";
  } else {
    //Set a power trace
    char* pw_date_generator_id = bprintf("pw_date_host_%ld", node->id);
    char* pw_value_generator_id = bprintf("pw_value_host_%ld", node->id);
    probabilist_event_generator_t pw_date_generator =
                          tmgr_event_generator_new_uniform(pw_date_generator_id, 5, 10);
    probabilist_event_generator_t pw_value_generator =
                          tmgr_event_generator_new_uniform(pw_value_generator_id, 0.6, 1.0);
    host_parameters.power_trace =
              tmgr_trace_generator_value(bprintf("pw_host_%ld", node->id),
                                                pw_date_generator,
                                                pw_value_generator);
  }

  platf_graph_promote_to_host(node, &host_parameters);

}

/** Labeler function
 * Set all links with the same bandwidth and latency
 * Add a state trace to each link
 */
void labeler_1(context_edge_t edge) {
  s_sg_platf_link_cbarg_t link_parameters;
  link_parameters.id = NULL;
  link_parameters.bandwidth = 100000000;
  link_parameters.bandwidth_trace = NULL;
  link_parameters.latency = 0.01;
  link_parameters.latency_trace = NULL;
  link_parameters.state = SURF_RESOURCE_ON;
  link_parameters.state_trace = NULL;
  link_parameters.policy = SURF_LINK_SHARED;
  link_parameters.properties = NULL;

  char* avail_generator_id = bprintf("avail_link_%ld", edge->id);
  char* unavail_generator_id = bprintf("unavail_link_%ld", edge->id);

  probabilist_event_generator_t avail_generator =
                          tmgr_event_generator_new_exponential(avail_generator_id, 0.0001);
  probabilist_event_generator_t unavail_generator =
                          tmgr_event_generator_new_uniform(unavail_generator_id, 10, 20);

  link_parameters.state_trace =
              tmgr_trace_generator_avail_unavail(bprintf("state_link_%ld", edge->id),
                                                avail_generator,
                                                unavail_generator,
                                                SURF_RESOURCE_ON);


  platf_graph_link_label(edge, &link_parameters);

}

int master(int argc, char *argv[])
{
  int slaves_count = 0;
  msg_host_t *slaves = NULL;
  int number_of_tasks = 0;
  double task_comp_size = 0;
  double task_comm_size = 0;
  int i;

  number_of_tasks = TASK_COUNT_PER_HOST*argc;
  task_comp_size = TASK_COMP_SIZE;
  task_comm_size = TASK_COMM_SIZE;

  {                             /* Process organization */
    slaves_count = argc;
    slaves = xbt_new0(msg_host_t, slaves_count);

    for (i = 0; i < argc; i++) {
      slaves[i] = MSG_get_host_by_name(argv[i]);
      if (slaves[i] == NULL) {
        XBT_INFO("Unknown host %s. Stopping Now! ", argv[i]);
        abort();
      }
    }
  }

  XBT_INFO("Got %d slave(s) :", slaves_count);
  for (i = 0; i < slaves_count; i++)
    XBT_INFO("%s", MSG_host_get_name(slaves[i]));

  XBT_INFO("Got %d task to process :", number_of_tasks);

  for (i = 0; i < number_of_tasks; i++) {
    msg_task_t task = MSG_task_create("Task", task_comp_size, task_comm_size,
                                    xbt_new0(double, 1));
    int a;
    *((double *) task->data) = MSG_get_clock();

    a = MSG_task_send_with_timeout(task,MSG_host_get_name(slaves[i % slaves_count]),10.0);

    if (a == MSG_OK) {
      XBT_INFO("Send completed");
    } else if (a == MSG_HOST_FAILURE) {
      XBT_INFO
          ("Gloups. The cpu on which I'm running just turned off!. See you!");
      free(task->data);
      MSG_task_destroy(task);
      free(slaves);
      return 0;
    } else if (a == MSG_TRANSFER_FAILURE) {
      XBT_INFO
          ("Mmh. Something went wrong with '%s'. Nevermind. Let's keep going!",
              MSG_host_get_name(slaves[i % slaves_count]));
      free(task->data);
      MSG_task_destroy(task);
    } else if (a == MSG_TIMEOUT) {
      XBT_INFO
          ("Mmh. Got timeouted while speaking to '%s'. Nevermind. Let's keep going!",
              MSG_host_get_name(slaves[i % slaves_count]));
      free(task->data);
      MSG_task_destroy(task);
    } else {
      XBT_INFO("Hey ?! What's up ? ");
      xbt_die( "Unexpected behavior");
    }
  }

  XBT_INFO
      ("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (i = 0; i < slaves_count; i++) {
    msg_task_t task = MSG_task_create("finalize", 0, 0, FINALIZE);
    int a = MSG_task_send_with_timeout(task,MSG_host_get_name(slaves[i]),1.0);
    if (a == MSG_OK)
      continue;
    if (a == MSG_HOST_FAILURE) {
      XBT_INFO
          ("Gloups. The cpu on which I'm running just turned off!. See you!");
      MSG_task_destroy(task);
      free(slaves);
      return 0;
    } else if (a == MSG_TRANSFER_FAILURE) {
      XBT_INFO("Mmh. Can't reach '%s'! Nevermind. Let's keep going!",
          MSG_host_get_name(slaves[i]));
      MSG_task_destroy(task);
    } else if (a == MSG_TIMEOUT) {
      XBT_INFO
          ("Mmh. Got timeouted while speaking to '%s'. Nevermind. Let's keep going!",
              MSG_host_get_name(slaves[i % slaves_count]));
      MSG_task_destroy(task);
    } else {
      XBT_INFO("Hey ?! What's up ? ");
      xbt_die("Unexpected behavior with '%s': %d", MSG_host_get_name(slaves[i]), a);
    }
  }

  XBT_INFO("Goodbye now!");
  free(slaves);
  return 0;
}                               /* end_of_master */

int slave(int argc, char *argv[])
{
  while (1) {
    msg_task_t task = NULL;
    int a;
    double time1, time2;

    time1 = MSG_get_clock();
    a = MSG_task_receive( &(task), MSG_host_get_name(MSG_host_self()) );
    time2 = MSG_get_clock();
    if (a == MSG_OK) {
      XBT_INFO("Received \"%s\"", MSG_task_get_name(task));
      if (MSG_task_get_data(task) == FINALIZE) {
        MSG_task_destroy(task);
        break;
      }
      if (time1 < *((double *) task->data))
        time1 = *((double *) task->data);
      XBT_INFO("Communication time : \"%f\"", time2 - time1);
      XBT_INFO("Processing \"%s\"", MSG_task_get_name(task));
      a = MSG_task_execute(task);
      if (a == MSG_OK) {
        XBT_INFO("\"%s\" done", MSG_task_get_name(task));
        free(task->data);
        MSG_task_destroy(task);
      } else if (a == MSG_HOST_FAILURE) {
        XBT_INFO
            ("Gloups. The cpu on which I'm running just turned off!. See you!");
        return 0;
      } else {
        XBT_INFO("Hey ?! What's up ? ");
        xbt_die("Unexpected behavior");
      }
    } else if (a == MSG_HOST_FAILURE) {
      XBT_INFO
          ("Gloups. The cpu on which I'm running just turned off!. See you!");
      return 0;
    } else if (a == MSG_TRANSFER_FAILURE) {
      XBT_INFO("Mmh. Something went wrong. Nevermind. Let's keep going!");
    } else if (a == MSG_TIMEOUT) {
      XBT_INFO("Mmh. Got a timeout. Nevermind. Let's keep going!");
    } else {
      XBT_INFO("Hey ?! What's up ? ");
      xbt_die("Unexpected behavior");
    }
  }
  XBT_INFO("I'm done. See you!");
  return 0;
}                               /* end_of_slave */

/** Main function */
int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;
  unsigned long seed_platf_gen[] = {134, 233445, 865, 2634, 424242, 876543};
  unsigned long seed_trace_gen[] = {8865244, 356772, 42, 77465, 2098754, 8725442};
  int connected;
  int max_tries = 10;

  //MSG initialization
  MSG_init(&argc, argv);

  //Set up the seed for the platform generation
  platf_random_seed(seed_platf_gen);

  //Set up the RngStream for trace generation
  sg_platf_rng_stream_init(seed_trace_gen);

  XBT_INFO("creating nodes...");
  platf_graph_uniform(10);

  do {
    max_tries--;
    XBT_INFO("creating links...");
    platf_graph_clear_links();
    platf_graph_interconnect_waxman(0.9, 0.4);
    XBT_INFO("done. Check connectedness...");
    connected = platf_graph_is_connected();
    XBT_INFO("Is it connected : %s", connected ? "yes" : (max_tries ? "no, retrying" : "no"));
  } while(!connected && max_tries);

  if(!connected && !max_tries) {
    xbt_die("Impossible to connect the graph, aborting.");
  }

  XBT_INFO("registering callbacks...");
  platf_graph_promoter(promoter_1);
  platf_graph_labeler(labeler_1);

  XBT_INFO("promoting...");
  platf_do_promote();

  XBT_INFO("labeling...");
  platf_do_label();

  XBT_INFO("Putting it in surf...");
  platf_generate();

  XBT_INFO("Let's get the available hosts and dispatch work:");

  unsigned int i;
  msg_host_t host = NULL;
  msg_host_t host_master = NULL;
  msg_process_t process = NULL;
  xbt_dynar_t host_dynar = MSG_hosts_as_dynar();
  char** hostname_list =
    xbt_malloc(sizeof(char*) * xbt_dynar_length(host_dynar));

  xbt_dynar_foreach(host_dynar, i, host) {
    process = MSG_process_create("slave", slave, NULL, host);
    MSG_process_auto_restart_set(process, TRUE);
    hostname_list[i] = (char*) MSG_host_get_name(host);
  }
  host_master = MSG_get_host_by_name("host_master");
  MSG_process_create_with_arguments("master", master, NULL, host_master,
                                    xbt_dynar_length(host_dynar),
                                    hostname_list);

  res = MSG_main();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
