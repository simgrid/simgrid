/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class Message {
  String sender;
  int size;
  Message(String sender, int size)
  {
    this.sender = sender;
    this.size   = size;
  }
}

class WifiSender extends Actor {
  String mailbox;
  int msg_size;
  int sleep_time;

  public WifiSender(String mailbox, int msg_size, int sleep_time)
  {
    this.mailbox    = mailbox;
    this.msg_size   = msg_size;
    this.sleep_time = sleep_time;
  }

  public void run() throws SimgridException
  {
    this.sleep_for(sleep_time);
    Mailbox mbox = this.get_engine().mailbox_by_name(mailbox);
    Message msg  = new Message(this.get_host().get_name(), msg_size);
    mbox.put(msg, msg_size);
  }
}

class WifiReceiver extends Actor {
  String mailbox;

  public WifiReceiver(String mailbox) { this.mailbox = mailbox; }

  public void run() throws SimgridException
  {
    Mailbox mbox = this.get_engine().mailbox_by_name(mailbox);
    Message msg  = (Message)mbox.get();
    Engine.info("[%s] %s received %d bytes from %s", mailbox, this.get_host().get_name(), msg.size, msg.sender);
  }
}

public class network_ns3_wifi {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    e.load_platform(args[0]);
    int msg_size = (int)1e5;

    /* Communication between STA in the same wifi zone */
    e.host_by_name("STA0-0").add_actor("sender", new WifiSender("1", msg_size, 10));
    e.host_by_name("STA0-1").add_actor("receiver", new WifiReceiver("1"));
    e.host_by_name("STA0-1").add_actor("sender", new WifiSender("2", msg_size, 20));
    e.host_by_name("STA0-0").add_actor("receiver", new WifiReceiver("2"));
    e.host_by_name("STA1-1").add_actor("sender", new WifiSender("3", msg_size, 30));
    e.host_by_name("STA1-2").add_actor("receiver", new WifiReceiver("3"));
    e.host_by_name("STA1-2").add_actor("sender", new WifiSender("4", msg_size, 40));
    e.host_by_name("STA1-1").add_actor("receiver", new WifiReceiver("4"));

    /* Communication between STA of different wifi zones */
    e.host_by_name("STA0-0").add_actor("sender", new WifiSender("5", msg_size, 50));
    e.host_by_name("STA1-0").add_actor("receiver", new WifiReceiver("5"));
    e.host_by_name("STA1-0").add_actor("sender", new WifiSender("6", msg_size, 60));
    e.host_by_name("STA0-0").add_actor("receiver", new WifiReceiver("6"));
    e.host_by_name("STA0-1").add_actor("sender", new WifiSender("7", msg_size, 70));
    e.host_by_name("STA1-2").add_actor("receiver", new WifiReceiver("7"));
    e.host_by_name("STA1-2").add_actor("sender", new WifiSender("8", msg_size, 80));
    e.host_by_name("STA0-1").add_actor("receiver", new WifiReceiver("8"));

    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.delete(); // We need a synchronous, same-thread native teardown for ns3
  }
}
