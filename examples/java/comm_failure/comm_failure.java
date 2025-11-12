/* Copyright (c) 2021-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to react to a failed communication, which occurs when a link is turned off,
 * or when the actor with whom you communicate fails because its host is turned off.
 */

import org.simgrid.s4u.*;

class Sender extends Actor {
  String mailbox1Name;
  String mailbox2Name;

  public Sender(String mailbox1Name, String mailbox2Name)
  {
    this.mailbox1Name = mailbox1Name;
    this.mailbox2Name = mailbox2Name;
  }

  public void run() throws SimgridException
  {
    Engine e         = this.get_engine();
    Mailbox mailbox1 = e.mailbox_by_name(mailbox1Name);
    Mailbox mailbox2 = e.mailbox_by_name(mailbox2Name);
    Integer payload1 = 666;
    Integer payload2 = 888;

    Engine.info("Initiating asynchronous send to %s", mailbox1.get_name());
    Comm comm1 = mailbox1.put_async(payload1, 5);
    Engine.info("Initiating asynchronous send to %s", mailbox2.get_name());
    Comm comm2 = mailbox2.put_async(payload2, 2);

    Engine.info("Calling wait_any..");
    ActivitySet pendingComms = new ActivitySet();
    pendingComms.push(comm1);
    pendingComms.push(comm2);
    try {
      Activity acti = pendingComms.await_any();
      Engine.info("Wait any returned comm to %s", ((Comm)acti).get_mailbox().get_name());
    } catch (NetworkFailureException ex) {
      Engine.info("Sender has experienced a network failure exception, so it knows that something went wrong");
      Engine.info("Now it needs to figure out which of the two comms failed by looking at their state:");
      Engine.info("  Comm to %s has state: %s", comm1.get_mailbox().get_name(), comm1.get_state_str());
      Engine.info("  Comm to %s has state: %s", comm2.get_mailbox().get_name(), comm2.get_state_str());
    }

    try {
      comm1.await();
    } catch (NetworkFailureException ex) {
      Engine.info("Waiting on a FAILED comm raises an exception: '%s'", ex.getMessage());
    }
    Engine.info("Wait for remaining comm, just to be nice");
    pendingComms.await_all();
  }
}

class Receiver extends Actor {
  Mailbox mailbox;

  public Receiver(String mailbox_name) { mailbox = this.get_engine().mailbox_by_name(mailbox_name); }

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
    Engine e     = new Engine(args);
    NetZone zone = e.get_netzone_root();
    Host host1   = zone.add_host("Host1", "1f");
    Host host2   = zone.add_host("Host2", "1f");
    Host host3   = zone.add_host("Host3", "1f");
    Link link2   = zone.add_link("linkto2", "1bps").seal();
    Link link3   = zone.add_link("linkto3", "1bps").seal();

    zone.add_route(host1, host2, new Link[] {link2});
    zone.add_route(host1, host3, new Link[] {link3});
    zone.seal();

    host1.add_actor("Sender", new Sender("mailbox2", "mailbox3"));
    host2.add_actor("Receiver", new Receiver("mailbox2"));
    host3.add_actor("Receiver", new Receiver("mailbox3"));

    host1.add_actor("LinkKiller", new Actor() {
      @Override public void run()
      {
        this.sleep_for(10.0);
        Engine.info("Turning off link 'linkto2'");
        this.get_engine().link_by_name("linkto2").turn_off();
      }
    });

    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
