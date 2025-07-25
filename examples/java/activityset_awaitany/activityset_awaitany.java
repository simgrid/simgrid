/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

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
    ActivitySet pendingActivities = new ActivitySet();
    pendingActivities.push(this.exec_async(5e9));
    pendingActivities.push(mbox.get_async());
    pendingActivities.push(mqueue.get_async());
    pendingActivities.push(disk.read_async(3e8));

    Engine.info("Wait for asynchronous activities to complete");
    while (!pendingActivities.empty()) {
      var completedOne = pendingActivities.await_any();
      if (completedOne != null) {
        if (completedOne instanceof Comm)
          Engine.info("Completed a Comm");
        else if (completedOne instanceof Mess)
          Engine.info("Completed a Mess");
        else if (completedOne instanceof Exec)
          Engine.info("Completed an Exec");
        else if (completedOne instanceof Io)
          Engine.info("Completed an I/O");
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
    this.sleep_for(2);
    String payload = "Control Message";
    Engine.info("Send '%s'", payload);
    get_engine().message_queue_by_name("mqueue").put(payload);
  }
}
public class activityset_awaitany {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    e.load_platform(args[0]);

    e.host_by_name("bob").add_actor("bob", new bob());
    e.host_by_name("alice").add_actor("alice", new alice());
    e.host_by_name("carl").add_actor("carl", new carl());

    e.run();
  }
}
