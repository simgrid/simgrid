
#include "simgrid/platf_generator.h"
#include "xbt.h"
#include "msg/msg.h"

#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(test_generation,
                             "Messages specific for this msg example");


#define FINALIZE ((void*)221297)        /* a magic number to tell people to stop working */

void promoter_1(context_node_t node);
void labeler_1(context_edge_t edge);
int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);

/** Promoter function
 * Just promote each node into a host, with fixed power
 */
void promoter_1(context_node_t node) {

  s_sg_platf_host_cbarg_t host_parameters;

  host_parameters.id = NULL;
  host_parameters.power_peak = xbt_dynar_new(sizeof(double), NULL);
  xbt_dynar_push_as(host_parameters.power_peak, double, 1000000.0);
  host_parameters.core_amount = 1;
  host_parameters.power_scale = 1;
  host_parameters.power_trace = NULL;
  host_parameters.initial_state = SURF_RESOURCE_ON;
  host_parameters.state_trace = NULL;
  host_parameters.coord = NULL;
  host_parameters.properties = NULL;

  platf_graph_promote_to_host(node, &host_parameters);

}

/** Labeler function
 * Set all links with the same bandwidth and latency
 */
void labeler_1(context_edge_t edge) {
  s_sg_platf_link_cbarg_t link_parameters;
  link_parameters.id = NULL;
  link_parameters.bandwidth = 1000000;
  link_parameters.bandwidth_trace = NULL;
  link_parameters.latency = 0.01;
  link_parameters.latency_trace = NULL;
  link_parameters.state = SURF_RESOURCE_ON;
  link_parameters.state_trace = NULL;
  link_parameters.policy = SURF_LINK_SHARED;
  link_parameters.properties = NULL;

  platf_graph_link_label(edge, &link_parameters);
}

/** Emitter function  */
int master(int argc, char *argv[])
{
  int slaves_count = 0;
  msg_host_t *slaves = NULL;
  int number_of_tasks = 0;
  double task_comp_size = 0;
  double task_comm_size = 0;
  int i;

  number_of_tasks = 3*argc;
  task_comp_size = 2400000*argc;
  task_comm_size = 1000000;

  {                             /* Process organisation */
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

    a = MSG_task_send(task,MSG_host_get_name(slaves[i % slaves_count]));

    if (a == MSG_OK) {
      XBT_INFO("Send completed");
    } else {
      XBT_INFO("Hey ?! What's up ? ");
      xbt_die( "Unexpected behavior");
    }
  }

  XBT_INFO
      ("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (i = 0; i < slaves_count; i++) {
    msg_task_t task = MSG_task_create("finalize", 0, 0, FINALIZE);
    int a = MSG_task_send(task,MSG_host_get_name(slaves[i]));
    if (a != MSG_OK) {
      XBT_INFO("Hey ?! What's up ? ");
      xbt_die("Unexpected behavior with '%s': %d", MSG_host_get_name(slaves[i]), a);
    }
  }

  XBT_INFO("Goodbye now!");
  free(slaves);
  return 0;
}                               /* end_of_master */

/** Receiver function  */
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
      } else {
        XBT_INFO("Hey ?! What's up ? ");
        xbt_die("Unexpected behavior");
      }
    } else {
      XBT_INFO("Hey ?! What's up ? error %d", a);
      xbt_die("Unexpected behavior");
    }
  }
  XBT_INFO("I'm done. See you!");
  return 0;
}

/**
 * Main function
 * Create the platform, list the available hosts and give them some work
 */
int main(int argc, char **argv) {

  unsigned long seed[] = {134, 233445, 865, 2634, 424242, 876543};
  int connected;
  int max_tries = 10;

  //MSG initialisation
  MSG_init(&argc, argv);

  //Set up the seed for the platform generation
  platf_random_seed(seed);

  XBT_INFO("creating nodes...");
  platf_graph_uniform(50);
  do {
    max_tries--;
    XBT_INFO("creating links...");
    platf_graph_clear_links();
    platf_graph_interconnect_uniform(0.07); //Unrealistic, but simple
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

  XBT_INFO("protmoting...");
  platf_do_promote();

  XBT_INFO("labeling...");
  platf_do_label();

  XBT_INFO("Putting it in surf...");
  platf_generate();

  XBT_INFO("Let's get the available hosts and dispatch work:");

  unsigned int i;
  msg_host_t host = NULL;
  msg_host_t host_master = NULL;
  xbt_dynar_t host_dynar = MSG_hosts_as_dynar();
  char** hostname_list = xbt_malloc(sizeof(char*) * xbt_dynar_length(host_dynar));

  xbt_dynar_foreach(host_dynar, i, host) {
    MSG_process_create("slave", slave, NULL, host);
    if(i==0) {
      //The first node will also be the master
      XBT_INFO("%s will be the master", MSG_host_get_name(host));
      host_master = host;
    }
    hostname_list[i] = (char*) MSG_host_get_name(host);
  }

  MSG_process_create_with_arguments("master", master, NULL, host_master, xbt_dynar_length(host_dynar), hostname_list);

  XBT_INFO("Let's rock!");
  MSG_main();
  XBT_INFO("Simulation time %g", MSG_get_clock());
  return 0;
}
