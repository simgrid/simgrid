/* Copyright (c) 2006-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class Competitor extends Actor {
  int id;
  ConditionVariable cv;
  Mutex mtx;
  Competitor(int id, ConditionVariable cv, Mutex mtx)
  {
    this.id  = id;
    this.cv  = cv;
    this.mtx = mtx;
  }
  public void run()
  {
    Engine.info("Entering the race...");
    mtx.lock();
    while (!main_actor.ready) {
      var now = Engine.get_clock();
      try {
        cv.await_until(mtx, now + (id + 1) * 0.25);
        Engine.info("Out of wait_until (YAY!)");
      } catch (TimeoutException ex) {
        Engine.info("Out of wait_until (timeout)");
      }
    }
    Engine.info("Running!");
    mtx.unlock();
  }
}

class Go extends Actor {
  ConditionVariable cv;
  Mutex mtx;
  Go(ConditionVariable cv, Mutex mtx)
  {
    this.cv  = cv;
    this.mtx = mtx;
  }
  public void run()
  {
    Engine.info("Are you ready? ...");
    sleep_for(3);
    mtx.lock();
    Engine.info("Go go go!");
    main_actor.ready = true;
    cv.notify_all();
    mtx.unlock();
  }
}

class main_actor extends Actor {
  public static boolean ready = false;
  public void run()
  {
    var e   = get_engine();
    var mtx = Mutex.create();
    var cv  = ConditionVariable.create();

    var host = get_host();
    for (int i = 0; i < 10; ++i)
      host.add_actor("competitor", new Competitor(i, cv, mtx));
    host.add_actor("go", new Go(cv, mtx));
  }
}
public class synchro_condition_variable_awaituntil {
  public static void main(String[] args)
  {

    var e = new Engine(args);
    e.load_platform(args[0]);

    e.host_by_name("Tremblay").add_actor("main", new main_actor());

    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
