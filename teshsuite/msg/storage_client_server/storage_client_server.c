/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(storage,"Messages specific for this simulation");

static void display_storage_properties(msg_storage_t storage){
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data;
  xbt_dict_t props = MSG_storage_get_properties(storage);
  if (xbt_dict_length(props) > 0){
    XBT_INFO("\tProperties of mounted storage: %s", MSG_storage_get_name(storage));
    xbt_dict_foreach(props, cursor, key, data)
    XBT_INFO("\t\t'%s' -> '%s'", key, data);
  }else{
  XBT_INFO("\tNo property attached.");
  }
}

static sg_size_t write_local_file(const char *dest, sg_size_t file_size)
{
  sg_size_t written;
  msg_file_t file = MSG_file_open(dest, NULL);
  written = MSG_file_write(file, file_size);
  XBT_INFO("%llu bytes on %llu bytes have been written by %s on /sd1",written, file_size,
           MSG_host_get_name(MSG_host_self()));
  MSG_file_close(file);
  return written;
}

static sg_size_t read_local_file(const char *src)
{
  sg_size_t read, file_size;
  msg_file_t file = MSG_file_open(src, NULL);
  file_size = MSG_file_get_size(file);

  read = MSG_file_read(file, file_size);
  XBT_INFO("%s has read %llu on %s",MSG_host_get_name(MSG_host_self()),read,src);
  MSG_file_close(file);

  return read;
}

// Read src file on local disk and send a put message to remote host (size of message = size of src file)
static int hsm_put(const char *remote_host, const char *src, const char *dest){
  // Read local src file, and return the size that was actually read
  sg_size_t read_size = read_local_file(src);

  // Send file
  XBT_INFO("%s sends %llu to %s",MSG_host_get_name(MSG_host_self()),read_size,remote_host);
  msg_task_t to_execute = MSG_task_create((const char*)"hsm_put", 0, (double) read_size, (void*)dest);
  MSG_task_send(to_execute, remote_host);
  MSG_process_sleep(.4);
  return 1;
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
  }
  else{
    XBT_INFO("Unable to retrieve storage element by its name: %s.", name);
  }
}

static void get_set_storage_data(const char *storage_name){
  XBT_INFO("*** GET/SET DATA for storage element: %s ***",storage_name);
  msg_storage_t storage = MSG_storage_get_by_name(storage_name);
  char *data = MSG_storage_get_data(storage);
  XBT_INFO("Get data: '%s'", data);

  MSG_storage_set_data(storage, xbt_strdup("Some data"));
  data = MSG_storage_get_data(storage);
  XBT_INFO("\tSet and get data: '%s'", data);
  xbt_free(data);
}

static void dump_platform_storages(void){
  unsigned int cursor;
  xbt_dynar_t storages = MSG_storages_as_dynar();
  msg_storage_t storage;
  xbt_dynar_foreach(storages, cursor, storage){
    XBT_INFO("Storage %s is attached to %s", MSG_storage_get_name(storage), MSG_storage_get_host(storage));
    MSG_storage_set_property_value(storage, "other usage", xbt_strdup("gpfs"), xbt_free_f);
  }
  xbt_dynar_free(&storages);
}

static void storage_info(msg_host_t host)
{
  const char* host_name = MSG_host_get_name(host);
  XBT_INFO("*** Storage info on %s ***", host_name);

  xbt_dict_cursor_t cursor = NULL;
  char* mount_name;
  char* storage_name;
  msg_storage_t storage;

  xbt_dict_t storage_list = MSG_host_get_mounted_storage_list(MSG_host_self());

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

static int client(int argc, char *argv[])
{
  hsm_put("alice","/home/doc/simgrid/examples/msg/icomms/small_platform.xml","c:\\Windows\\toto.cxx");
  hsm_put("alice","/home/doc/simgrid/examples/msg/parallel_task/test_ptask_deployment.xml","c:\\Windows\\titi.xml");
  hsm_put("alice","/home/doc/simgrid/examples/msg/alias/masterslave_forwarder_with_alias.c","c:\\Windows\\tata.c");

  msg_task_t finalize = MSG_task_create("finalize", 0, 0, NULL);
  MSG_task_send(finalize, "alice");

  get_set_storage_data("Disk1");

  return 1;
}

static int server(int argc, char *argv[])
{
  msg_task_t to_execute = NULL;
  XBT_ATTRIB_UNUSED int res;

  storage_info(MSG_host_self());

  XBT_INFO("Server waiting for transfers ...");
  while(1){
    res = MSG_task_receive(&(to_execute), MSG_host_get_name(MSG_host_self()));
    xbt_assert(res == MSG_OK, "MSG_task_get failed");

    const char *task_name;
    task_name = MSG_task_get_name(to_execute);

    if (!strcmp(task_name, "finalize")) { // Shutdown ...
      MSG_task_destroy(to_execute);
      break;
    } else if(!strcmp(task_name,"hsm_put")){// Receive file to save
      // Write file on local disk
      char *dest = MSG_task_get_data(to_execute);
      sg_size_t size_to_write = (sg_size_t)MSG_task_get_bytes_amount(to_execute);
      write_local_file(dest, size_to_write);
    }

    MSG_task_destroy(to_execute);
    to_execute = NULL;
  }

  storage_info(MSG_host_self());
  dump_platform_storages();
  return 1;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);

  /* Check the arguments */
  xbt_assert(argc == 2,"Usage: %s platform_file\n", argv[0]);

  MSG_create_environment(argv[1]);

  MSG_process_create("server", server, NULL, MSG_get_host_by_name("alice"));
  MSG_process_create("client", client, NULL, MSG_get_host_by_name("bob"));

  msg_error_t res = MSG_main();
  XBT_INFO("Simulated time: %g", MSG_get_clock());

  return res != MSG_OK;
}
