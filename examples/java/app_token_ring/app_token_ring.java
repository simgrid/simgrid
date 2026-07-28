/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class RelayRunner extends Actor {
  public void run() throws SimgridException
  {
    long token_size = 1000000; /* The token is 1MB long */
    Mailbox my_mailbox;
    Mailbox neighbor_mailbox;
    int rank;

    try {
      rank = Integer.parseInt(this.get_name());
    } catch (NumberFormatException e) {
      throw new NumberFormatException("Actors of this example must have a numerical name, not " + this.get_name());
    }
    my_mailbox = this.get_engine().mailbox_by_name(String.valueOf(rank));
    if (rank + 1 == this.get_engine().get_host_count())
      /* The last actor sends the token back to rank 0 */
      neighbor_mailbox = this.get_engine().mailbox_by_name("0");
    else
      /* The others actors send to their right neighbor (rank+1) */
      neighbor_mailbox = this.get_engine().mailbox_by_name(String.valueOf(rank + 1));

    if (rank == 0) {
      /* The root actor (rank 0) first sends the token then waits to receive it back */
      Engine.info("Host \"%d\" send 'Token' to Host \"%s\"", rank, neighbor_mailbox.get_name());
      String msg = "Token";
      neighbor_mailbox.put(msg, token_size);
      String res = (String)my_mailbox.get();
      Engine.info("Host \"%d\" received \"%s\"", rank, res);
    } else {
      String res = (String)my_mailbox.get();
      Engine.info("Host \"%d\" received \"%s\"", rank, res);
      Engine.info("Host \"%d\" send 'Token' to Host \"%s\"", rank, neighbor_mailbox.get_name());
      neighbor_mailbox.put(res, token_size);
    }
  }
}

public class app_token_ring {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    if (args.length < 1)
      Engine.die("Usage: %s platform.xml\n");
    e.load_platform(args[0]);

    Engine.info("Number of hosts '%d'", e.get_host_count());
    int id = 0;
    for (Host host : e.get_all_hosts()) {
      /* - Give a unique rank to each host and create a RelayRunner actor on each */
      host.add_actor(String.valueOf(id), new RelayRunner());
      id++;
    }
    e.run();
    Engine.info("Simulation time %g", Engine.get_clock());

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
