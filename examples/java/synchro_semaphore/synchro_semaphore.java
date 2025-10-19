/* Copyright (c) 2006-2025. The SimGrid Team. All rights reserved.          */

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
  public producer(Semaphore sem_empty, Semaphore sem_full, Vector<String> args)
  {
    this.sem_empty = sem_empty;
    this.sem_full  = sem_full;
    this.args      = args;
  }

  public void run()
  {
    for (String str : args) {
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
  public consumer(Semaphore sem_empty, Semaphore sem_full)
  {
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
    Engine e = new Engine(args);
    e.load_platform(args.length >= 1 ? args[0] : "../../platforms/two_hosts.xml");

    Vector<String> params = new Vector<>(List.of("one", "two", "three", ""));

    Semaphore sem_empty = Semaphore.create(1); /* indicates whether the buffer is empty */
    Semaphore sem_full  = Semaphore.create(0); /* indicates whether the buffer is full */

    e.host_by_name("Tremblay").add_actor("producer", new producer(sem_empty, sem_full, params));
    e.host_by_name("Jupiter").add_actor("consumer", new consumer(sem_empty, sem_full));
    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
