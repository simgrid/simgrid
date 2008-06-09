#include <stdio.h>
#include <stdlib.h>
#include "msg/msg.h"
/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,"Messages specific for this msg example");

int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);
MSG_error_t test_all(const char *platform_file, const char *application_file);


typedef enum {
  PORT_22 = 0,
  MAX_CHANNEL
} channel_t;


double time=0;
double task_comm_size=0.0;

#define FINALIZE ((void*)221297) /* a magic number to tell people to stop working */

/** Master */
int master(int argc, char *argv[]){
  char *slavename = NULL;
  m_task_t todo = NULL;
  m_host_t slave;
  MSG_error_t a;
  
  if(argc != 3){
    xbt_assert1(0, "Invalid number of arguments in master process, expected 4 got %d\n", argc);
  }

  /* data size */
  xbt_assert1(sscanf(argv[1],"%lg", &task_comm_size),
      "Invalid argument %s\n", argv[1]);
  INFO1("task_comm_size : %f\n", task_comm_size);
 
  /* slave name */
  slavename = argv[2];


  /*  Task creation.  */
  char sprintf_buffer[64] = "Task_0";
  todo = MSG_task_create(sprintf_buffer, 0, task_comm_size, NULL);
  
  /* Process organisation */
  slave = MSG_get_host_by_name(slavename);
    
  /* Time measurement */
  time = MSG_get_clock();

  a = MSG_task_put(todo, slave, PORT_22);
  xbt_assert0(!a,"MSG_task_put failure"); 

  return 0;
} /* end_of_master */


/** Receiver function  */
int slave(int argc, char *argv[])
{ 
  m_task_t task = NULL;
  int a;

  a = MSG_task_get(&(task), PORT_22);
  xbt_assert0(!a, "MSG_task_get failure");

  /* Elapsed time */
  time = MSG_get_clock() - time;
  
  INFO1("Total transmission time: %f", time);
  INFO1("Hockney estimated bandwidth: %f", ((task_comm_size*8)/time)/1000);

  MSG_task_destroy(task);
  return 0;
} /* end_of_slave */




/** Test function */
MSG_error_t test_all(const char *platform_file,
		     const char *application_file){
  MSG_error_t res = MSG_OK;

  /*  Simulation setting */
  MSG_set_channel_number(MAX_CHANNEL);
  MSG_paje_output("msg_test.trace");
  MSG_create_environment(platform_file);
    
  /*   Application deployment */
  MSG_function_register("master", master);
  MSG_function_register("slave", slave);
  MSG_launch_application(application_file);
  
  res = MSG_main();
  return res;
} /* end_of_test_all */


/** Main function */
int main(int argc, char *argv[]){
  MSG_error_t res = MSG_OK;

  MSG_global_init(&argc,argv);
  if (argc < 3) {
     printf ("Usage: %s platform_file deployment_file\n",argv[0]);
     exit(1);
  }
  res = test_all(argv[1],argv[2]);

  MSG_clean();

  if(res==MSG_OK) return 0; 
  else return 1;
} /* end_of_main */
