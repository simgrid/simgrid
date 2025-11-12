/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to suspend and resume an asynchronous communication. */

import org.simgrid.s4u.*;

class sender extends Actor {
  public void run() throws SimgridException
  {
    Mailbox mbox = this.get_engine().mailbox_by_name("receiver");

    String payload = "Sent message";

    /* Create a communication representing the ongoing communication and then */
    Comm comm = mbox.putInit(payload, 13194230);
    Engine.info(String.format("Suspend the communication before it starts (remaining: %.0f bytes) and wait a second.",
                              comm.get_remaining()));
    sleep_for(1);
    Engine.info(String.format("Now, start the communication (remaining: %.0f bytes) and wait another second.",
                              comm.get_remaining()));
    comm.start();
    sleep_for(1);

    Engine.info(String.format("There is still %.0f bytes to transfer in this communication. Suspend it for one second.",
                              comm.get_remaining()));
    comm.suspend();
    Engine.info(String.format("Now there is %.0f bytes to transfer. Resume it and wait for its completion.",
                              comm.get_remaining()));
    comm.resume();
    comm.await();
    Engine.info(
        String.format("There is %f bytes to transfer after the communication completion.", comm.get_remaining()));
    Engine.info("Suspending a completed activity is a no-op.");
    comm.suspend();
  }
}

class receiver extends Actor {
  public void run() throws SimgridException
  {
    Mailbox mbox = this.get_engine().mailbox_by_name("receiver");
    Engine.info("Wait for the message.");
    String received = (String)mbox.get();

    Engine.info("I got '" + received + "'.");
  }
}

public class comm_suspend {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    e.load_platform(args[0]);
    e.host_by_name("Tremblay").add_actor("sender", new sender());
    e.host_by_name("Jupiter").add_actor("receiver", new receiver());

    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
