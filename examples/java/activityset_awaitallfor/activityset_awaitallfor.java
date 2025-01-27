/* Copyright (c) 2010-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class bob extends Actor {
  public void run() throws SimgridException
  {
    var e               = get_engine();
    Mailbox mbox        = e.mailbox_by_name("mbox");
    MessageQueue mqueue = e.message_queue_by_name("mqueue");
    Disk disk           = get_host().get_disks()[0];

    Engine.info("Create my asynchronous activities");
    ActivitySet pending_activities = new ActivitySet();
    pending_activities.push(this.exec_async(5e9));
    pending_activities.push(mbox.get_async());
    pending_activities.push(mqueue.get_async());
    pending_activities.push(disk.read_async(3e8));

    Engine.info("Wait for asynchronous activities to complete");
    while (!pending_activities.empty()) {
      try {
        pending_activities.await_all_for(1);
      } catch (TimeoutException ex) {
        Engine.info("Not all activities are terminated yet.");
      }
      Activity completed_one;
      while ((completed_one = pending_activities.test_any()) != null) {
        if (completed_one != null) {
          if (completed_one instanceof Comm)
            Engine.info("Completed a Comm");
          else if (completed_one instanceof Mess)
            Engine.info("Completed a Mess");
          else if (completed_one instanceof Exec)
            Engine.info("Completed an Exec");
          else if (completed_one instanceof Io)
            Engine.info("Completed an I/O");
        }
      }
    }
    Engine.info("Last activity is complete");
  }
}

class alice extends Actor {
  public void run()
  {
    String payload = "Message";
    Engine.info("Send '%s'", payload);
    get_engine().mailbox_by_name("mbox").put(payload, 6e8);
  }
}

class carl extends Actor {
  public void run()
  {
    this.sleep_for(1.99);
    String payload = "Control Message";
    Engine.info("Send '%s'", payload);
    get_engine().message_queue_by_name("mqueue").put(payload);
  }
}
public class activityset_awaitallfor {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    e.load_platform(args[0]);

    e.add_actor("bob", e.host_by_name("bob"), new bob());
    e.add_actor("alice", e.host_by_name("alice"), new alice());
    e.add_actor("carl", e.host_by_name("carl"), new carl());

    e.run();
  }
}
