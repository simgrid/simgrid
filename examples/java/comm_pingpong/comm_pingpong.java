/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;;

class Pinger extends Actor {
  Mailbox mailbox_in;
  Mailbox mailbox_out;
  public Pinger(Mailbox mailbox_in, Mailbox mailbox_out)
  {
    this.mailbox_in = mailbox_in;
    this.mailbox_out = mailbox_out;
  }
  @Override public void run() throws SimgridException
  {
    Engine.info("Ping from mailbox "+mailbox_in.get_name()+" to mailbox "+mailbox_out.get_name());

    /* - Do the ping with a 1-Byte payload (latency bound) ... */
    Double payload = Engine.get_clock();

    mailbox_out.put(payload, 1);
    /* - ... then wait for the (large) pong */
    Double sender_time = (Double)mailbox_in.get();

    double communication_time = Engine.get_clock() - sender_time;
    Engine.info("Payload received : large communication (bandwidth bound)");
    Engine.info("Pong time (bandwidth bound): "+ communication_time);
  }
}

class Ponger extends Actor {
  Mailbox mailbox_in;
  Mailbox mailbox_out;
  public Ponger(Mailbox mailbox_in, Mailbox mailbox_out)
  {
    this.mailbox_in = mailbox_in;
    this.mailbox_out = mailbox_out;
  }
  @Override public void run() throws SimgridException
  {
    Engine.info("Pong from mailbox "+mailbox_in.get_name()+" to mailbox "+mailbox_out.get_name());

    /* - Receive the (small) ping first ....*/
    Double sender_time        = (Double)mailbox_in.get();
    double communication_time = Engine.get_clock() - sender_time;
    Engine.info("Payload received : small communication (latency bound)");
    Engine.info("Ping time (latency bound) " + communication_time);
  
    /*  - ... Then send a 1GB pong back (bandwidth bound) */
    Double payload = Engine.get_clock();
    Engine.info("payload = " + payload);
  
    mailbox_out.put(payload, (long)1e9);
  }
}

public class comm_pingpong {
  
  public static void main(String[] args) {

    Engine e = new Engine(args);
    e.load_platform(args[0]);

    Mailbox mb1 = e.mailbox_by_name("Mailbox 1");
    Mailbox mb2 = e.mailbox_by_name("Mailbox 2");

    e.host_by_name("Tremblay").add_actor("pinger", new Pinger(mb1, mb2));
    e.host_by_name("Jupiter").add_actor("ponger", new Ponger(mb2, mb1));

    e.run();
    Engine.info("The simulation is terminating.");
  }
}
