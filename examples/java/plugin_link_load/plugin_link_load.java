/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class Sender extends Actor {
  String mailbox;
  long msgSize;
  Sender(String mailbox, long msgSize)
  {
    this.mailbox  = mailbox;
    this.msgSize  = msgSize;
  }
  public void run()
  {
    int payload = 42;
    this.get_engine().mailbox_by_name(mailbox).put(payload, msgSize);
  }
}

class Receiver extends Actor {
  String mailbox;
  Receiver(String mailbox) { this.mailbox = mailbox; }
  public void run() throws NetworkFailureException { this.get_engine().mailbox_by_name(mailbox).get(); }
}

class execute_load_test extends Actor {
  static void runTransfer(Host srcHost, Host dstHost, String mailbox, long msgSize)
  {
    Engine.info("Launching the transfer of %d bytes", msgSize);
    srcHost.add_actor("sender", new Sender(mailbox, msgSize));
    dstHost.add_actor("receiver", new Receiver(mailbox));
  }

  public void run()
  {
    Engine e  = this.get_engine();
    var host0 = e.host_by_name("node-0.simgrid.org");
    var host1 = e.host_by_name("node-1.simgrid.org");

    sleep_for(1);
    runTransfer(host0, host1, "1", 1000L * 1000 * 1000);

    sleep_for(10);
    runTransfer(host0, host1, "2", 1000L * 1000 * 1000);
    sleep_for(3);
    runTransfer(host0, host1, "3", 1000L * 1000 * 1000);
  }
}

class Monitor extends Actor {
  static void showLinkLoad(String linkName, Link link)
  {
    Engine.info("%s link load (cum, avg, min, max): (%g, %g, %g, %g)", linkName, link.load.get_cumulative(),
                link.load.get_average(), link.load.get_min_instantaneous(), link.load.get_max_instantaneous());
  }

  public void run()
  {
    Engine e          = this.get_engine();
    var linkBackbone  = e.link_by_name("cluster0_backbone");
    var linkHost0     = e.link_by_name("cluster0_link_0_UP");
    var linkHost1     = e.link_by_name("cluster0_link_1_DOWN");

    Engine.info("Tracking desired links");
    linkBackbone.load.track();
    linkHost0.load.track();
    linkHost1.load.track();

    showLinkLoad("Backbone", linkBackbone);
    while (Engine.get_clock() < 5) {
      sleep_for(1);
      showLinkLoad("Backbone", linkBackbone);
    }

    Engine.info("Untracking the backbone link");
    linkBackbone.load.untrack();

    showLinkLoad("Host0_UP", linkHost0);
    showLinkLoad("Host1_UP", linkHost1);

    Engine.info("Now resetting and probing host links each second.");

    while (Engine.get_clock() < 29) {
      linkHost0.load.reset();
      linkHost1.load.reset();

      sleep_for(1);

      showLinkLoad("Host0_UP", linkHost0);
      showLinkLoad("Host1_UP", linkHost1);
    }
  }
}

class plugin_link_load {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    if (args.length == 1)
      Engine.die("Usage: %s platform_file\n\tExample: plugin_link_load ../platforms/cluster_backbone.xml\n");

    e.plugin_link_load_init();
    e.load_platform(args[0]);

    e.host_by_name("node-42.simgrid.org").add_actor("load_test", new execute_load_test());
    e.host_by_name("node-51.simgrid.org").add_actor("monitor", new Monitor());

    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
