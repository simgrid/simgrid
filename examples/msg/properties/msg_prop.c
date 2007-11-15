/* 	$Id$	 */

/* Copyright (c) 2007. SimGrid Team. All rights reserved.                   */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg/msg.h" /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "xbt/sysdep.h" /* calloc, printf */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"

#include <stdio.h>
#include <stdlib.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test,"Property test");

int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);
int forwarder(int argc, char *argv[]);
MSG_error_t test_all(const char *platform_file, const char *application_file);

/** Emitter function  */
int master(int argc, char *argv[])
{
  int slaves_count = 0;
  m_host_t *slaves = NULL;
  xbt_dict_t props; 
  xbt_dict_cursor_t cursor=NULL;
  int i;
  char *key,*data;
  const char *noexist="Unknown";
  const char*value;
  char exist[]="Hdd";

  {                  /* Process organisation */
    slaves_count = argc - 4;
    slaves = xbt_new(m_host_t, sizeof(m_host_t) * slaves_count);
      
    for (i = 4; i < argc; i++) {     
      slaves[i-4] = MSG_get_host_by_name(argv[i]);
      xbt_assert1(slaves[i-4]!=NULL, "Unknown host %s. Stopping Now! ", argv[i]);

      /* Get the property list of the host */
      props = MSG_host_get_properties(slaves[i-4]);
     
      

      /* Print the properties of the host */
      xbt_dict_foreach(props,cursor,key,data) {
         INFO3("Property: %s for host: %s has value: %s",key,argv[i],data);
      }

     /* Try to get a property that does not exist */
     
     value = MSG_host_get_property_value(slaves[i-4], noexist);
     if ( value == NULL) 
       INFO2("Property: %s for host %s is undefined", noexist, argv[i]);
     else
       INFO3("Property: %s for host %s has value: %s",(char*) noexist, argv[i], value);

      /* Modify an existing property test. First check it exists */\
      INFO0("Trying to modify a host property");
      
      value = MSG_host_get_property_value(slaves[i-4],exist);
      xbt_assert1(value,"\tProperty %s is undefined", exist);
      INFO2("\tProperty: %s old value: %s", exist, value);
      xbt_dict_set(props, exist, xbt_strdup("250"), free);  
 
      /* Test if we have changed the value */
      value = MSG_host_get_property_value(slaves[i-4],exist);
      xbt_assert1(value,"\tProperty %s is undefined", exist);
      INFO2("\tProperty: %s new value: %s", exist, value);
    }
  }
  
  free(slaves);
  return 0;
} /* end_of_master */

/** Receiver function  */
int slave(int argc, char *argv[])
{
  /* Get the property list of current slave process */
  xbt_dict_t props = MSG_process_get_properties(MSG_process_self());
  xbt_dict_cursor_t cursor=NULL;
  char *key,*data;
  const char *noexist="UnknownProcessProp";
  const char *value;

  /* Print the properties of the process */
  xbt_dict_foreach(props,cursor,key,data) {
     INFO3("Property: %s for process %s has value: %s",key,MSG_process_get_name(MSG_process_self()),data);
  }

  /* Try to get a property that does not exist */
 
  value = MSG_process_get_property_value(MSG_process_self(),noexist);
  if ( value == NULL) 
    INFO2("Property: %s for process %s is undefined", noexist, MSG_process_get_name(MSG_process_self()));
  else
    INFO3("Property: %s for process %s has value: %s", noexist, MSG_process_get_name(MSG_process_self()), value);

  return 0;
} /* end_of_slave */

/** Test function */
MSG_error_t test_all(const char *platform_file,
			    const char *application_file)
{
  MSG_error_t res = MSG_OK;

  /* MSG_config("surf_workstation_model","KCCFLN05"); */
  {				/*  Simulation setting */
//    MSG_set_channel_number(MAX_CHANNEL);
    MSG_paje_output("msg_test.trace");
    MSG_create_environment(platform_file);
  }
  {                            /*   Application deployment */
    MSG_function_register("master", master);
    MSG_function_register("slave", slave);
    MSG_launch_application(application_file);
  }
  res = MSG_main();
  
  INFO1("Simulation time %g",MSG_get_clock());
  return res;
} /* end_of_test_all */


/** Main function */
int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;

  MSG_global_init(&argc,argv);
  if (argc < 3) {
     printf ("Usage: %s platform_file deployment_file\n",argv[0]);
     printf ("example: %s msg_platform.xml msg_deployment.xml\n",argv[0]);
     exit(1);
  }
  res = test_all(argv[1],argv[2]);
  MSG_clean();

  if(res==MSG_OK)
    return 0;
  else
    return 1;
} /* end_of_main */
