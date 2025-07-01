/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class Writer extends Actor {
  public void run()
  {
    /* - Retrieve all disks from current host */
    Disk[] disk_list = Host.current().get_disks();
    /* - Write 4,000,000 bytes on Disk1 */
    disk_list[0].write(4000000);
    Engine.info("First write done.");
    /* - Write 4,000,000 bytes on Disk1 again */
    disk_list[0].write(4000000);
    Engine.info("Second write done.");
  }
}
class PrivilegedWriter extends Actor {
  public void run() throws TimeoutException
  {
    /* - Retrieve all disks from current host */
    Disk[] disk_list = Host.current().get_disks();

    /* - Write 4,000,000 bytes on Disk1 but specifies that this I/O operation gets a larger share of the resource.
     *
     * Since the priority is 2, it writes twice as fast as a regular one.
     *
     * So instead of a half/half sharing between the two, we get a 1/3 vs. 2/3 sharing. */
    disk_list[0].write(4000000, 2);
    Engine.info("First write done.");

    /* Note that the timings printed when running this example are a bit misleading, because the uneven sharing only
     * last until the privileged actor ends. After this point, the unprivileged one gets 100% of the disk and finishes
     * quite quickly. */

    /* Resynchronize actors before second write */
    this.sleep_for(0.05);

    /* - Write 4,000,000 bytes on Disk1 again and this time :
     *    - Start the I/O operation asynchronously to get an IoPtr
     *    - Let the actor sleep while half of the data is written
     *    - Double its priority
     *    - Wait for the end of the I/O operation
     *
     *   Writing the second half of the data with a higher priority, and thus 2/3 of the bandwidth takes 0.075s.
     *   In the meantime, the regular writer has only 1/3 of the bandwidth and thus only writes 1MB. After the
     * completion of the I/O initiated by the privileged writer, the regular writer has the full bandwidth available and
     * only needs 0.025s to write the last MB.
     */

    Io io = disk_list[0].write_async(4000000);
    this.sleep_for(0.1);
    Engine.info("Increase priority for the priviledged writer (%.0f bytes remaining to write)", io.get_remaining());
    io.update_priority(2);
    io.await();
    Engine.info("Second write done.");
  }
}

public class io_priority {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    e.load_platform(args[0]);

    e.host_by_name("bob").add_actor("writer", new Writer());
    e.host_by_name("bob").add_actor("privileged_writer", new PrivilegedWriter());

    e.run();
  }
}
