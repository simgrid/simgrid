/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This source code simply loads the platform. This is only useful to play
 * with the tracing module. See the tesh file to see how to generate the
 * traces.
 *
 * TODO: the Java bindings do not expose simgrid::instr::declare_tracing_category(), so unlike the C++ version,
 * the "migration_order" category used below is never explicitly declared (with a color) before being used. The
 * per-communication simgrid.s4u.Comm.set_tracing_category() call, which does the actual tagging, is available and
 * used as usual.
 */

import org.simgrid.s4u.*;

/* The guy we will move from host to host. It moves alone and then is moved by policeman back */
class Emigrant extends Actor {
  public void run() throws SimgridException
  {
    Mailbox mailbox = this.get_engine().mailbox_by_name("master_mailbox");

    this.sleep_for(2);

    while (true) { // I am an eternal emigrant
      String destination = (String)mailbox.get();
      if (destination.isEmpty())
        break; // there is no destination, die
      this.set_host(this.get_engine().host_by_name(destination));
      this.sleep_for(2); // I am tired, have to sleep for 2 seconds
    }
  }
}

class Policeman extends Actor {
  public void run() throws SimgridException
  {
    // I am the master of emigrant actor,
    // I tell it where it must emigrate to.
    String[] destinations = {"Tremblay", "Jupiter",  "Fafard",  "Ginette", "Bourassa",
                             "Fafard",   "Tremblay", "Ginette", ""};
    Mailbox mailbox       = this.get_engine().mailbox_by_name("master_mailbox");

    for (String destination : destinations) {
      Comm comm = mailbox.put_init(destination, 0);
      comm.set_tracing_category("migration_order");
      comm.await();
    }
  }
}

public class trace_process_migration {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    args     = e.get_args(); // Get ride of any configuration parameter on the command line

    if (args.length < 1)
      Engine.die("Usage: trace_process_migration platform_file\n \tExample: small_platform.xml\n");

    e.load_platform(args[0]);

    e.host_by_name("Fafard").add_actor("emigrant", new Emigrant());
    e.host_by_name("Tremblay").add_actor("policeman", new Policeman());

    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
