/* Copyright (c) 2008-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include <unistd.h>

#define FILENAME1 "/sd1/doc/simgrid/examples/cxx/autoDestination/Main.cxx"

XBT_LOG_NEW_DEFAULT_CATEGORY(storage,"Messages specific for this simulation");

static void display_storage_properties(msg_storage_t storage){
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data;
  xbt_dict_t props = MSG_storage_get_properties(storage);
  if (xbt_dict_length(props) > 0){
    XBT_INFO("\tProperties of mounted storage: %s", MSG_storage_get_name(storage));
    xbt_dict_foreach(props, cursor, key, data)
    XBT_INFO("\t\t'%s' -> '%s'", key, data);
  } else {
  XBT_INFO("\tNo property attached.");
  }
}

static void display_storage_content(msg_storage_t storage){
  XBT_INFO("Print the content of the storage element: %s",MSG_storage_get_name(storage));
  xbt_dict_cursor_t cursor = NULL;
  char *file;
  sg_size_t *psize;
  xbt_dict_t content = MSG_storage_get_content(storage);
  if (content){
    xbt_dict_foreach(content, cursor, file, psize)
    XBT_INFO("\t%s size: %llu bytes", file, *psize);
  } else {
    XBT_INFO("\tNo content.");
  }
  xbt_dict_free(&content);
}

static void dump_storage_by_name(char *name){
  XBT_INFO("*** Dump a storage element ***");
  msg_storage_t storage = MSG_storage_get_by_name(name);

  if(storage){
    display_storage_content(storage);
  } else{
    XBT_INFO("Unable to retrieve storage element by its name: %s.", name);
  }
}
static void storage_info(msg_host_t host)
{
  const char* host_name = MSG_host_get_name(host);
  XBT_INFO("*** Storage info on %s ***", host_name);

  xbt_dict_cursor_t cursor = NULL;
  char* mount_name;
  char* storage_name;
  msg_storage_t storage;

  xbt_dict_t storage_list = MSG_host_get_mounted_storage_list(host);

  xbt_dict_foreach(storage_list,cursor,mount_name,storage_name){
    XBT_INFO("\tStorage name: %s, mount name: %s", storage_name, mount_name);
    storage = MSG_storage_get_by_name(storage_name);

    sg_size_t free_size = MSG_storage_get_free_size(storage);
    sg_size_t used_size = MSG_storage_get_used_size(storage);

    XBT_INFO("\t\tFree size: %llu bytes", free_size);
    XBT_INFO("\t\tUsed size: %llu bytes", used_size);

    display_storage_properties(storage);
    dump_storage_by_name(storage_name);
  }
  xbt_dict_free(&storage_list);
}

static int host(int argc, char *argv[])
{
  char name[2048];
  sprintf(name,"%s%i", FILENAME1,MSG_process_self_PID());
  msg_file_t file = MSG_file_open(name, NULL);
  //MSG_file_read(file, MSG_file_get_size(file));
  MSG_file_write(file, 500000);

  XBT_INFO("Size of %s: %llu", MSG_file_get_name(file), MSG_file_get_size(file));
  MSG_file_close(file);

  return 1;
}

int main(int argc, char **argv)
{
  MSG_init(&argc, argv);
  MSG_create_environment(argv[1]);

  MSG_function_register("host", host);
  storage_info(MSG_host_by_name(xbt_strdup("host")));
  for(int i = 0 ; i<10; i++){
    MSG_process_create(xbt_strdup("host"), host, NULL, MSG_host_by_name(xbt_strdup("host")));
  }

  int res = MSG_main();
  storage_info(MSG_host_by_name(xbt_strdup("host")));
  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
