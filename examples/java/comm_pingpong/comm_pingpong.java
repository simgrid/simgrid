/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;;

class Pinger extends Actor {
  Mailbox mailboxIn;
  Mailbox mailboxOut;
  public Pinger(Mailbox mailboxIn, Mailbox mailboxOut)
  {
    this.mailboxIn  = mailboxIn;
    this.mailboxOut = mailboxOut;
  }
  @Override public void run() throws SimgridException
  {
    Engine.info("Ping from mailbox " + mailboxIn.get_name() + " to mailbox " + mailboxOut.get_name());

    /* - Do the ping with a 1-Byte payload (latency bound) ... */
    Double payload = Engine.get_clock();

    mailboxOut.put(payload, 1);
    /* - ... then wait for the (large) pong */
    Double sender_time = (Double)mailboxIn.get();

    double communication_time = Engine.get_clock() - sender_time;
    Engine.info("Payload received : large communication (bandwidth bound)");
    Engine.info("Pong time (bandwidth bound): "+ communication_time);
  }
}

class Ponger extends Actor {
  Mailbox mailboxIn;
  Mailbox mailboxOut;
  public Ponger(Mailbox mailboxIn, Mailbox mailboxOut)
  {
    this.mailboxIn  = mailboxIn;
    this.mailboxOut = mailboxOut;
  }
  @Override public void run() throws SimgridException
  {
    Engine.info("Pong from mailbox " + mailboxIn.get_name() + " to mailbox " + mailboxOut.get_name());

    /* - Receive the (small) ping first ....*/
    Double senderTime        = (Double)mailboxIn.get();
    double communicationTime = Engine.get_clock() - senderTime;
    Engine.info("Payload received : small communication (latency bound)");
    Engine.info("Ping time (latency bound) " + communicationTime);

    /*  - ... Then send a 1GB pong back (bandwidth bound) */
    Double payload = Engine.get_clock();
    Engine.info("payload = " + payload);

    mailboxOut.put(payload, (long)1e9);
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

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
