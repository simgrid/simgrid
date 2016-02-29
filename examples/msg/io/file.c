/* Copyright (c) 2008-2010, 2012-2015. The SimGrid Team.
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

#define FILENAME1 "/home/doc/simgrid/examples/platforms/g5k.xml"
#define FILENAME2 "c:\\Windows\\setupact.log"
#define FILENAME3 "/home/doc/simgrid/examples/platforms/g5k_cabinets.xml"
#define FILENAME4 "/home/doc/simgrid/examples/platforms/nancy.xml"

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(io_file, "Messages specific for this io example");

static int host(int argc, char *argv[])
{
  msg_file_t file = NULL;
  sg_size_t read,write;
  msg_storage_t st;
  const char* st_name;

  if(!strcmp(MSG_process_get_name(MSG_process_self()),"0")){
    file = MSG_file_open(FILENAME1, NULL);
    MSG_file_dump(file);
    st_name = "Disk4";
  } else if(!strcmp(MSG_process_get_name(MSG_process_self()),"1")) {
    file = MSG_file_open(FILENAME2, NULL);
    st_name = "Disk2";
  } else if(!strcmp(MSG_process_get_name(MSG_process_self()),"2")){
    file = MSG_file_open(FILENAME3, NULL);
    st_name = "Disk3";
  } else if(!strcmp(MSG_process_get_name(MSG_process_self()),"3")){
    file = MSG_file_open(FILENAME4, NULL);
    st_name = "Disk1";
  }
  else xbt_die("FILENAME NOT DEFINED %s",MSG_process_get_name(MSG_process_self()));

  const char* filename = MSG_file_get_name(file);
  XBT_INFO("\tOpen file '%s'",filename);
  st = MSG_storage_get_by_name(st_name);

  XBT_INFO("\tCapacity of the storage element '%s' is stored on: %llu / %llu",
            filename, MSG_storage_get_used_size(st), MSG_storage_get_size(st));

  /* Try to read for 10MB */
  read = MSG_file_read(file, 10000000);
  XBT_INFO("\tHave read %llu from '%s'",read,filename);

  /* Write 100KB in file from the current position, i.e, end of file or 10MB */
  write = MSG_file_write(file, 100000);
  XBT_INFO("\tHave written %llu in '%s'. Size now is: %llu",write,filename, MSG_file_get_size(file));


  XBT_INFO("\tCapacity of the storage element '%s' is stored on: %llu / %llu",
            filename, MSG_storage_get_used_size(st), MSG_storage_get_size(st));

  /* rewind to the beginning of the file */
  XBT_INFO("\tComing back to the beginning of the stream for file '%s'", filename);
  MSG_file_seek(file, 0, SEEK_SET);

  /* Try to read 110KB */
  read = MSG_file_read(file, 110000);
  XBT_INFO("\tHave read %llu from '%s' (of size %llu)",read,filename, MSG_file_get_size(file));

  /* rewind once again to the beginning of the file */
  XBT_INFO("\tComing back to the beginning of the stream for file '%s'", filename);
  MSG_file_seek(file, 0, SEEK_SET);

  /* Write 110KB in file from the current position, i.e, end of file or 10MB */
  write = MSG_file_write(file, 110000);
  XBT_INFO("\tHave written %llu in '%s'. Size now is: %llu", write,filename, MSG_file_get_size(file));

  XBT_INFO("\tCapacity of the storage element '%s' is stored on: %llu / %llu",
            filename, MSG_storage_get_used_size(st), MSG_storage_get_size(st));

  XBT_INFO("\tClose file '%s'",filename);
  MSG_file_close(file);

  return 0;
}

int main(int argc, char **argv)
{
  MSG_init(&argc, argv);
  MSG_create_environment(argv[1]);
  xbt_dynar_t hosts =  MSG_hosts_as_dynar();
  MSG_function_register("host", host);
  unsigned long nb_hosts = xbt_dynar_length(hosts);
  XBT_INFO("Number of host '%lu'",nb_hosts);
  for(int i = 0 ; i<nb_hosts; i++){
    char* name_host = bprintf("%d",i);
    MSG_process_create( name_host, host, NULL, xbt_dynar_get_as(hosts,i,msg_host_t) );
    free(name_host);
  }
  xbt_dynar_free(&hosts);

  int res = MSG_main();
  XBT_INFO("Simulation time %g", MSG_get_clock());
  return res != MSG_OK;
}
