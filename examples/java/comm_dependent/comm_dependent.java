/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class Sender extends Actor {
  Mailbox mailbox;
  public Sender(String name, Host location, Mailbox mb)
  {
    super(name, location);
    mailbox = mb;
  }
  public void run()
  {
    Double computation_amount = get_host().get_speed();

    Exec exec = exec_init(2 * computation_amount);
    Comm comm = mailbox.put_init(computation_amount, 7e6);

    exec.set_name("exec on sender").add_successor(comm).start();
    comm.set_name("comm to receiver").start();
    exec.await();
    comm.await();
  }
}
class Receiver extends Actor {
  Mailbox mailbox;
  public Receiver(String name, Host location, Mailbox mb)
  {
    super(name, location);
    mailbox = mb;
  }
  public void run()
  {
    Double computation_amount = get_host().get_speed();

    Exec exec = exec_init(2 * computation_amount);
    Comm comm = mailbox.get_init();

    comm.set_name("comm from sender").add_successor(exec).start();
    exec.set_name("exec on receiver").start();

    comm.await();
    exec.await();
    var received = (double)comm.get_payload();
    Engine.info("Received: " + (int)received + " flops were computed on sender");
  }
}

public class comm_dependent {

  public static void main(String[] args)
  {
    var e = new Engine(args);

    e.load_platform(args[0]);

    var mbox = e.mailbox_by_name_or_create("Mailbox");

    new Sender("sender", e.host_by_name("Tremblay"), mbox);
    new Receiver("receiver", e.host_by_name("Jupiter"), mbox);

    e.run();

    Engine.info("Simulation ends");
  }
}
