/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class runner extends Actor {
  public void run() throws SimgridException
  {
    var e               = this.get_engine();
    Host multicore_host = e.host_by_name("MyHost1");
    // First test with less than, same number as, and more threads than cores
    double start_time = Engine.get_clock();
    this.thread_execute(multicore_host, 1e9, 2);
    Engine.info("Computed 2-thread activity on a 4-core host. Took %.1f s", Engine.get_clock() - start_time);

    start_time = Engine.get_clock();
    this.thread_execute(multicore_host, 1e9, 4);
    Engine.info("Computed 4-thread activity on a 4-core host. Took %.1f s", Engine.get_clock() - start_time);

    start_time = Engine.get_clock();
    this.thread_execute(multicore_host, 1e9, 6);
    Engine.info("Computed 6-thread activity on a 4-core host. Took %.1f s", Engine.get_clock() - start_time);

    Exec background_task = this.exec_async(2.5e9);
    Engine.info("Start a 1-core background task on the 4-core host.");

    start_time = Engine.get_clock();
    this.thread_execute(multicore_host, 1e9, 2);
    Engine.info("Computed 2-thread activity on a 4-core host. Took %.1f s", Engine.get_clock() - start_time);

    start_time = Engine.get_clock();
    this.thread_execute(multicore_host, 1e9, 4);
    Engine.info("Computed 4-thread activity on a 4-core host. Took %.1f s", Engine.get_clock() - start_time);

    background_task.await();
    Engine.info("The background task has ended.");

    background_task = this.exec_init(2e9).set_thread_count(4).start();
    Engine.info("Start a 4-core background task on the 4-core host.");

    Engine.info("Sleep for 5 seconds before starting another competing task");
    this.sleep_for(5);

    start_time = Engine.get_clock();
    this.execute(1e9);
    Engine.info("Computed 1-thread activity on a 4-core host. Took %.1f s", Engine.get_clock() - start_time);

    background_task.await();
    Engine.info("The background task has ended.");
  }
}

public class exec_threads {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    if (args.length == 0)
      Engine.die("Usage: exec_threads <platform file>");

    e.load_platform(args[0]);
    e.host_by_name("MyHost1").add_actor("test", new runner());

    e.run();
    Engine.info("Simulation done.");

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}