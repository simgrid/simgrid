/* Copyright (c) 2021-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This examples shows how to survive to host failure exceptions that occur when an host is turned off.
 *
 * The actors do not get notified when the host on which they run is turned off: they are just terminated
 * in this case, and their on_exit() callback gets executed.
 *
 * For remote executions on failing hosts however, any blocking operation such as exec or await will
 * raise an exception that you can catch and react to, as illustrated in this example.
 *
 * TODO: the Java bindings do not expose Host.set_state_profile()/ProfileBuilder, so unlike the C++ version, Host2's
 * failure at t=12 (and recovery at t=20) is not driven by a profile. Instead, a second HostToggler actor plays
 * the exact same role explicitly, using the same turn_off()/turn_on() primitives the HostKiller actor already
 * uses for Host1: the observable simulation behavior is identical.
 */

import java.util.ArrayList;
import java.util.List;
import org.simgrid.s4u.*;

class Dispatcher extends Actor {
  Host[] hosts;
  public Dispatcher(Host[] hosts) { this.hosts = hosts; }

  public void run() throws SimgridException
  {
    List<Exec> pending_execs = new ArrayList<>();
    for (Host host : hosts) {
      Engine.info("Initiating asynchronous exec on %s", host.get_name());
      // Computing 20 flops on an host which speed is 1f takes 20 seconds (when it does not fail)
      Exec exec = this.exec_init(20).set_host(host);
      pending_execs.add(exec);
      exec.start();
    }

    Engine.info("---------------------------------");
    Engine.info("Wait on the first exec, which host is turned off at t=10 by the other actor.");
    try {
      pending_execs.get(0).await();
      Engine.die("This wait was not supposed to succeed.");
    } catch (HostFailureException e) {
      Engine.info("Dispatcher has experienced a host failure exception, so it knows that something went wrong.");
    }

    Engine.info("State of each exec:");
    for (Exec exec : pending_execs)
      Engine.info("  Exec on %s has state: %s", exec.get_host().get_name(), exec.get_state_str());

    Engine.info("---------------------------------");
    Engine.info("Wait on the second exec, which host is turned off at t=12 by the other actor.");
    try {
      pending_execs.get(1).await();
      Engine.die("This wait was not supposed to succeed.");
    } catch (HostFailureException e) {
      Engine.info("Dispatcher has experienced a host failure exception, so it knows that something went wrong.");
    }
    Engine.info("State of each exec:");
    for (Exec exec : pending_execs)
      Engine.info("  Exec on %s has state: %s", exec.get_host().get_name(), exec.get_state_str());

    Engine.info("---------------------------------");
    Engine.info("Wait on the third exec, which should succeed.");
    try {
      pending_execs.get(2).await();
      Engine.info("No exception occured.");
    } catch (HostFailureException e) {
      Engine.die("This wait was not supposed to fail.");
    }
    Engine.info("State of each exec:");
    for (Exec exec : pending_execs)
      Engine.info("  Exec on %s has state: %s", exec.get_host().get_name(), exec.get_state_str());
  }
}

class HostKiller extends Actor {
  Host to_kill;
  public HostKiller(Host to_kill) { this.to_kill = to_kill; }

  public void run() throws SimgridException
  {
    this.sleep_for(10.0);
    Engine.info("HostKiller turns off the host '%s'.", to_kill.get_name());
    to_kill.turn_off();
  }
}

/* Plays the same role the state profile attached to Host2 plays in the C++ version: turns Host2 off at t=12,
 * then back on at t=20. */
class HostToggler extends Actor {
  Host to_toggle;
  public HostToggler(Host to_toggle) { this.to_toggle = to_toggle; }

  public void run() throws SimgridException
  {
    this.sleep_for(12.0);
    Engine.info("HostToggler turns off the host '%s'.", to_toggle.get_name());
    to_toggle.turn_off();
    this.sleep_for(8.0); // wait until t=20
    Engine.info("HostToggler turns the host '%s' back on.", to_toggle.get_name());
    to_toggle.turn_on();
  }
}

public class exec_failure {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    NetZone zone   = e.get_netzone_root();
    Host[] hosts   = new Host[3];
    String[] names = {"Host1", "Host2", "Host3"};
    for (int i = 0; i < names.length; i++)
      hosts[i] = zone.add_host(names[i], "1f");

    zone.seal();

    hosts[2].add_actor("Dispatcher", new Dispatcher(hosts));
    hosts[2].add_actor("HostKiller", new HostKiller(hosts[0]));
    hosts[2].add_actor("HostToggler", new HostToggler(hosts[1]));

    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
