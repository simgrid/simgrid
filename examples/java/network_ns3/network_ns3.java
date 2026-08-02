/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import java.util.HashMap;
import java.util.Map;
import org.simgrid.s4u.*;

class MasterWorkerNames {
  String master;
  String worker;
  MasterWorkerNames(String master, String worker)
  {
    this.master = master;
    this.worker = worker;
  }
}

class Payload {
  double msg_size;
  double start_time;
  Payload(double msg_size, double start_time)
  {
    this.msg_size   = msg_size;
    this.start_time = start_time;
  }
}

class Master extends Actor {
  static Map<Integer, MasterWorkerNames> names = new HashMap<>();

  String[] args;
  public Master(String[] args) { this.args = args; }

  public void run() throws SimgridException
  {
    if (args.length != 3)
      Engine.die("Strange number of arguments, expected 3 got %d", args.length);

    Engine.debug("Master started");

    /* data size */
    double msg_size = Double.parseDouble(args[0]);
    int id          = Integer.parseInt(args[2]); // unique id to control statistics

    /* master and worker names */
    names.putIfAbsent(id, new MasterWorkerNames(this.get_host().get_name(), args[1]));

    Mailbox mbox = this.get_engine().mailbox_by_name(args[2]);

    Payload payload = new Payload(msg_size, Engine.get_clock());
    mbox.put(payload, (long)msg_size);

    Engine.debug("Finished");
  }
}

class Worker extends Actor {
  String[] args;
  public Worker(String[] args) { this.args = args; }

  public void run() throws SimgridException
  {
    if (args.length != 1)
      Engine.die("Strange number of arguments, expected 1 got %d", args.length);

    int id       = Integer.parseInt(args[0]);
    Mailbox mbox = this.get_engine().mailbox_by_name(args[0]);

    Engine.debug("Worker started");

    Payload payload = (Payload)mbox.get();

    double elapsed_time = Engine.get_clock() - payload.start_time;

    MasterWorkerNames mw = Master.names.get(id);
    Engine.info("FLOW[%d] : Receive %.0f bytes from %s to %s", id, payload.msg_size, mw.master, mw.worker);
    Engine.debug("FLOW[%d] : transferred in %f seconds", id, elapsed_time);

    Engine.debug("Finished");
  }
}

public class network_ns3 {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    if (args.length < 2)
      Engine.die("Usage: network_ns3 platform_file deployment_file\n"
                 + "\tExample: platform.xml deployment.xml\n");

    e.load_platform(args[0]);
    e.load_deployment(args[1]);

    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.delete(); // We need a synchronous, same-thread native teardown for ns3
  }
}
