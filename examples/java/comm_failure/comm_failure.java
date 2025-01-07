/* Copyright (c) 2021-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to react to a failed communication, which occurs when a link is turned off,
 * or when the actor with whom you communicate fails because its host is turned off.
 */

import org.simgrid.s4u.*;

class Sender extends Actor {
  String mailbox1_name;
  String mailbox2_name;

  public Sender(String name, Host location, String mailbox1_name_, String mailbox2_name_)
  {
    super(name, location);
    mailbox1_name = mailbox1_name_;
    mailbox2_name = mailbox2_name_;
  }

  public void run() throws SimgridException
  {
    var mailbox1     = Mailbox.by_name(mailbox1_name);
    var mailbox2     = Mailbox.by_name(mailbox2_name);
    Integer payload1 = 666;
    Integer payload2 = 888;

    Engine.info("Initiating asynchronous send to %s", mailbox1.get_name());
    var comm1 = mailbox1.put_async(payload1, 5);
    Engine.info("Initiating asynchronous send to %s", mailbox2.get_name());
    var comm2 = mailbox2.put_async(payload2, 2);

    Engine.info("Calling wait_any..");
    var pending_comms = new ActivitySet();
    pending_comms.push(comm1);
    pending_comms.push(comm2);
    try {
      Activity acti = pending_comms.await_any();
      Engine.info("Wait any returned comm to %s", ((Comm)acti).get_mailbox().get_name());
    } catch (NetworkFailureException e) {
      Engine.info("Sender has experienced a network failure exception, so it knows that something went wrong");
      Engine.info("Now it needs to figure out which of the two comms failed by looking at their state:");
      Engine.info("  Comm to %s has state: %s", comm1.get_mailbox().get_name(), comm1.get_state_str());
      Engine.info("  Comm to %s has state: %s", comm2.get_mailbox().get_name(), comm2.get_state_str());
    }

    try {
      comm1.await();
    } catch (NetworkFailureException e) {
      Engine.info("Waiting on a FAILED comm raises an exception: '%s'", e.getMessage());
    }
    Engine.info("Wait for remaining comm, just to be nice");
    pending_comms.await_all();
  }
};

class Receiver extends Actor {
  Mailbox mailbox;

  public Receiver(String name, Host location, String mailbox_name)
  {
    super(name, location);
    mailbox = Mailbox.by_name(mailbox_name);
  }

  public void run()
  {
    Engine.info("Receiver posting a receive...");
    try {
      Integer payload = (Integer)mailbox.get();
      Engine.info("Receiver has successfully received %d!", (int)payload);
    } catch (NetworkFailureException e) {
      Engine.info("Receiver has experience a network failure exception");
    }
  }
}
public class comm_failure {
  public static void main(String[] args)
  {
    var e     = new Engine(args);
    var zone  = e.set_rootzone_full("AS0");
    var host1 = zone.create_host("Host1", "1f");
    var host2 = zone.create_host("Host2", "1f");
    var host3 = zone.create_host("Host3", "1f");
    var link2 = zone.create_link("linkto2", "1bps").seal();
    var link3 = zone.create_link("linkto3", "1bps").seal();

    zone.add_route(host1, host2, new Link[] {link2});
    zone.add_route(host1, host3, new Link[] {link3});
    zone.seal();

    new Sender("Sender", host1, "mailbox2", "mailbox3");
    new Receiver("Receiver", host2, "mailbox2");
    new Receiver("Receiver", host3, "mailbox3");

    new Actor("LinkKiller", host1) {
      @Override public void run()
      {
        this.sleep_for(10.0);
        Engine.info("Turning off link 'linkto2'");
        Link.by_name("linkto2").turn_off();
      }
    };

    e.run();
  }
}
