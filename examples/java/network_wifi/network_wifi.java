/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example demonstrates how to use wifi links in SimGrid. Most of the interesting things happen in the
 * corresponding XML file: examples/platforms/wifi.xml
 */

import org.simgrid.s4u.*;

class Sender extends Actor {
  Mailbox mailbox;
  int data_size;

  public Sender(Mailbox mailbox, int data_size)
  {
    this.mailbox   = mailbox;
    this.data_size = data_size;
  }

  public void run() throws SimgridException
  {
    Engine.info("Send a message to the other station.");
    String message = "message";
    mailbox.put(message, data_size);
    Engine.info("Done.");
  }
}

class Receiver extends Actor {
  Mailbox mailbox;

  public Receiver(Mailbox mailbox) { this.mailbox = mailbox; }

  public void run() throws SimgridException
  {
    Engine.info("Wait for a message.");
    mailbox.get();
    Engine.info("Done.");
  }
}

public class network_wifi {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    if (args.length < 1)
      Engine.die("Usage: network_wifi platform_file\n\tExample: platform.xml deployment.xml\n");

    e.load_platform(args[0]);

    /* Exchange a message between the 2 stations */
    Mailbox mailbox = e.mailbox_by_name("mailbox");
    Host station1   = e.host_by_name("Station 1");
    Host station2   = e.host_by_name("Station 2");
    station1.add_actor("sender", new Sender(mailbox, (int)1e7));
    station2.add_actor("receiver", new Receiver(mailbox));

    /* Declare that the stations are not at the same distance from their AP */
    Link ap = e.link_by_name("AP1");
    ap.set_host_wifi_rate(station1, 1); // The host "Station 1" uses the second level of bandwidths on that AP
    ap.set_host_wifi_rate(station2, 0); // This is perfectly useless as level 0 is used by default

    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
