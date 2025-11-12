/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class HostRunner extends Actor {
  public void run()
  {

    /* -Add an extra disk in a programmatic way */
    Host.current().add_disk("Disk3", /*read bandwidth*/ 9.6e7, /*write bandwidth*/ 6.4e7).seal();

    /* - Display information on the disks mounted by the current host */
    Engine.info("*** Storage info on %s ***", Host.current().get_name());

    /* - Retrieve all disks from current host */
    Disk[] disk_list = Host.current().get_disks();

    /* - For each disk mounted on host, display disk name and mount point */
    for (Disk disk : disk_list)
      Engine.info("Disk name: %s (read: %.0f B/s -- write: %.0f B/s)", disk.get_name(), disk.get_read_bandwidth(),
                  disk.get_write_bandwidth());

    /* - Write 400,000 bytes on Disk1 */
    Disk disk = disk_list[0];
    long write = disk.write(400000);
    Engine.info("Wrote %d bytes on '%s'", write, disk.get_name());

    /*  - Now read 200,000 bytes */
    long read = disk.read(200000);
    Engine.info("Read %d bytes on '%s'", read, disk.get_name());

    /* - Write 800,000 bytes on Disk3 */
    Disk disk3         = disk_list[disk_list.length - 1];
    long write_on_disk3 = disk3.write(800000);
    Engine.info("Wrote %d bytes on '%s'", write_on_disk3, disk3.get_name());

    /* - Attach some user data to disk1 */
    Engine.info("*** Get/set data for storage element: Disk1 ***");

    String data = (String)disk.get_data();

    Engine.info("Get storage data: '%s'", data);

    disk.set_data("Some user data");
    data = (String)disk.get_data();
    Engine.info("Set and get data: '%s'", data);
  }
}

public class io_disk_raw {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.load_platform(args[0]);

    /* - Display Host properties */
    for (Host h : e.get_all_hosts()) {
      Engine.info("*** %s properties ****", h.get_name());
      for (String key : h.get_properties_names())
        Engine.info("  %s -> %s", key, h.get_property(key));
    }

    e.host_by_name("bob").add_actor("", new HostRunner());

    e.run();
    Engine.info("Simulated ends.");

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
