/* Copyright (c) 2006-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

// This example implements a simple producer/consumer schema,
// passing a bunch of items from one to the other

import java.util.List;
import java.util.Vector;
import org.simgrid.s4u.*;

class producer extends Actor {
  Semaphore sem_empty;
  Semaphore sem_full;
  Vector<String> args;
  public producer(String name, Host location, Semaphore sem_empty, Semaphore sem_full, Vector<String> args)
  {
    super(name, location);

    this.sem_empty = sem_empty;
    this.sem_full  = sem_full;
    this.args      = args;
  }

  public void run()
  {
    for (var str : args) {
      sem_empty.acquire();
      Engine.info("Pushing '%s'", str);
      synchro_semaphore.buffer = str;
      sem_full.release();
    }

    Engine.info("Bye!");
  }
}

class consumer extends Actor {
  Semaphore sem_empty;
  Semaphore sem_full;
  public consumer(String name, Host location, Semaphore sem_empty, Semaphore sem_full)
  {
    super(name, location);

    this.sem_empty = sem_empty;
    this.sem_full  = sem_full;
  }
  public void run()
  {
    String str;
    do {
      sem_full.acquire();
      str = synchro_semaphore.buffer;
      Engine.info("Receiving '%s'", str);
      sem_empty.release();
    } while (str != "");

    Engine.info("Bye!");
  }
}

public class synchro_semaphore {
  static public String buffer; /* Where the data is exchanged */

  public static void main(String[] args)
  {
    var e = Engine.get_instance(args);
    e.load_platform(args.length >= 1 ? args[0] : "../../platforms/two_hosts.xml");

    Vector<String> params = new Vector<>(List.of("one", "two", "three", ""));

    var sem_empty = Semaphore.create(1); /* indicates whether the buffer is empty */
    var sem_full  = Semaphore.create(0); /* indicates whether the buffer is full */

    new producer("producer", e.host_by_name("Tremblay"), sem_empty, sem_full, params).start();
    new consumer("consumer", e.host_by_name("Jupiter"), sem_empty, sem_full).start();
    e.run();
  }
}
