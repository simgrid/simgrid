/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "simix/simix.h"
#include "msg/msg.h"
#include "surf/surf_private.h"

int host(int argc, char *argv[]);

XBT_LOG_NEW_DEFAULT_CATEGORY(file,
                             "Messages specific for this simix example");

int host(int argc, char *argv[])
{
  size_t read = simcall_file_read(NULL,0,0,NULL);
  XBT_INFO("Host '%s' read %ld",MSG_host_get_name(MSG_host_self()),read);
  size_t write = simcall_file_write(NULL,0,0,NULL);
  XBT_INFO("Host '%s' write %ld",MSG_host_get_name(MSG_host_self()), write);
  return 0;
}

int main(int argc, char **argv)
{
    int i,res;
  MSG_global_init(&argc, argv);
  MSG_create_environment(argv[1]);
  m_host_t *host_table =  MSG_get_host_table();
  int number_of_hosts = MSG_get_host_number();
  MSG_function_register("host", host);

  XBT_INFO("Number of host '%d'",number_of_hosts);
  for(i = 0 ; i<number_of_hosts; i++)
  {
    char* name_host = bprintf("%d",i);
    MSG_process_create( name_host, host, NULL, host_table[i] );
    free(name_host);
  }
  xbt_free(host_table);

  res = MSG_main();
  XBT_INFO("Simulation time %g", MSG_get_clock());
  MSG_clean();
  if (res == MSG_OK)
    return 0;
  else
    return 1;

}
