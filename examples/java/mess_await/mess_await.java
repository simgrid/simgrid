/* Copyright (c) 2023-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to use message queues. It is very similar to the comm_await test, but with message queues :)
 */

import org.simgrid.s4u.*;

class Sender extends Actor {
  int messages_count;
  public Sender(String name, Host location, int messages_count)
  {
    super(name, location);
    this.messages_count = messages_count;
  }
  public void run()
  {
    MessageQueue mqueue = MessageQueue.by_name("control");

    this.sleep_for(0.5);

    Engine.info("Send 'hello' to 'receiver'");
    mqueue.put(new String("hello"));

    for (int i = 0; i < messages_count; i++) {
      String payload = "Message " + i;

      /* Create a control message and put it in the message queue */
      Mess mess = mqueue.put_async(payload);
      Engine.info("Send '%s' to '%s'", payload, mqueue.get_name());
      mess.await();
    }

    /* Send message to let the receiver know that it should stop */
    Engine.info("Send 'finalize' to 'receiver'");
    mqueue.put(new String("finalize"));
  }
}

/* Receiver actor expects no argument */
class Receiver extends Actor {
  Receiver(String name, Host location) { super(name, location); }
  public void run()
  {
    var mqueue = MessageQueue.by_name("control");
    this.sleep_for(1);

    Mess hello = mqueue.get_async();
    hello.await();
    var msg = (String)hello.get_payload();
    Engine.info("I got a '%s'.", msg);

    Engine.info("Await for my first message");
    for (boolean cont = true; cont;) {
      Mess mess = mqueue.get_async();

      this.sleep_for(0.1);
      mess.await();
      String received = (String)mess.get_payload();

      Engine.info("I got a '%s'.", received);
      if (received.equals("finalize"))
        cont = false; // If it's a finalize message, we're done.
    }
  }
}

public class mess_await {
  public static void main(String[] args)
  {
    var e = new Engine(args);

    e.load_platform(args[0]);

    new Sender("sender", e.host_by_name("Tremblay"), 3);
    new Receiver("receiver", e.host_by_name("Fafard"));

    e.run();
  }
}
