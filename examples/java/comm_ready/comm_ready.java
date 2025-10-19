/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

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
  int myId;
  int messagesCount;
  int payloadSize;
  int peersCount;
  peer(int myId_, int messagesCount_, double payloadSize_, int peersCount_)
  {
    myId          = myId_;
    messagesCount = messagesCount_;
    payloadSize   = (int)payloadSize_;
    peersCount    = peersCount_;
  }
  public void run() throws SimgridException
  {
    Engine e = this.get_engine();
    /* Set myself as the persistent receiver of my mailbox so that messages start flowing to me as soon as they are put
     * into it */
    Mailbox myMbox = e.mailbox_by_name("peer-" + myId);
    myMbox.set_receiver(Actor.self());

    ActivitySet pending_comms = new ActivitySet();

    /* Start dispatching all messages to peers others that myself */
    for (int i = 0; i < messagesCount; i++) {
      for (int peer_id = 0; peer_id < peersCount; peer_id++) {
        if (peer_id != myId) {
          Mailbox mbox   = e.mailbox_by_name("peer-" + peer_id);
          String payload = "Message " + i + " from peer " + myId;
          Engine.info("Send '" + payload + "' to '" + mbox.get_name() + "'");
          /* Create a communication representing the ongoing communication */
          pending_comms.push(mbox.put_async(payload, payloadSize));
        }
      }
    }

    /* Start sending messages to let peers know that they should stop */
    for (int peer_id = 0; peer_id < peersCount; peer_id++) {
      if (peer_id != myId) {
        Mailbox mbox   = e.mailbox_by_name("peer-" + peer_id);
        String payload = "finalize";
        pending_comms.push(mbox.put_async(payload, payloadSize));
        Engine.info("Send 'finalize' to 'peer-" + peer_id + "'");
      }
    }
    Engine.info("Done dispatching all messages");

    /* Retrieve all the messages other peers have been sending to me until I receive all the corresponding "Finalize"
     * messages */
    long pendingFinalizeMessages = peersCount - 1;
    while (pendingFinalizeMessages > 0) {
      if (myMbox.ready()) {
        double start        = Engine.get_clock();
        String received     = (String)myMbox.get();
        double waitingTime  = Engine.get_clock() - start;
        if (waitingTime > 0)
          Engine.die("Expecting the waiting time to be 0 because the communication was supposedly ready, but got " +
                     waitingTime + " instead");
        Engine.info("I got a '" + received + "'.");

        if (received.equals("finalize"))
          pendingFinalizeMessages--;

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
    Engine e = new Engine(args);
    e.load_platform(args[0]);
    e.host_by_name("Tremblay").add_actor("peer", new peer(0, 2, 5e7, 3));
    e.host_by_name("Ruby").add_actor("peer", new peer(1, 6, 2.5e5, 3));
    e.host_by_name("Perl").add_actor("peer", new peer(2, 0, 5e7, 3));

    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}