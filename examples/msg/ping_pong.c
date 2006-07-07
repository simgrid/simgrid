/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg/msg.h" /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "xbt/sysdep.h" /* calloc, printf */


/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,"Messages specific for this msg example");

int sender(int argc, char *argv[]);
int receiver(int argc, char *argv[]);

MSG_error_t test_all(const char *platform_file, const char *application_file);

typedef enum 
  {
    PORT_22 = 0,
    MAX_CHANNEL
  } channel_t;

double task_comm_size_lat = 0;
double task_comm_size_bw = 100000000;

/** Emitter function  */
int sender(int argc,char *argv[] )
{INFO0("sender");
  int nbr = 0;
  m_host_t *hosts = NULL; 
  int i;
  double time;
  double task_comp_size = 0;
  m_task_t task=NULL;
  char sprintf_buffer[64];
 
  nbr = argc - 1;
 hosts = calloc(nbr, sizeof(m_host_t));
    
  for (i = 1; i < argc; i++) 
    {
 INFO2("host [%d]= %s",i-1, argv[i]);
      hosts[i-1] = MSG_get_host_by_name(argv[i]);
      if(hosts[i-1]==NULL) 
	{
	  INFO1("Unknown host %s. Stopping Now! ", argv[i]);
	  abort();
	}
    }

  for (i = 0; i < nbr; i++) 
    { 
      /* Latency */
      /* time=MSG_get_clock(); */
/*       task = MSG_task_create(sprintf_buffer, task_comp_size, task_comm_size_lat, NULL); */
/* INFO1("task = %p",task); */
/*       task->data=xbt_new0(double,1); */
/*       *(double*)(task->data)=time; */
/* INFO1("task->data = %le", *(double*)(task->data)); */
/* INFO2("host [%d]=%s ",i, argv[i+1]); */
/*       MSG_task_put(task, hosts[i],PORT_22);    */

      /* Bandwidth */
      time=MSG_get_clock();
      task = MSG_task_create(sprintf_buffer, task_comp_size, task_comm_size_bw, NULL);
      task->data=xbt_new0(double,1);
      *(double*)(task->data)=time;
      MSG_task_put(task, hosts[i % nbr],PORT_22);  
    }



  return 0;
} /* end_of_client */

/** Receiver function  */
int receiver(int argc, char *argv[])
{
 INFO0("receiver");
  double communication_time=0;
  while(1) 
    {
      double time, time1, sender_time;
      m_task_t task = NULL;
      int a;

      time=MSG_get_clock();
      a=MSG_task_get(&task,PORT_22);
      if (a == MSG_OK) 
	{
     
      time1=MSG_get_clock();
      sender_time=(*(double*)(task->data));
     
      if (sender_time > time) 
	time=sender_time;
      communication_time=time1-time;
      INFO1("Communic. time %le",communication_time);
      MSG_task_destroy(task);
      INFO1("Communic. time %le",communication_time);

      INFO1("--- bw %f ----",task_comm_size_bw/communication_time);
	}
      else 
	{
	  xbt_assert0(0,"Unexpected behavior");
	}
    } 
 return 0;
}/* end_of_receiver */


/** Test function */
MSG_error_t test_all(const char *platform_file,
			    const char *application_file)
{
  INFO0("test_all");
  MSG_error_t res = MSG_OK; 
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
} /* end_of_test_all */


/** Main function */
int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;

  MSG_global_init(&argc,argv);
  if (argc < 3) 
{
     printf ("Usage: %s platform_file deployment_file\n",argv[0]);
     printf ("example: %s msg_platform.xml msg_deployment.xml\n",argv[0]);
     exit(1);
  }
  res = test_all(argv[1],argv[2]);
  MSG_clean();

  if(res==MSG_OK) return 0; 
  else return 1;
} /* end_of_main */
