/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This simple example demonstrates the Comm.sento_init() Comm.sento_async() functions,
   that can be used to create a direct communication from one host to another without
   relying on the mailbox mechanism.

   There is not much to say, actually: The _init variant creates the communication and
   leaves it unstarted (in case you want to modify this communication before it starts),
   while the _async variant creates and start it. In both cases, you need to wait() it.

   It is mostly useful when you want to have a centralized simulation of your settings,
   with a central actor declaring all communications occurring on your distributed system.
  */

import org.simgrid.s4u.*;

class sender extends Actor {
  Host h1, h2, h3, h4;
  public sender(Host host1, Host host2, Host host3, Host host4)
  {
    h1 = host1;
    h2 = host2;
    h3 = host3;
    h4 = host4;
  }
  public void run() throws SimgridException
  {
    Engine.info("Send c12 with sendto_async(" + h1.get_name() + " -> " + h2.get_name() +
                "), and c34 with sendto_init(" + h3.get_name() + " -> " + h4.get_name() + ")");

    Comm c12 = Comm.sendto_async(h1, h2, 1.5e7); // Creates and start a direct communication
    Comm c34 = Comm.sendto_init(h3, h4);         // Creates but do not start another direct communication
    c34.set_payload_size(1e7);                  // Specify the amount of bytes to exchange in this comm

    // You can also detach() communications that you never plan to test() or wait().
    // Here we create a communication that only slows down the other ones
    Comm noise = Comm.sendto_init(h1, h2);
    noise.set_payload_size(10000);
    noise.detach();

    Engine.info("After creation,  c12 is " + c12.get_state_str() +
                " (remaining: " + String.format("%.2e", c12.get_remaining()) + " bytes); c34 is " +
                c34.get_state_str() + " (remaining: " + String.format("%.2e", c34.get_remaining()) + " bytes)");

    sleep_for(1);
    Engine.info("One sec later,   c12 is " + c12.get_state_str() +
                " (remaining: " + String.format("%.2e", c12.get_remaining()) + " bytes); c34 is " +
                c34.get_state_str() + " (remaining: " + String.format("%.2e", c34.get_remaining()) + " bytes)");

    c34.start();
    Engine.info("After c34.start, c12 is " + c12.get_state_str() +
                " (remaining: " + String.format("%.2e", c12.get_remaining()) + " bytes); c34 is " +
                c34.get_state_str() + " (remaining: " + String.format("%.2e", c34.get_remaining()) + " bytes)");

    c12.await();
    Engine.info("After c12.await, c12 is " + c12.get_state_str() +
                " (remaining: " + String.format("%.2e", c12.get_remaining()) + " bytes); c34 is " +
                c34.get_state_str() + " (remaining: " + String.format("%.2e", c34.get_remaining()) + " bytes)");

    c34.await();
    Engine.info("After c34.await, c12 is " + c12.get_state_str() +
                " (remaining: " + String.format("%.2e", c12.get_remaining()) + " bytes); c34 is " +
                c34.get_state_str() + " (remaining: " + String.format("%.2e", c34.get_remaining()) + " bytes)");

    /* As usual, you don't have to explicitly start communications that were just init()ed.
       The await() will start it automatically. */
    Comm c14 = Comm.sendto_init(h1, h4);
    c14.set_payload_size(100).await(); // Chaining 2 operations on this new communication
  }
}

public class comm_host2host {

  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    e.load_platform(args[0]);
    e.host_by_name("Boivin").add_actor("sender", 
                   new sender(e.host_by_name("Tremblay"), e.host_by_name("Jupiter"), e.host_by_name("Fafard"),
                              e.host_by_name("Ginette")));
    e.run();

    Engine.info("Simulation ends");

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
