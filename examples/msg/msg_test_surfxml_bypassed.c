/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg/msg.h"  /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "surf/surf_parse.h" /* to override surf_parse */
#include "surf/surfxml.h"    /* to hijack surf_parse_lex */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,"Messages specific for this msg example");

static int surf_parse_bypass(void)
{
/* <platform_description> */
  STag_platform_description();


/*   <cpu name="Cpu A" power="100.00" availability_file="trace_A.txt"/> */
  A_cpu_name = "Cpu A";
  A_cpu_power= "100.00";
  A_cpu_availability = "1.0";
  A_cpu_availability_file = "trace_A.txt";
  A_cpu_state = A_cpu_state_ON;
  A_cpu_state_file = NULL;
  A_cpu_interference_send = "1.0";
  A_cpu_interference_recv = "1.0";
  A_cpu_interference_send_recv = "1.0";
  A_cpu_max_outgoing_rate = "-1.0";

  STag_cpu_fun();
  ETag_cpu_fun();

/*   <cpu name="Cpu B" power="100.00" availability_file="trace_B.txt"/> */
  A_cpu_name = "Cpu B";
  A_cpu_power= "100.00";
  A_cpu_availability = "1.0";
  A_cpu_availability_file = "trace_B.txt";
  A_cpu_state = A_cpu_state_ON;
  A_cpu_state_file = NULL;
  A_cpu_interference_send = "1.0";
  A_cpu_interference_recv = "1.0";
  A_cpu_interference_send_recv = "1.0";
  A_cpu_max_outgoing_rate = "-1.0";

  STag_cpu_fun();
  ETag_cpu_fun();

/*   <network_link name="LinkA" bandwidth="10.0" latency="0.2"/> */
  A_network_link_name = "LinkA";
  A_network_link_bandwidth = "10.0";
  A_network_link_bandwidth_file = NULL;
  A_network_link_latency = "0.2";
  A_network_link_latency_file = NULL;
  A_network_link_state = A_network_link_state_ON;
  A_network_link_state_file = NULL;
  A_network_link_sharing_policy = A_network_link_sharing_policy_SHARED;
  STag_network_link();
  ETag_network_link();

/*   <route src="Cpu A" dst="Cpu B"><route_element name="LinkA"/></route> */
  A_route_src = "Cpu A";
  A_route_dst = "Cpu B";
  A_route_impact_on_src = "0.0";
  A_route_impact_on_dst = "0.0";
  A_route_impact_on_src_with_other_recv = "0.0";
  A_route_impact_on_dst_with_other_send = "0.0";

  STag_route();

  A_route_element_name = "LinkA";
  STag_route_element();
  ETag_route_element();

  ETag_route();

/*   <route src="Cpu B" dst="Cpu A"><route_element name="LinkA"/></route> */
  A_route_src = "Cpu B";
  A_route_dst = "Cpu A";
  A_route_impact_on_src = "0.0";
  A_route_impact_on_dst = "0.0";
  A_route_impact_on_src_with_other_recv = "0.0";
  A_route_impact_on_dst_with_other_send = "0.0";

  STag_route();

  A_route_element_name = "LinkA";
  STag_route_element();
  ETag_route_element();

  ETag_route();

/*   <process host="Cpu A" function="master"> */
  A_process_host = "Cpu A";
  A_process_function = "master";
  A_process_start_time = "-1.0";
  A_process_kill_time = "-1.0";
  STag_process();

/*      <argument value="20"/> */
  A_argument_value = "20";
  STag_argument();
  ETag_argument();

/*      <argument value="50000"/> */
  A_argument_value = "50000";
  STag_argument();
  ETag_argument();

/*      <argument value="10"/> */
  A_argument_value = "10";
  STag_argument();
  ETag_argument();

/*      <argument value="Cpu B"/> */
  A_argument_value = "Cpu B";
  STag_argument();
  ETag_argument();

/*   </process> */
  ETag_process();

/*   <process host="Cpu B" function="slave"/> */
  A_process_host = "Cpu B";
  A_process_function = "slave";
  A_process_start_time = "-1.0";
  A_process_kill_time = "-1.0";
  STag_process();
  ETag_process();

/* </platform_description> */
  ETag_platform_description();

  return 0;
}

int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);
void test_all(const char *platform_file, const char *application_file);

typedef enum {
  PORT_22 = 0,
  MAX_CHANNEL
} channel_t;

/* This function is just used so that users can check that each process
 *  has received the arguments it was supposed to receive.
 */
static void print_args(int argc, char** argv)
{
  int i ; 

  fprintf(stderr,"<");
  for(i=0; i<argc; i++) 
    fprintf(stderr,"%s ",argv[i]);
  fprintf(stderr,">\n");
}

/** Emitter function  */
int master(int argc, char *argv[])
{
  int slaves_count = 0;
  m_host_t *slaves = NULL;
  m_task_t *todo = NULL;
  int number_of_tasks = 0;
  double task_comp_size = 0;
  double task_comm_size = 0;


  int i;

  print_args(argc,argv);

  xbt_assert1(sscanf(argv[1],"%d", &number_of_tasks),
	 "Invalid argument %s\n",argv[1]);
  xbt_assert1(sscanf(argv[2],"%lg", &task_comp_size),
	 "Invalid argument %s\n",argv[2]);
  xbt_assert1(sscanf(argv[3],"%lg", &task_comm_size),
	 "Invalid argument %s\n",argv[3]);

  {                  /*  Task creation */
    char sprintf_buffer[64];

    todo = calloc(number_of_tasks, sizeof(m_task_t));

    for (i = 0; i < number_of_tasks; i++) {
      sprintf(sprintf_buffer, "Task_%d", i);
      todo[i] = MSG_task_create(sprintf_buffer, task_comp_size, task_comm_size, NULL);
    }
  }

  {                  /* Process organisation */
    slaves_count = argc - 4;
    slaves = calloc(slaves_count, sizeof(m_host_t));
    
    for (i = 4; i < argc; i++) {
      slaves[i-4] = MSG_get_host_by_name(argv[i]);
      if(slaves[i-4]==NULL) {
	INFO1("Unknown host %s. Stopping Now! ", argv[i]);
	abort();
      }
    }
  }

  INFO1("Got %d slave(s) :", slaves_count);
  for (i = 0; i < slaves_count; i++)
    INFO1("\t %s", slaves[i]->name);

  INFO1("Got %d task to process :", number_of_tasks);

  for (i = 0; i < number_of_tasks; i++)
    INFO1("\t\"%s\"", todo[i]->name);

  for (i = 0; i < number_of_tasks; i++) {
    INFO2("Sending \"%s\" to \"%s\"",
                  todo[i]->name,
                  slaves[i % slaves_count]->name);
    if(MSG_host_self()==slaves[i % slaves_count]) {
      INFO0("Hey ! It's me ! :)");
    }
    MSG_task_put(todo[i], slaves[i % slaves_count],
                 PORT_22);
    INFO0("Send completed");
  }
  
  INFO0("All tasks have been dispatched. Bye!");
  free(slaves);
  free(todo);
  return 0;
} /* end_of_master */

/** Receiver function  */
int slave(int argc, char *argv[])
{
  print_args(argc,argv);

  while(1) {
    m_task_t task = NULL;
    int a;
    a = MSG_task_get(&(task), PORT_22);
    if (a == MSG_OK) {
      INFO1("Received \"%s\" ", task->name);
      INFO1("Processing \"%s\" ", task->name);
      MSG_task_execute(task);
      INFO1("\"%s\" done ", task->name);
      MSG_task_destroy(task);
    } else {
      INFO0("Hey ?! What's up ? ");
      xbt_assert0(0,"Unexpected behaviour");
    }
  }
  INFO0("I'm done. See you!");
  return 0;
} /* end_of_slave */

/** Test function */
void test_all(void)
{

  /* MSG_config("surf_workstation_model","KCCFLN05"); */
  {				/*  Simulation setting */
    MSG_set_channel_number(MAX_CHANNEL);
    MSG_paje_output("msg_test.trace");
    surf_parse = surf_parse_bypass;
    MSG_create_environment(NULL);
  }
  {                            /*   Application deployment */
    MSG_function_register("master", master);
    MSG_function_register("slave", slave);
    MSG_function_register("forwarder", forwarder);
    MSG_launch_application(NULL);
  }
  MSG_main();
  
  INFO1("Simulation time %g",MSG_get_clock());
} /* end_of_test_all */


/** Main function */
int main(int argc, char *argv[])
{
  MSG_global_init(&argc,argv);
  test_all();
  MSG_clean();
  return (0);
} /* end_of_main */

