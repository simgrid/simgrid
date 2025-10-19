/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class Sender extends Actor {
  Mailbox mailbox;
  public Sender(Mailbox mb) { mailbox = mb; }
  public void run() throws SimgridException
  {
    Double computationAmount = get_host().get_speed();

    Exec exec = exec_init(2 * computationAmount);
    Comm comm = mailbox.put_init(computationAmount, 7e6);

    exec.set_name("exec on sender").add_successor(comm).start();
    comm.set_name("comm to receiver").start();
    exec.await();
    comm.await();
  }
}
class Receiver extends Actor {
  Mailbox mailbox;
  public Receiver(Mailbox mb) { mailbox = mb; }
  public void run() throws SimgridException
  {
    Double computationAmount = get_host().get_speed();

    Exec exec = exec_init(2 * computationAmount);
    Comm comm = mailbox.get_init();

    comm.set_name("comm from sender").add_successor(exec).start();
    exec.set_name("exec on receiver").start();

    comm.await();
    exec.await();
    double received = (double)comm.get_payload();
    Engine.info("Received: " + (int)received + " flops were computed on sender");
  }
}

public class comm_dependent {

  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    e.load_platform(args[0]);

    Mailbox mbox = e.mailbox_by_name("Mailbox");

    e.host_by_name("Tremblay").add_actor("sender", new Sender(mbox));
    e.host_by_name("Jupiter").add_actor("receiver", new Receiver(mbox));

    e.run();

    Engine.info("Simulation ends");

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
