/* Copyright (c) 2010-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to use Activity.await() to wait for a given communication. */

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
    double sleep_start_time = 5.0;
    double sleep_test_time  = 0;

    Mailbox mbox = this.get_engine().mailbox_by_name("receiver");

    Engine.info("sleep_start_time : %f , sleep_test_time : %f", sleep_start_time, sleep_test_time);
    sleep_for(sleep_start_time);

    for (int i = 0; i < messages_count; i++) {
      String payload = "Message " + i;

      /* Create a communication representing the ongoing communication and then */
      Comm comm = mbox.put_async(payload, payload_size);
      Engine.info("Send '%s' to '%s'", payload, mbox.get_name());

      if (sleep_test_time > 0) { /* - "test_time" is set to 0, wait */
        while (!comm.test()) {   /* - Call test() every "sleep_test_time" otherwise */
          sleep_for(sleep_test_time);
        }
      } else {
        comm.await();
      }
    }

    /* Send message to let the receiver know that it should stop */
    Engine.info("Send 'finalize' to 'receiver'");
    mbox.put("finalize", 0);
  }
}

class receiver extends Actor {
  public void run()
  {
    double sleep_start_time = 1.0;
    double sleep_test_time  = 0.1;

    Mailbox mbox = this.get_engine().mailbox_by_name("receiver");

    Engine.info("sleep_start_time : %f , sleep_test_time : %f", sleep_start_time, sleep_test_time);
    sleep_for(sleep_start_time);

    Engine.info("Wait for my first message");
    for (boolean more_messages = true; more_messages;) { // While we expect more messages

      Comm comm = mbox.get_async();
      // Such an active loop is possible but inefficient. If you don't have anything to do meanwhile, just use await()
      while (!comm.test()) {
        sleep_for(sleep_test_time);
        // I could deal with other things while waiting for the message to come
      }

      String received = (String)comm.get_payload();
      Engine.info("I got a '%s'.", received);
      if (received.equals("finalize"))
        more_messages = false; // If it's a finalize message, we're done.
    }
  }
}

public class comm_await {
  public static void main(String[] args)
  {
    var e = new Engine(args);

    e.load_platform(args[0]);

    e.add_actor("sender", e.host_by_name("Tremblay"), new sender(3, 482117300));
    e.add_actor("receiver", e.host_by_name("Ruby"), new receiver());

    e.run();
  }
}
