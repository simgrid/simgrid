/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class wizard extends Actor {
  public void run() throws SimgridException
  {
    var e        = this.get_engine();
    Host fafard  = e.host_by_name("Fafard");
    Host ginette = e.host_by_name("Ginette");
    Host boivin  = e.host_by_name("Boivin");

    Engine.info("I'm a wizard! I can run an activity on the Ginette host from the Fafard one! Look!");
    Exec exec = this.exec_init(48.492e6);
    exec.set_host(ginette);
    exec.start();
    Engine.info("It started. Running 48.492Mf takes exactly one second on Ginette (but not on Fafard).");

    this.sleep_for(0.1);
    Engine.info("Loads in flops/s: Boivin=%.0f; Fafard=%.0f; Ginette=%.0f", boivin.get_load(), fafard.get_load(),
                ginette.get_load());

    exec.await();

    Engine.info("Done!");
    Engine.info("And now, harder. Start a remote activity on Ginette and move it to Boivin after 0.5 sec");
    exec = this.exec_init(73293500).set_host(ginette);
    exec.start();

    this.sleep_for(0.5);
    Engine.info("Loads before the move: Boivin=%.0f; Fafard=%.0f; Ginette=%.0f", boivin.get_load(), fafard.get_load(),
                ginette.get_load());

    exec.set_host(boivin);

    this.sleep_for(0.1);
    Engine.info("Loads after the move: Boivin=%.0f; Fafard=%.0f; Ginette=%.0f", boivin.get_load(), fafard.get_load(),
                ginette.get_load());

    exec.await();
    Engine.info("Done!");

    Engine.info("And now, even harder. Start a remote activity on Ginette and turn off the host after 0.5 sec");
    exec = this.exec_init(48.492e6).set_host(ginette);
    exec.start();

    this.sleep_for(0.5);
    ginette.turn_off();
    try {
      exec.await();
    } catch (HostFailureException ex) {
      Engine.info("Execution failed on %s", ginette.get_name());
    }
    Engine.info("Done!");
  }
}

public class exec_remote {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.load_platform(args[0]);
    e.host_by_name("Fafard").add_actor("test", new wizard());

    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
