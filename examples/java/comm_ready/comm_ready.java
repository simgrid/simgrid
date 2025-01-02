/* Copyright (c) 2010-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to use Mailbox.ready() to check for completed communications.
 *
 * We have a number of peers which send and receive messages in two phases:
 * -> sending phase:   each one of them sends a number of messages to the others followed
 *                     by a single "finalize" message.
 * -> receiving phase: each one of them receives all the available messages that reached
 *                     their corresponding mailbox until it has all the needed "finalize"
 *                     messages to know that no more work needs to be done.
 *
 * To avoid doing an await() over the ongoing communications, each peer makes use of the
 * Mailbox.ready() method. If it returns true then a following get() will fetch the
 * message immediately, if not the peer will sleep for a fixed amount of time before checking again.
 *
 */
import org.simgrid.s4u.*;

class peer extends Actor {
  int my_id;
  int messages_count;
  int payload_size;
  int peers_count;
  peer(String name, Host location, int my_id_, int messages_count_, double payload_size_, int peers_count_)
  {
    super(name, location);
    my_id          = my_id_;
    messages_count = messages_count_;
    payload_size   = (int)payload_size_;
    peers_count    = peers_count_;
  }
  public void run()
  {
    /* Set myself as the persistent receiver of my mailbox so that messages start flowing to me as soon as they are put
     * into it */
    Mailbox my_mbox = Mailbox.by_name("peer-" + my_id);
    my_mbox.set_receiver(Actor.self());

    ActivitySet pending_comms = new ActivitySet();

    /* Start dispatching all messages to peers others that myself */
    for (int i = 0; i < messages_count; i++) {
      for (int peer_id = 0; peer_id < peers_count; peer_id++) {
        if (peer_id != my_id) {
          Mailbox mbox   = Mailbox.by_name("peer-" + peer_id);
          String payload = "Message " + i + " from peer " + my_id;
          Engine.info("Send '" + payload + "' to '" + mbox.get_name() + "'");
          /* Create a communication representing the ongoing communication */
          pending_comms.push(mbox.put_async(payload, payload_size));
        }
      }
    }

    /* Start sending messages to let peers know that they should stop */
    for (int peer_id = 0; peer_id < peers_count; peer_id++) {
      if (peer_id != my_id) {
        Mailbox mbox   = Mailbox.by_name("peer-" + peer_id);
        String payload = "finalize";
        pending_comms.push(mbox.put_async(payload, payload_size));
        Engine.info("Send 'finalize' to 'peer-" + peer_id + "'");
      }
    }
    Engine.info("Done dispatching all messages");

    /* Retrieve all the messages other peers have been sending to me until I receive all the corresponding "Finalize"
     * messages */
    long pending_finalize_messages = peers_count - 1;
    while (pending_finalize_messages > 0) {
      if (my_mbox.ready()) {
        double start        = Engine.get_clock();
        String received     = (String)my_mbox.get();
        double waiting_time = Engine.get_clock() - start;
        if (waiting_time > 0)
          Engine.die("Expecting the waiting time to be 0 because the communication was supposedly ready, but got " +
                     waiting_time + " instead");
        Engine.info("I got a '" + received + "'.");

        if (received.equals("finalize"))
          pending_finalize_messages--;

      } else {
        Engine.info("Nothing ready to consume yet, I better sleep for a while");
        sleep_for(.01);
      }
    }

    Engine.info("I'm done, just waiting for my peers to receive the messages before exiting");
    pending_comms.await_all();

    Engine.info("Goodbye now!");
  }
}

class comm_ready {
  public static void main(String[] args)
  {
    var e = Engine.get_instance(args);
    e.load_platform(args[0]);

    new peer("peer", e.host_by_name("Tremblay"), 0, 2, 5e7, 3).start();
    new peer("peer", e.host_by_name("Ruby"), 1, 6, 2.5e5, 3).start();
    new peer("peer", e.host_by_name("Perl"), 2, 0, 5e7, 3).start();

    e.run();
  }
}