/* Copyright (c) 2008-2010, 2012-2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @addtogroup MSG_examples
 * 
 * @subsection MSG_ex_resources Other resource kinds
 * 
 * This section contains some sparse examples of how to use the other
 * kind of resources, such as disk or GPU. These resources are quite
 * experimental for now, but here we go anyway.
 * 
 * - <b>io/file.c</b> Example with the disk resource
 */

#define FILENAME1 "./doc/simgrid/examples/platforms/g5k.xml"

#include <stdio.h>
#include <stdlib.h>
#include "msg/msg.h"
#include "surf/surf_private.h"

int host(int argc, char *argv[]);

XBT_LOG_NEW_DEFAULT_CATEGORY(io_file,
                             "Messages specific for this io example");

int host(int argc, char *argv[])
{
  msg_file_t file = NULL;
  char* mount = xbt_strdup("/home");
  size_t write;

  // First open
  XBT_INFO("\tOpen file '%s'",FILENAME1);
  file = MSG_file_open(mount,FILENAME1, NULL);

  // Unlink the file
  XBT_INFO("\tUnlink file '%s'",file->fullname);
  MSG_file_unlink(file);

  // Re Open the file wich is in fact created
  XBT_INFO("\tOpen file '%s'",FILENAME1);
  file = MSG_file_open(mount,FILENAME1, NULL);

  // Write into the new file
  write = MSG_file_write(100000,file);  // Write for 100Ko
  XBT_INFO("\tHave written %zu on %s",write,file->fullname);

  // Close the file
  XBT_INFO("\tClose file '%s'",file->fullname);
  MSG_file_close(file);

  xbt_dict_t dict_ls;
  char* key;
  surf_stat_t data = NULL;
  xbt_dict_cursor_t cursor = NULL;

  dict_ls = MSG_file_ls(mount,"./");
  XBT_INFO(" ");XBT_INFO("ls ./");
  xbt_dict_foreach(dict_ls,cursor,key,data){
    if(data) XBT_INFO("FILE : %s",key);
    else     XBT_INFO("DIR  : %s",key);
  }
  xbt_dict_free(&dict_ls);

  dict_ls = MSG_file_ls(mount,"./doc/simgrid/examples/platforms/");
  XBT_INFO(" ");XBT_INFO("ls ./doc/simgrid/examples/platforms/");
  xbt_dict_foreach(dict_ls,cursor,key,data){
    if(data) XBT_INFO("FILE : %s",key);
    else     XBT_INFO("DIR  : %s",key);
  }
  xbt_dict_free(&dict_ls);

  dict_ls = MSG_file_ls(mount,"./doc/simgrid/examples/msg/");
  XBT_INFO(" ");XBT_INFO("ls ./doc/simgrid/examples/msg/");
  xbt_dict_foreach(dict_ls,cursor,key,data){
    if(data) XBT_INFO("FILE : %s",key);
    else     XBT_INFO("DIR  : %s",key);
  }
  xbt_dict_free(&dict_ls);

  free(mount);

  return 0;
}

int main(int argc, char **argv)
{
  int res;
  MSG_init(&argc, argv);
  MSG_create_environment(argv[1]);
  xbt_dynar_t hosts =  MSG_hosts_as_dynar();
  MSG_function_register("host", host);
  unsigned long nb_hosts = xbt_dynar_length(hosts);
  XBT_INFO("Number of host '%lu'",nb_hosts);
  char* name_host = xbt_strdup("0");
  MSG_process_create( name_host, host, NULL, xbt_dynar_get_as(hosts,0,msg_host_t) );
  free(name_host);

  xbt_dynar_free(&hosts);

  res = MSG_main();
  XBT_INFO("Simulation time %g", MSG_get_clock());
  if (res == MSG_OK)
    return 0;
  else
    return 1;

}
