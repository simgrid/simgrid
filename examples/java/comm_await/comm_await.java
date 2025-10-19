/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to use Activity.await() to wait for a given communication. */

import org.simgrid.s4u.*;

class sender extends Actor {
  int messagesCount;
  int payloadSize;

  public sender(int messagesCount, int payloadSize)
  {
    this.messagesCount = messagesCount;
    this.payloadSize   = payloadSize;
  }

  public void run() throws SimgridException
  {
    double sleepStartTime = 5.0;
    double sleepTestTime  = 0;

    Mailbox mbox = this.get_engine().mailbox_by_name("receiver");

    Engine.info("sleep_start_time : %f , sleep_test_time : %f", sleepStartTime, sleepTestTime);
    sleep_for(sleepStartTime);

    for (int i = 0; i < messagesCount; i++) {
      String payload = "Message " + i;

      /* Create a communication representing the ongoing communication and then */
      Comm comm = mbox.put_async(payload, payloadSize);
      Engine.info("Send '%s' to '%s'", payload, mbox.get_name());

      if (sleepTestTime > 0) {   /* - "test_time" is set to 0, wait */
        while (!comm.test()) {   /* - Call test() every "sleep_test_time" otherwise */
          sleep_for(sleepTestTime);
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
    double sleepStartTime = 1.0;
    double sleepTestTime  = 0.1;

    Mailbox mbox = this.get_engine().mailbox_by_name("receiver");

    Engine.info("sleep_start_time : %f , sleep_test_time : %f", sleepStartTime, sleepTestTime);
    sleep_for(sleepStartTime);

    Engine.info("Wait for my first message");
    for (boolean moreMessages = true; moreMessages;) { // While we expect more messages

      Comm comm = mbox.get_async();
      // Such an active loop is possible but inefficient. If you don't have anything to do meanwhile, just use await()
      while (!comm.test()) {
        sleep_for(sleepTestTime);
        // I could deal with other things while waiting for the message to come
      }

      String received = (String)comm.get_payload();
      Engine.info("I got a '%s'.", received);
      if (received.equals("finalize"))
        moreMessages = false; // If it's a finalize message, we're done.
    }
  }
}

public class comm_await {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    e.load_platform(args[0]);

    e.host_by_name("Tremblay").add_actor("sender", new sender(3, 482117300));
    e.host_by_name("Ruby").add_actor("receiver", new receiver());

    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
