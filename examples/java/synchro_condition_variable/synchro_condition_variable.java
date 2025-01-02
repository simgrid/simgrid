/* Copyright (c) 2006-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class worker extends Actor {
  ConditionVariable cv;
  Mutex mutex;
  public worker(String name, Host location, ConditionVariable cv, Mutex mutex)
  {
    super(name, location);
    this.cv    = cv;
    this.mutex = mutex;
  }

  public void run()
  {
    Engine.info("Start processing data which is '%s'.", master.data);
    master.data += " after processing";

    // Send data back to main()
    Engine.info("Signal to master that the data processing is completed, and exit.");

    master.done = true;
    mutex.lock();
    cv.notify_one();
    mutex.unlock();
  }
}

class master extends Actor {
  public master(String name, Host location) { super(name, location); }
  static String data  = "Example data";
  static boolean done = false;
  public void run()
  {
    var mutex = Mutex.create();
    var cv    = ConditionVariable.create();

    mutex.lock();
    var worker = new worker("worker", Host.by_name("Jupiter"), cv, mutex).start();

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
    var e = Engine.get_instance(args);
    e.load_platform(args[0]);
    new master("main", e.host_by_name("Tremblay")).start();
    e.run();
  }
}
