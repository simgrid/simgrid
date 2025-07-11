/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class Sender extends Actor {
  String mailbox;
  long msg_size;
  Sender(String mailbox, long msg_size)
  {
    this.mailbox  = mailbox;
    this.msg_size = msg_size;
  }
  public void run()
  {
    int payload = 42;
    this.get_engine().mailbox_by_name(mailbox).put(payload, msg_size);
  }
}

class Receiver extends Actor {
  String mailbox;
  Receiver(String mailbox) { this.mailbox = mailbox; }
  public void run() throws NetworkFailureException { this.get_engine().mailbox_by_name(mailbox).get(); }
}

class execute_load_test extends Actor {
  static void run_transfer(Engine e, Host src_host, Host dst_host, String mailbox, long msg_size)
  {
    Engine.info("Launching the transfer of %d bytes", msg_size);
    src_host.add_actor("sender", new Sender(mailbox, msg_size));
    dst_host.add_actor("receiver", new Receiver(mailbox));
  }

  public void run()
  {
    Engine e  = this.get_engine();
    var host0 = e.host_by_name("node-0.simgrid.org");
    var host1 = e.host_by_name("node-1.simgrid.org");

    sleep_for(1);
    run_transfer(e, host0, host1, "1", 1000L * 1000 * 1000);

    sleep_for(10);
    run_transfer(e, host0, host1, "2", 1000L * 1000 * 1000);
    sleep_for(3);
    run_transfer(e, host0, host1, "3", 1000L * 1000 * 1000);
  }
}

class Monitor extends Actor {
  static void show_link_load(String link_name, Link link)
  {
    Engine.info("%s link load (cum, avg, min, max): (%g, %g, %g, %g)", link_name, link.load.get_cumulative(),
                link.load.get_average(), link.load.get_min_instantaneous(), link.load.get_max_instantaneous());
  }

  public void run()
  {
    Engine e          = this.get_engine();
    var link_backbone = e.link_by_name("cluster0_backbone");
    var link_host0    = e.link_by_name("cluster0_link_0_UP");
    var link_host1    = e.link_by_name("cluster0_link_1_DOWN");

    Engine.info("Tracking desired links");
    link_backbone.load.track();
    link_host0.load.track();
    link_host1.load.track();

    show_link_load("Backbone", link_backbone);
    while (Engine.get_clock() < 5) {
      sleep_for(1);
      show_link_load("Backbone", link_backbone);
    }

    Engine.info("Untracking the backbone link");
    link_backbone.load.untrack();

    show_link_load("Host0_UP", link_host0);
    show_link_load("Host1_UP", link_host1);

    Engine.info("Now resetting and probing host links each second.");

    while (Engine.get_clock() < 29) {
      link_host0.load.reset();
      link_host1.load.reset();

      sleep_for(1);

      show_link_load("Host0_UP", link_host0);
      show_link_load("Host1_UP", link_host1);
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
  }
}
