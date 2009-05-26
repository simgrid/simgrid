/* 	$Id$	    */
/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */
/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include<stdio.h>

#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "xbt/sysdep.h"         /* calloc */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

int sender(int argc, char *argv[]);
int receiver(int argc, char *argv[]);

MSG_error_t test_all(const char *platform_file, const char *application_file);

typedef enum {
  PORT_22 = 0,
  MAX_CHANNEL
} channel_t;

double task_comm_size_lat = 10e0;
double task_comm_size_bw = 10e8;

/** Emitter function  */
int sender(int argc, char *argv[])
{
  m_host_t host = NULL;
  double time;
  m_task_t task_la = NULL;
  m_task_t task_bw = NULL;
  char sprintf_buffer_la[64];
  char sprintf_buffer_bw[64];

  INFO0("sender");

  /*host = xbt_new0(m_host_t,1); */

  INFO1("host = %s", argv[1]);

  host = MSG_get_host_by_name(argv[1]);

  if (host == NULL) {
    INFO1("Unknown host %s. Stopping Now! ", argv[1]);
    abort();
  }

  /* Latency */
  time = MSG_get_clock();
  sprintf(sprintf_buffer_la, "latency task");
  task_la = MSG_task_create(sprintf_buffer_la, 0.0, task_comm_size_lat, NULL);
  task_la->data = xbt_new(double, 1);
  *(double *) task_la->data = time;
  INFO1("task_la->data = %le", *((double *) task_la->data));
  MSG_task_put(task_la, host, PORT_22);

  /* Bandwidth */
  time = MSG_get_clock();
  sprintf(sprintf_buffer_bw, "bandwidth task");
  task_bw = MSG_task_create(sprintf_buffer_bw, 0.0, task_comm_size_bw, NULL);
  task_bw->data = xbt_new(double, 1);
  *(double *) task_bw->data = time;
  INFO1("task_bw->data = %le", *((double *) task_bw->data));
  MSG_task_put(task_bw, host, PORT_22);

  return 0;
}                               /* end_of_client */

/** Receiver function  */
int receiver(int argc, char *argv[])
{
  double time, time1, sender_time;
  m_task_t task_la = NULL;
  m_task_t task_bw = NULL;
  int a;
  double communication_time = 0;

  INFO0("receiver");

  time = MSG_get_clock();

  /* Get Latency */
  a = MSG_task_get(&task_la, PORT_22);
  if (a == MSG_OK) {
    time1 = MSG_get_clock();
    sender_time = *((double *) (task_la->data));
    time = sender_time;
    communication_time = time1 - time;
    INFO1("Task received : %s", task_la->name);
    MSG_task_destroy(task_la);
    INFO1("Communic. time %le", communication_time);
    INFO1("--- la %f ----", communication_time);
  } else {
    xbt_assert0(0, "Unexpected behavior");
  }


  /* Get Bandwidth */
  a = MSG_task_get(&task_bw, PORT_22);
  if (a == MSG_OK) {
    time1 = MSG_get_clock();
    sender_time = *((double *) (task_bw->data));
    time = sender_time;
    communication_time = time1 - time;
    INFO1("Task received : %s", task_bw->name);
    MSG_task_destroy(task_bw);
    INFO1("Communic. time %le", communication_time);
    INFO1("--- bw %f ----", task_comm_size_bw / communication_time);
  } else {
    xbt_assert0(0, "Unexpected behavior");
  }


  return 0;
}                               /* end_of_receiver */


/** Test function */
MSG_error_t test_all(const char *platform_file, const char *application_file)
{

  MSG_error_t res = MSG_OK;



  INFO0("test_all");

  /*  Simulation setting */
  MSG_set_channel_number(MAX_CHANNEL);
  MSG_paje_output("msg_test.trace");
  MSG_create_environment(platform_file);

  /*   Application deployment */
  MSG_function_register("sender", sender);
  MSG_function_register("receiver", receiver);

  MSG_launch_application(application_file);

  res = MSG_main();

  return res;
}                               /* end_of_test_all */


/** Main function */
int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;

#ifdef _MSC_VER
  unsigned int prev_exponent_format = _set_output_format(_TWO_DIGIT_EXPONENT);
#endif

  MSG_global_init(&argc, argv);


  if (argc != 3) {
    CRITICAL1("Usage: %s platform_file deployment_file <model>\n", argv[0]);
    CRITICAL1
      ("example: %s msg_platform.xml msg_deployment.xml KCCFLN05_Vegas\n",
       argv[0]);
    exit(1);
  }

  /* Options for the workstation_model:

     KCCFLN05              => for maxmin
     KCCFLN05_proportional => for proportional (Vegas)
     KCCFLN05_Vegas        => for TCP Vegas
     KCCFLN05_Reno         => for TCP Reno
   */
  //MSG_config("workstation_model", argv[3]);

  res = test_all(argv[1], argv[2]);

  INFO1("Total simulation time: %le", MSG_get_clock());

  MSG_clean();

#ifdef _MSC_VER
  _set_output_format(prev_exponent_format);
#endif

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
