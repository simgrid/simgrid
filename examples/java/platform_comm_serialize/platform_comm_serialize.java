/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to serialize a set of communications going through a link using Link::set_concurrency_limit()
 *
 * This example is very similar to the other asynchronous communication examples, but messages get serialized by the
 * platform. Without this call to Link::set_concurrency_limit(2) in main, all messages would be received at the exact
 * same timestamp since they are initiated at the same instant and are of the same size. But with this extra
 * configuration to the link, at most 2 messages can travel through the link at the same time.
 */

import org.simgrid.s4u.*;

class Sender extends Actor {
  int messages_count; /* - number of messages */
  int msg_size;       /* - message size in bytes */

  Sender(int size, int count)
  {
    this.messages_count = count;
    this.msg_size       = size;
  }
  public void run() throws SimgridException
  {
    /* ActivitySet in which we store all ongoing communications */
    ActivitySet pending_comms = new ActivitySet();

    /* Mailbox to use */
    Mailbox mbox = get_engine().mailbox_by_name("receiver");

    /* Start dispatching all messages to receiver */
    for (int i = 0; i < messages_count; i++) {
      String msg_content = "Message " + i;
      // Copy the data we send: the 'msg_content' variable is not a stable storage location.
      // It will be destroyed when this actor leaves the loop, ie before the receiver gets it
      var payload = new String(msg_content);

      Engine.info("Send '%s' to '%s'", msg_content, mbox.get_name());

      /* Create a communication representing the ongoing communication, and store it in pending_comms */
      Comm comm = mbox.put_async(payload, msg_size);
      pending_comms.push(comm);
    }

    Engine.info("Done dispatching all messages");

    /* Now that all message exchanges were initiated, wait for their completion in one single call */
    pending_comms.await_all();

    Engine.info("Goodbye now!");
  }
}

/* Receiver actor expects 1 argument: number of messages to be received */
class Receiver extends Actor {
  Mailbox mbox;
  int messages_count = 10; /* - number of messages */

  Receiver(int count) { messages_count = count; }
  public void run() throws SimgridException
  {
    mbox = get_engine().mailbox_by_name("receiver");
    /* Where we store all incoming msgs */
    ActivitySet pending_comms = new ActivitySet();

    Engine.info("Wait for %d messages asynchronously", messages_count);
    for (int i = 0; i < messages_count; i++) {
      var comm = mbox.get_async();
      pending_comms.push(comm);
    }

    while (!pending_comms.empty()) {
      var completed_one = pending_comms.await_any();
      if (completed_one != null) {
        var comm = (Comm)completed_one;
        var msg  = comm.get_payload();
        Engine.info("I got '%s'.", msg);
      }
    }
  }
};

public class platform_comm_serialize {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    /* Creates the platform
     *  ________                 __________
     * | Sender |===============| Receiver |
     * |________|    Link1      |__________|
     */
    var zone     = e.get_netzone_root();
    var sender   = zone.add_host("sender", 1);
    var receiver = zone.add_host("receiver", 1);

    /* create split-duplex link1 (UP/DOWN), limiting the number of concurrent flows in it for 2 */
    var link = zone.add_split_duplex_link("link1", 10e9).set_latency(10e-6).set_concurrency_limit(2);

    /* create routes between nodes */
    zone.add_route(sender, receiver, new Link[] {link});
    zone.seal();

    /* create actors Sender/Receiver */
    receiver.add_actor("receiver", new Receiver(10));
    sender.add_actor("sender", new Sender((int)1e6, 10));

    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
