/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to use Activity.await_until() and
 * Activity.await_for() on a given communication.
 *
 * It is very similar to the comm_await example, but the sender initially
 * does some waits that are too short before doing an infinite wait.
 */

import java.util.Vector;
import org.simgrid.s4u.*;

class sender extends Actor {
  int messages_count;
  int payload_size;

  public sender(int messages_count_, int payload_size_)
  {
    messages_count = messages_count_;
    payload_size   = payload_size_;
  }
  public void run() throws SimgridException
  {
    Vector<Comm> pending_comms = new Vector<>();
    Mailbox mbox               = this.get_engine().mailbox_by_name("receiver-0");

    /* Start dispatching all messages to the receiver */
    for (int i = 0; i < messages_count; i++) {
      String payload = "Message " + i;

      // 'msgName' is not a stable storage location
      Engine.info("Send '%s' to '%s'", payload, mbox.get_name());
      /* Create a communication representing the ongoing communication */
      Comm comm = mbox.put_async(payload, payload_size);
      /* Add this comm to the vector of all known comms */
      pending_comms.add(comm);
    }

    /* Start the finalize signal to the receiver*/
    String payload = "finalize";

    Comm final_comm = mbox.put_async(payload, 0);
    pending_comms.add(final_comm);
    Engine.info("Send 'finalize' to 'receiver-0'");

    Engine.info("Done dispatching all messages");

    /* Now that all message exchanges were initiated, wait for their completion, in order of creation. */
    while (pending_comms.size() > 0) {
      Comm comm = pending_comms.remove(0);
      comm.await_for(1);
    }

    Engine.info("Goodbye now!");
  }
}
class receiver extends Actor {
  public void run() throws SimgridException
  {
    Mailbox mbox = this.get_engine().mailbox_by_name("receiver-0");

    Engine.info("Wait for my first message");
    for (boolean more_messages = true; more_messages;) {
      String received = (String)mbox.get();
      Engine.info("I got a '%s'.", received);
      if (received.equals("finalize"))
        more_messages = false; // If it's a finalize message, we're done.
    }
  }
}

public class comm_awaituntil {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    e.load_platform(args[0]);
    e.add_actor("sender", e.host_by_name("Tremblay"), new sender(3, (int)5e7));
    e.add_actor("receiver", e.host_by_name("Ruby"), new receiver());

    e.run();
  }
}
