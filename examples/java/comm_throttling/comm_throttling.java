/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class sender extends Actor {
  Mailbox mailbox;
  public sender(Mailbox mailbox_) { mailbox = mailbox_; }
  public void run() throws SimgridException
  {
    Engine.info("Send at full bandwidth");

    /* - First send a 2.5e8 Bytes payload at full bandwidth (1.25e8 Bps) */
    Double payload = Engine.get_clock();
    mailbox.put(payload, 2.5e8);

    Engine.info("Throttle the bandwidth at the Comm level");
    /* - ... then send it again but throttle the Comm */
    payload = Engine.get_clock();
    /* get a handler on the comm first */
    Comm comm = mailbox.put_init(payload, 2.5e8);

    /* let throttle the communication. It amounts to set the rate of the comm to half the nominal bandwidth of the link,
     * i.e., 1.25e8 / 2. This second communication will thus take approximately twice as long as the first one*/
    comm.set_rate(1.25e8 / 2).await();
  }
}

class receiver extends Actor {
  Mailbox mailbox;
  public receiver(Mailbox mailbox) { this.mailbox = mailbox; }
  public void run() throws SimgridException
  {
    /* - Receive the first payload sent at full bandwidth */
    Double senderTime        = (Double)mailbox.get();
    double communicationTime = Engine.get_clock() - senderTime;
    Engine.info(String.format("Payload received (full bandwidth) in %f seconds", communicationTime));

    /*  - ... Then receive the second payload sent with a throttled Comm */
    senderTime        = (Double)mailbox.get();
    communicationTime = Engine.get_clock() - senderTime;
    Engine.info(String.format("Payload received (throttled) in %f seconds", communicationTime));
  }
}

public class comm_throttling {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.load_platform(args[0]);

    Mailbox mbox = e.mailbox_by_name("Mailbox");

    e.host_by_name("node-0.simgrid.org").add_actor("sender", new sender(mbox));
    e.host_by_name("node-1.simgrid.org").add_actor("receiver", new receiver(mbox));

    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
