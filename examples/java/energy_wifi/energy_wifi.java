/* Copyright (c) 2020-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

/**
 * Test the wifi energy plugin
 * Desactivate cross-factor to get round values
 * Launch with: ./test test_platform_2STA.xml --cfg=plugin:link_energy_wifi --cfg=network/crosstraffic:0
 */

class Sender extends Actor {
  public void run()
  {
    // start sending after 5 seconds
    sleep_until(5);

    String mbName = "MailBoxRCV";
    Mailbox dst   = get_engine().mailbox_by_name(mbName);

    int size = 6750000;

    Engine.info("SENDING 1 msg of size %d to %s", size, mbName);
    String message = "message";
    dst.put(message, size);
    Engine.info("finished sending");
  }
}

class Receiver extends Actor {
  public void run() throws SimgridException
  {
    String mbName = "MailBoxRCV";
    Engine.info("RECEIVING on mb %s", mbName);
    Mailbox myBox = get_engine().mailbox_by_name(mbName);
    myBox.get();

    Engine.info("received all messages");
  }
}

public class energy_wifi {

  public static void main(String[] args)
  {

    var engine = new Engine(args);
    engine.plugin_wifi_energy_init();
    engine.load_platform(args[0]);

    // setup WiFi bandwidths
    var l = engine.link_by_name("AP1");
    l.set_host_wifi_rate(engine.host_by_name("Station 1"), 0);
    l.set_host_wifi_rate(engine.host_by_name("Station 2"), 0);

    // create the two actors for the test
    engine.host_by_name("Station 1").add_actor("act0", new Sender());
    engine.host_by_name("Station 2").add_actor("act1", new Receiver());

    engine.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    engine.force_garbage_collection();
  }
}
