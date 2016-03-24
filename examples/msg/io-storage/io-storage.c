/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @addtogroup MSG_examples
 * 
 * - <b>io/storage.c</b> demo of all main storage and file functions
 */

/********************* Files and Storage handling ****************************
 * This example implements all main storage and file functions of the MSG API
 *
 * Scenario :
 * - display information on the disks mounted by the current host
 * - create a 200,000 bytes file
 * - completely read the created file
 * - write 100,000 bytes in the file
 * - rename the created file
 * - attach some user data to a disk
 * - dump disk's contents
 *
******************************************************************************/

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(storage,"Messages specific for this simulation");

static int host(int argc, char *argv[]){
  const char* host_name = MSG_host_get_name(MSG_host_self());

  // display information on the disks mounted by the current host
  XBT_INFO("*** Storage info on %s ***", host_name);

  xbt_dict_cursor_t cursor = NULL;
  char* mount_name;
  char* storage_name;
  msg_storage_t storage = NULL;

  // Retrieve all mount points of current host
  xbt_dict_t storage_list = MSG_host_get_mounted_storage_list(MSG_host_self());

  xbt_dict_foreach(storage_list,cursor,mount_name,storage_name)  {
    // For each disk mounted on host
    XBT_INFO("Storage name: %s, mount name: %s", storage_name, mount_name);
    storage = MSG_storage_get_by_name(storage_name);

    // Retrieve disk's information
    sg_size_t free_size = MSG_storage_get_free_size(storage);
    sg_size_t used_size = MSG_storage_get_used_size(storage);
    sg_size_t size = MSG_storage_get_size(storage);

    XBT_INFO("Total size: %llu bytes", size);
    XBT_INFO("Free size: %llu bytes", free_size);
    XBT_INFO("Used size: %llu bytes", used_size);
  }
  xbt_dict_free(&storage_list);

  // Create a 200,000 bytes file named './tmp/data.txt' on /sd1
  char* file_name = xbt_strdup("/home/tmp/data.txt");
  msg_file_t file = NULL;
  sg_size_t write, read, file_size;

  // Open an non-existing file amounts to create it!
  file = MSG_file_open(file_name, NULL);
  write = MSG_file_write(file, 200000);  // Write 200,000 bytes
  XBT_INFO("Create a %llu bytes file named '%s' on /sd1", write, file_name);
  MSG_file_dump(file);

  // check that sizes have changed
  XBT_INFO("Free size: %llu bytes", MSG_storage_get_free_size(storage));
  XBT_INFO("Used size: %llu bytes", MSG_storage_get_used_size(storage));

  // Now retrieve the size of created file and read it completely
  file_size = MSG_file_get_size(file);
  MSG_file_seek(file, 0, SEEK_SET);
  read = MSG_file_read(file, file_size);
  XBT_INFO("Read %llu bytes on %s", read, file_name);

  // Now write 100,000 bytes in tmp/data.txt
  write = MSG_file_write(file, 100000);  // Write 100,000 bytes
  XBT_INFO("Write %llu bytes on %s", write, file_name);
  MSG_file_dump(file);

  storage_name = xbt_strdup("Disk4");
  storage = MSG_storage_get_by_name(storage_name);

  // Now rename file from ./tmp/data.txt to ./tmp/simgrid.readme
  XBT_INFO("*** Move '/tmp/data.txt' into '/tmp/simgrid.readme'");
  MSG_file_move(file, "/home/tmp/simgrid.readme");

  // Attach some user data to the file
  MSG_file_set_data(file, xbt_strdup("777"));
  // Retrieve these data
  char *data = MSG_file_get_data(file);
  XBT_INFO("User data attached to the file: %s", data);

  MSG_file_close(file);
  free(file_name);

  // Now attach some user data to disk1
  XBT_INFO("*** Get/set data for storage element: %s ***",storage_name);

  data = MSG_storage_get_data(storage);

  XBT_INFO("Get storage data: '%s'", data);

  MSG_storage_set_data(storage, xbt_strdup("Some user data"));
  data = MSG_storage_get_data(storage);
  XBT_INFO("Set and get data: '%s'", data);
  xbt_free(data);
  xbt_free(storage_name);

  // Dump disks contents
  XBT_INFO("*** Dump content of %s ***",MSG_host_get_name(MSG_host_self()));
  xbt_dict_t contents = NULL;
  contents = MSG_host_get_storage_content(MSG_host_self()); // contents is a dict of dicts
  xbt_dict_cursor_t curs, curs2 = NULL;
  char* mountname;
  xbt_dict_t content;
  char* path;
  sg_size_t *size;
  xbt_dict_foreach(contents, curs, mountname, content){
    XBT_INFO("Print the content of mount point: %s",mountname);
    xbt_dict_foreach(content,curs2,path,size){
       XBT_INFO("%s size: %llu bytes", path,*((sg_size_t*)size));
    }
  xbt_dict_free(&content);
  }
  xbt_dict_free(&contents);
  return 1;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  MSG_create_environment(argv[1]);
  MSG_function_register("host", host);
  xbt_dynar_t hosts =  MSG_hosts_as_dynar();
  MSG_process_create(NULL, host, NULL, xbt_dynar_get_as(hosts,0,msg_host_t) );
  xbt_dynar_free(&hosts);

  msg_error_t res = MSG_main();
  XBT_INFO("Simulated time: %g", MSG_get_clock());

  return res != MSG_OK;
}
