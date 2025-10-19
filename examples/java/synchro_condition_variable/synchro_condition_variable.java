/* Copyright (c) 2006-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class Worker extends Actor {
  ConditionVariable cv;
  Mutex mutex;
  public Worker(ConditionVariable cv, Mutex mutex)
  {
    this.cv    = cv;
    this.mutex = mutex;
  }

  public void run()
  {
    Engine.info("Start processing data which is '%s'.", Main.data);
    Main.data += " after processing";

    // Send data back to main()
    Engine.info("Signal to master that the data processing is completed, and exit.");

    Main.done = true;
    mutex.lock();
    cv.notify_one();
    mutex.unlock();
  }
}

class Main extends Actor {
  static String data  = "Example data";
  static boolean done = false;
  public void run()
  {
    Engine e             = this.get_engine();
    Mutex mutex          = Mutex.create();
    ConditionVariable cv = ConditionVariable.create();

    mutex.lock();
    Actor worker = e.host_by_name("Jupiter").add_actor("worker", new Worker(cv, mutex));

    // wait for the worker
    cv.await(mutex);
    mutex.unlock();
    Engine.info("data is now '%s'.", data);

    worker.join();
  }
}

public class synchro_condition_variable {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.load_platform(args[0]);
    e.host_by_name("Tremblay").add_actor("main", new Main());
    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
