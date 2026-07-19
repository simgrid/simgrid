/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

import java.util.ArrayList;

/**
 * An actor is an independent stream of execution in your distributed application.
 *
 * It is located on a (simulated) host, but can interact with the whole simulated platform.
 *
 * You can think of an actor as a process in your distributed application, or as a thread in a multithreaded program.
 * This is the only component in SimGrid that actually does something on its own, executing its own code.
 * A resource will not get used if you don't schedule activities on them. This is the code of Actors that create and
 * schedule these activities.
 */
public abstract class Actor {
  protected Actor() {}
  /**
   * This is the method you should implement in your actor (it's the main of your actor).
   */
  public abstract void run() throws SimgridException; /* your code here */

  /* Internal method executing your code, and catching the actor-killing exception */
  public void do_run(long cPtr)
  {
    this.swigCPtr = cPtr;
    try {
      run();
    } catch (org.simgrid.s4u.ForcefulKillException e) {
      /* Actor killed, this is fine. */
    } catch (SimgridException e) {
      Engine.error("Actor killed by a %s exception: %s", e.getClass().getName(), e.getCause());
      e.printStackTrace();
    }
    fire_termination_signal(this);
  }

  private transient long swigCPtr;

  protected static long getCPtr(Actor obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  @SuppressWarnings({"deprecation", "removal"})
  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0)
      simgridJNI.delete_Actor(swigCPtr);
    swigCPtr = 0;
  }

  /** Retrieve a reference to the current actor */
  public static Actor self() { return simgridJNI.Actor_self(); }
  /** Returns the engine instance this actor is running in */
  public Engine get_engine() { return new Engine(simgridJNI.Engine_get_instance(), false); }

  /** Add a callback fired when this specific actor is suspended (right before the suspend) */
  public void on_this_suspend_cb(CallbackActor cb) { simgridJNI.Actor_on_this_suspend_cb(swigCPtr, this, cb); }
  /** Add a callback fired when this specific actor is resumed (right before the resume) */
  public void on_this_resume_cb(CallbackActor cb) { simgridJNI.Actor_on_this_resume_cb(swigCPtr, this, cb); }
  /** Add a callback fired when this specific actor starts sleeping */
  public void on_this_sleep_cb(CallbackActor cb) { simgridJNI.Actor_on_this_sleep_cb(swigCPtr, this, cb); }
  /** Add a callback fired when this specific actor wakes up from a sleep */
  public void on_this_wake_up_cb(CallbackActor cb) { simgridJNI.Actor_on_this_wake_up_cb(swigCPtr, this, cb); }

  /** Add a callback fired when this specific actor has been migrated to another host */
  public void on_this_host_change_cb(CallbackActorHost cb)
  {
    simgridJNI.Actor_on_this_host_change_cb(swigCPtr, this, cb);
  }

  /**
   * This actor will be automatically terminated when the last non-daemon actor finishes.
   *
   * Daemons are killed as soon as the last regular actor disappears. If another regular actor gets restarted later
   * on by a timer or when its host reboots, the daemons do not get restarted.
   */
  public Actor daemonize() {
    simgridJNI.Actor_daemonize(swigCPtr, this);
    return this;
  }

  /** Returns whether or not this actor has been daemonized */
  public boolean is_daemon() {
    return simgridJNI.Actor_is_daemon(swigCPtr, this);
  }

  /** Retrieves the name of this actor */
  public String get_name() {
    return simgridJNI.Actor_get_name(swigCPtr, this);
  }

  /** Retrieves the host on which this actor is running */
  public Host get_host() {
    long cPtr = simgridJNI.Actor_get_host(swigCPtr, this);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  /** Retrieves the actor ID (PID) of this actor */
  public int get_pid() {
    return simgridJNI.Actor_get_pid(swigCPtr, this);
  }

  /** Retrieves the actor ID (PID) of this actor's creator */
  public int get_ppid() {
    return simgridJNI.Actor_get_ppid(swigCPtr, this);
  }

  /** Suspend this actor, that is blocked until resume()d by another actor. */
  public void suspend() {
    simgridJNI.Actor_suspend(swigCPtr, this);
  }

  /** Resume this actor that was previously suspended. */
  public void resume() {
    simgridJNI.Actor_resume(swigCPtr, this);
  }

  /** Returns true if this actor is currently suspended. */
  public boolean is_suspended() {
    return simgridJNI.Actor_is_suspended(swigCPtr, this);
  }

  /**
   * Specify whether this actor shall restart when its host reboots.
   *
   * Some elements of the actor are remembered over reboots: name, host, properties, the on_exit functions, whether it
   * is daemonized and whether it should automatically restart when its host reboots. Note that the state after reboot
   * is the one when set_auto_restart() is called.
   *
   * If you daemonize this actor after marking it auto_restart, then the new actor after reboot will not be a daemon.
   */
  public Actor set_auto_restart(boolean autorestart) {
    simgridJNI.Actor_set_auto_restart(swigCPtr, this, autorestart);
    return this;
  }

  /** Equivalent to {@code set_auto_restart(true)}. */
  public Actor set_auto_restart() {
    simgridJNI.Actor_set_auto_restart(swigCPtr, this, true);
    return this;
  }

  /** Returns the number of reboots that this actor did. Before the first reboot, this function returns 0. */
  public int get_restart_count() {
    return simgridJNI.Actor_get_restart_count(swigCPtr, this);
  }

  /** Sets the time at which this actor should be killed */
  public void set_kill_time(double time) {
    simgridJNI.Actor_set_kill_time(swigCPtr, this, time);
  }

  /** Retrieves the time at which this actor will be killed (or -1 if not set) */
  public double get_kill_time() {
    return simgridJNI.Actor_get_kill_time(swigCPtr, this);
  }

  /**
   * Moves this actor to another host.
   *
   * If the actor is currently blocked on an execution activity, the activity is also migrated to the new host. If
   * it's blocked on another kind of activity, an error is raised as the mandated code is not written yet. Please
   * report that bug if you need it.
   *
   * Asynchronous activities started by the actor are not migrated automatically, so you have to take care of this
   * yourself (only you knows which ones should be migrated).
   */
  public void set_host(Host new_host) {
    simgridJNI.Actor_set_host(swigCPtr, this, Host.getCPtr(new_host), new_host);
  }

  /**
   * Ask this actor to die.
   *
   * Any blocking activity will be canceled, and it will be rescheduled to free its memory. Being killed is not
   * something that actors can defer or avoid.
   */
  public void kill() {
    simgridJNI.Actor_kill(swigCPtr, this);
  }

  /** Retrieves the actor that has the given PID (or null if not existing) */
  public static Actor by_pid(int pid) { return simgridJNI.Actor_by_pid(pid); }

  /**
   * Wait for this actor to finish.
   *
   * Blocks the calling actor until the joined actor is terminated. If actor alice executes bob.join(), then alice is
   * blocked until bob terminates.
   */
  public void join() {
    simgridJNI.Actor_join__SWIG_0(swigCPtr, this);
  }

  /**
   * Wait for this actor to finish, or for the timeout to elapse.
   *
   * Blocks the calling actor until the joined actor is terminated. If actor alice executes bob.join(), then alice is
   * blocked until bob terminates.
   */
  public void join(double timeout) {
    simgridJNI.Actor_join__SWIG_1(swigCPtr, this, timeout);
  }

  /** Kill this actor and restart it from start. */
  public Actor restart() {
    swigCPtr = simgridJNI.Actor_restart(swigCPtr, this);
    return this;
  }

  /** Kill all actors (but the caller). Being killed is not something that actors can delay or avoid. */
  public static void kill_all() {
    simgridJNI.Actor_kill_all();
  }

  /** Retrieve the property value (or null if not set) */
  public String get_property(String key) {
    return simgridJNI.Actor_get_property(swigCPtr, this, key);
  }

  /** Set a property (old values will be overwritten) */
  public void set_property(String key, String value) {
    simgridJNI.Actor_set_property(swigCPtr, this, key, value);
  }
  /** Block the current actor sleeping for that amount of seconds. Only call this on the current thread (on `this`) */
  protected void sleep_for(double duration) { simgridJNI.Actor_sleep_for(swigCPtr, this, duration); }
  /**
   * Block the current actor sleeping until the specified timestamp. Only call this on the current thread (on `this`)
   */
  public void sleep_until(double wakeup_time) { simgridJNI.Actor_sleep_until(swigCPtr, this, wakeup_time); }
  /** Block the current actor, computing the given amount of flops. Only call this on the current thread (on `this`) */
  public void execute(double flop) { simgridJNI.Actor_execute__SWIG_0(swigCPtr, this, flop); }
  /**
   * Block the current actor, computing the given amount of flops at the given priority. An execution of priority 2
   * computes twice as fast as an execution at priority 1. Only call this on the current thread (on `this`)
   */
  public void execute(double flop, double priority)
  {
    simgridJNI.Actor_execute__SWIG_1(swigCPtr, this, flop, priority);
  }
  /**
   * Block the current actor until the built multi-thread execution completes. Only call this on the current thread
   * (on `this`)
   */
  public void thread_execute(Host host, double flop_amounts, int thread_count)
  {
    simgridJNI.Actor_thread_execute(swigCPtr, this, Host.getCPtr(host), host, flop_amounts, thread_count);
  }
  /** Block the current actor until the built parallel execution completes. */
  public void parallel_execute(Host[] hosts, double[] flops_amounts, double[] bytes_amounts)
  {
    long[] jhosts = new long[hosts.length];
    for (int i = 0; i < hosts.length; i++)
      jhosts[i] = Host.getCPtr(hosts[i]);
    simgridJNI.Actor_parallel_execute(swigCPtr, this, jhosts, flops_amounts, bytes_amounts);
  }
  /**
   * Initialize a sequential execution that must then be started manually. Only call this on the current thread
   * (on `this`)
   */
  public Exec exec_init(double flops_amounts)
  {
    return new Exec(simgridJNI.Actor_exec_seq_init(swigCPtr, this, flops_amounts), true);
  }
  /** Initialize a parallel execution that must then be started manually. */
  public Exec exec_init(Host[] hosts, double[] flops_amounts, double[] bytes_amounts)
  {
    long[] jhosts = new long[hosts.length];
    for (int i = 0; i < hosts.length; i++)
      jhosts[i] = Host.getCPtr(hosts[i]);
    return new Exec(simgridJNI.Actor_exec_par_init(swigCPtr, this, jhosts, flops_amounts, bytes_amounts), true);
  }

  /** Initialize and start a sequential execution. Only call this on the current thread (on `this`) */
  public Exec exec_async(double flops_amounts)
  {
    return new Exec(simgridJNI.Actor_exec_async(swigCPtr, this, flops_amounts), true);
  }

  /**
   * Yield the current actor, give the control to the other actors. It will be executed again in the next scheduling
   * round. Only call this on the current thread (on `this`)
   */
  public void yield() { simgridJNI.Actor_yield(swigCPtr, this); }

  /** Kill the current actor. Only call this on the current thread (on `this`) */
  public void exit() { simgridJNI.Actor_exit(swigCPtr, this); }

  /**
   * Add a function to the list of "on_exit" functions of the current actor.
   *
   * The on_exit functions are the functions executed when your actor is killed. You should use them to free the
   * data used by your actor.
   *
   * The parameter of on_exit's callbacks denotes whether or not the actor's execution failed. It will be set to
   * true if the actor was killed or failed because of an exception or if the simulation deadlocked, while it will
   * remain to false if the actor terminated gracefully.
   */
  public void on_exit(CallbackBoolean code) { simgridJNI.Actor_on_exit(swigCPtr, code); }

  // we need a signal specific to the Java world because the C++ signal is fired before we have any chance to fully
  // initialize the Java side of the actor
  static ArrayList<CallbackActor> on_creation_signal = new ArrayList<>();
  /** Add a callback fired when a new actor has been created */
  public static void on_creation_cb(CallbackActor cb) { on_creation_signal.add(cb); }

  public static void fire_creation_signal(Actor actor, long cPtr)
  {
    // Usually, swigCPtr is set by do_run(), but we need it in advance here. Doing it only here fails for actors added
    // to VMs, because the new actor runs before its creator in this case (maybe an extra simcal). The new actor thus
    // finds its swgCPtr null.
    actor.swigCPtr = cPtr;
    for (var cb : on_creation_signal)
      cb.run(actor);
  }

  // Same here: the C++ signal for destruction is called after Java is cleaned up, so don't use it
  static ArrayList<CallbackActor> on_termination_signal = new ArrayList<>();
  /**
   * Add a callback fired when any actor terminates its code. The actor may continue to exist if it is still
   * referenced in the simulation, but it's not active anymore.
   */
  public static void on_termination_cb(CallbackActor cb) { on_termination_signal.add(cb); }

  public static void fire_termination_signal(Actor actor)
  {
    for (var cb : on_termination_signal)
      cb.run(actor);
  }
}
