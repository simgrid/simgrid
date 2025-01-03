/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class test_detach extends Actor {
  int size;
  test_detach(String name, Host location, double size)
  {
    super(name, location);
    this.size = (int)size;
  }
  public void run()
  {
    Disk disk = Host.current().get_disks()[0];
    this.sleep_for(0.2);
    Engine.info("Hello! read %d bytes from %s", size, disk.get_name());

    Io activity = disk.io_init(size, Io.OpType.READ);
    // Attach a callback to print some log when the detached activity completes
    activity.on_this_completion_cb(new CallbackIo() {
      @Override public void run(Io io)
      {
        Engine.info("Detached activity is done");
      }
    });
    activity.detach();

    Engine.info("Goodbye now!");
  }
}

class test extends Actor {
  int size;
  test(String name, Host location, double size)
  {
    super(name, location);
    this.size = (int)size;
  }
  public void run() throws SimgridException
  {
    Disk disk = Host.current().get_disks()[0];
    Engine.info("Hello! read %d bytes from %s", size, disk.get_name());

    Io activity = disk.io_init(size, Io.OpType.READ);
    activity.start();
    activity.await();

    Engine.info("Goodbye now!");
  }
}

class test_waitfor extends Actor {
  int size;
  test_waitfor(String name, Host location, double size)
  {
    super(name, location);
    this.size = (int)size;
  }
  public void run()
  {
    Disk disk = Host.current().get_disks()[0];
    Engine.info("Hello! write %d bytes from %s", size, disk.get_name());

    Io activity = disk.write_async(size);
    try {
      activity.await_for(0.5);
    } catch (TimeoutException e) {
      Engine.info("Asynchronous write: Timeout!");
    }

    Engine.info("Goodbye now!");
  }
}

class test_cancel extends Actor {
  int size;
  test_cancel(String name, Host location, double size)
  {
    super(name, location);
    this.size = (int)size;
  }
  public void run()
  {
    Disk disk = Host.current().get_disks()[0];
    this.sleep_for(0.5);
    Engine.info("Hello! write %d bytes from %s", size, disk.get_name());

    Io activity = disk.write_async(size);
    this.sleep_for(0.5);
    Engine.info("I changed my mind, cancel!");
    activity.cancel();

    Engine.info("Goodbye now!");
  }
}

class test_monitor extends Actor {
  int size;
  test_monitor(String name, Host location, double size)
  {
    super(name, location);
    this.size = (int)size;
  }
  public void run() throws SimgridException
  {
    Disk disk = Host.current().get_disks()[0];
    this.sleep_for(1);
    Io activity = disk.write_async(size);

    while (!activity.test()) {
      Engine.info("Remaining amount of bytes to write: %g", activity.get_remaining());
      this.sleep_for(0.2);
    }
    activity.await();

    Engine.info("Goodbye now!");
  }
}

public class io_async {
  public static void main(String[] args)
  {
    var e = Engine.get_instance(args);

    e.load_platform(args[0]);
    new test("test", Host.by_name("bob"), 2e7);
    new test_detach("test_detach", Host.by_name("bob"), 2e7);
    new test_waitfor("test_waitfor", e.host_by_name("alice"), 5e7);
    new test_cancel("test_cancel", e.host_by_name("alice"), 5e7);
    new test_monitor("test_monitor", e.host_by_name("alice"), 5e7);

    e.run();

    Engine.info("Simulation ends.");
  }
}
