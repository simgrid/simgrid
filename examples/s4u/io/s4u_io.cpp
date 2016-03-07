/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <map>
#include <unordered_map>
#include <vector>

#include "simgrid/s4u.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "a sample log category");

class myHost : simgrid::s4u::Actor {
public:
  myHost(const char*procname, simgrid::s4u::Host *host,int argc, char **argv)
: simgrid::s4u::Actor(procname,host,argc,argv){}

  void show_info(boost::unordered_map <std::string, simgrid::s4u::Storage*> const&mounts) {
    XBT_INFO("Storage info on %s:",
      simgrid::s4u::Host::current()->name().c_str());

    for (const auto&kv : mounts) {
      const char* mountpoint = kv.first.c_str();
      simgrid::s4u::Storage &storage = *kv.second;

      // Retrieve disk's information
      sg_size_t free_size = storage.sizeFree();
      sg_size_t used_size = storage.sizeUsed();
      sg_size_t size = storage.size();

      XBT_INFO("    %s (%s) Used: %llu; Free: %llu; Total: %llu.",
          storage.name(), mountpoint, used_size, free_size, size);
    }
  }

  int main(int argc, char **argv) {
    boost::unordered_map <std::string, simgrid::s4u::Storage *> const& mounts =
      simgrid::s4u::Host::current()->mountedStorages();

    show_info(mounts);

    // Open an non-existing file to create it
    const char *filename = "/home/tmp/data.txt";
    simgrid::s4u::File *file = new simgrid::s4u::File(filename, NULL);
    sg_size_t write, read, file_size;

    write = file->write(200000);  // Write 200,000 bytes
    XBT_INFO("Create a %llu bytes file named '%s' on /sd1", write, filename);

    // check that sizes have changed
    show_info(mounts);

    // Now retrieve the size of created file and read it completely
    file_size = file->size();
    file->seek(0);
    read = file->read(file_size);
    XBT_INFO("Read %llu bytes on %s", read, filename);

    // Now write 100,000 bytes in tmp/data.txt
    write = file->write(100000);  // Write 100,000 bytes
    XBT_INFO("Write %llu bytes on %s", write, filename);

    simgrid::s4u::Storage &storage = simgrid::s4u::Storage::byName("Disk4");

    // Now rename file from ./tmp/data.txt to ./tmp/simgrid.readme
    const char *newpath = "/home/tmp/simgrid.readme";
    XBT_INFO("Move '%s' to '%s'", file->path(), newpath);
    file->move(newpath);

    // Test attaching some user data to the file
    file->setUserdata(xbt_strdup("777"));
    XBT_INFO("User data attached to the file: %s", (char*)file->userdata());

    // Close the file
    delete file;

    // Now attach some user data to disk1
    XBT_INFO("Get/set data for storage element: %s",storage.name());
    XBT_INFO("    Uninitialized storage data: '%s'", (char*)storage.userdata());

    storage.setUserdata(xbt_strdup("Some user data"));
    XBT_INFO("    Set and get data: '%s'", (char*)storage.userdata());

    /*
      // Dump disks contents
      XBT_INFO("*** Dump content of %s ***",Host::current()->name());
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
     */
    return 0;
  }
};

int main(int argc, char **argv) {
  simgrid::s4u::Engine *e = new simgrid::s4u::Engine(&argc,argv);
  e->loadPlatform("../../platforms/storage/storage.xml");

  new myHost("host", simgrid::s4u::Host::by_name("denise"), 0, NULL);
  e->run();
  return 0;
}
