/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import java.util.Random;
import java.util.Vector;
import org.simgrid.s4u.*;

class Sender extends Actor {
  int flow_amount;
  int comm_size;
  Sender(Vector<String> args)
  {
    if (args.size() != 2)
      Engine.die("The Sender actor expects 2 arguments.");
    flow_amount = Integer.parseInt(args.elementAt(0));
    comm_size   = Integer.parseInt(args.elementAt(1));
  }
  public void run() throws SimgridException
  {
    Engine.info("Send %d bytes, in %d flows", comm_size, flow_amount);
    Mailbox mailbox = get_engine().mailbox_by_name("message");

    /* Sleep a while before starting the example */
    sleep_for(10);

    if (flow_amount == 1) {
      /* - Send the task to the @ref worker */
      String payload = String.valueOf(comm_size);
      mailbox.put(payload, comm_size);
    } else {
      // Start all comms in parallel, and wait for all completions in one shot
      ActivitySet comms = new ActivitySet();
      for (int i = 0; i < flow_amount; i++)
        comms.push(mailbox.put_async(String.valueOf(i), comm_size));
      comms.await_all();
    }
    Engine.info("sender done.");
  }
}

class Receiver extends Actor {
  int flow_amount;
  Receiver(Vector<String> args)
  {
    if (args.size() != 1)
      Engine.die("The Receiver actor expects 1 arguments.");
    flow_amount = Integer.parseInt(args.elementAt(0));
  }

  public void run() throws SimgridException
  {
    Engine.info("Receiving %d flows ...", flow_amount);
    Mailbox mailbox = get_engine().mailbox_by_name("message");

    if (flow_amount == 1) {
      mailbox.get();
    } else {

      // Start all comms in parallel, and wait for their completion in one shot
      ActivitySet comms = new ActivitySet();
      for (int i = 0; i < flow_amount; i++)
        comms.push(mailbox.get_async());

      comms.await_all();
    }
    Engine.info("receiver done.");
  }
}

public class energy_link {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    Engine.info("Activating the SimGrid link energy plugin");
    e.plugin_link_energy_init();

    if (args.length < 1)
      Engine.die("\nUsage: java -cp energy_link.jar energy_link platform_file [flowCount [datasize]]\n"
                 + "\tExample: java -cp energy_link.jar energy_link platform.xml \n");
    e.load_platform(args[0]);
    args = e.get_args(); // Get the args cleaned from the SimGrid-specific parameters

    /* prepare to launch the actors */
    Vector<String> argSender   = new Vector<>();
    Vector<String> argReceiver = new Vector<>();
    if (args.length > 1) {
      argSender.add(args[1]); // Take the amount of flows from the command line
      argReceiver.add(args[1]);
    } else {
      argSender.add("1"); // Default value
      argReceiver.add("1");
    }

    if (args.length > 2) {
      if (args[2] == "random") { // We're asked to get a random size
        Random rand = new Random();

        /* Parameters of the random generation of the flow size */
        int min_size = (int)1e6;
        int max_size = (int)1e9;

        int size = rand.nextInt(max_size - min_size) + min_size;

        argSender.add(String.valueOf(size));
      } else {                  // Not "random" ? Then it should be the size to use
        argSender.add(args[2]); // Take the datasize from the command line
      }
    } else { // No parameter at all? Then use the default value
      argSender.add("25000");
    }
    e.host_by_name("MyHost1").add_actor("sender", new Sender(argSender));
    e.host_by_name("MyHost2").add_actor("receiver", new Receiver(argReceiver));

    /* And now, launch the simulation */
    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
