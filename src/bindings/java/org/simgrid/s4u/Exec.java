/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

/**
 * Computation Activity, representing an asynchronous execution.
 *
 * Most of them are created with this_actor.exec_init() or Host.execute(), and represent a classical
 * (sequential) execution. This can be used to simulate some computation occurring in another thread when the
 * calling actor is not blocked during the execution.
 *
 * You can also use this_actor.parallel_execute() to create parallel executions. These objects represent
 * distributed computations involving computations on several hosts and communications between them. Once
 * created, parallel Execs are very similar to the sequential ones, except that they cannot be migrated, and
 * their remaining amount of work can only be defined as a ratio.
 */
public class Exec extends Activity {
  protected Exec(long cPtr, boolean cMemoryOwn) { super(cPtr, cMemoryOwn); }

  /** Initiate the creation of an Exec. Setters have to be called afterwards */
  public static Exec init() {
    long cPtr = simgridJNI.Exec_init();
    return (cPtr == 0) ? null : new Exec(cPtr, true);
  }
  /** Start that execution. */
  public Exec start()
  {
    super.start();
    return this;
  }

  /**
   * On sequential executions, returns the amount of flops that remain to be done; This cannot be used on
   *  parallel executions.
   */
  public double get_remaining() { return simgridJNI.Exec_get_remaining(getCPtr(), this); }

  /** Returns the ratio of elements that are still to do, between 0 (completely done) and 1 (nothing done yet). */
  public double get_remaining_ratio() { return simgridJNI.Exec_get_remaining_ratio(getCPtr(), this); }

  /**
   * Change the host on which this activity takes place. This cannot be done once the activity is terminated,
   *  but it can be done on started executions.
   */
  public Exec set_host(Host host) {
    simgridJNI.Exec_set_host(getCPtr(), this, Host.getCPtr(host), host);
    return this;
  }

  /** Change this sequential execution into a parallel one, spread over the given hosts. */
  public Exec set_hosts(Host[] jhosts)
  {
    long[] chosts = new long[jhosts.length];
    for (int i = 0; i < jhosts.length; i++)
      chosts[i] = Host.getCPtr(jhosts[i]);
    simgridJNI.Exec_set_hosts(getCPtr(), this, chosts);
    return this;
  }

  /** Reset the execution to a state with no host assigned, so that it can be (re)assigned later on. */
  public Exec unset_host() {
    simgridJNI.Exec_unset_host(getCPtr(), this);
    return this;
  }

  /** Reset the execution to a state with no host assigned. Same as unset_host(). */
  public Exec unset_hosts() {
    simgridJNI.Exec_unset_hosts(getCPtr(), this);
    return this;
  }

  /**
   * Set the amount of flops to execute, for sequential executions. Cannot be changed once the exec has
   *  started. Not to be used on parallel executions: use set_flops_amounts() instead.
   */
  public Exec set_flops_amount(double flops_amount) {
    simgridJNI.Exec_set_flops_amount(getCPtr(), this, flops_amount);
    return this;
  }

  /**
   * Set the amount of flops to execute on each host, for parallel executions. This array must have the same
   *  size as the array of hosts given to set_hosts().
   */
  public Exec set_flops_amounts(double[] flops_amounts)
  {
    simgridJNI.Exec_set_flops_amounts(getCPtr(), this, flops_amounts);
    return this;
  }

  /**
   * Set the amount of bytes to exchange between each pair of hosts, for parallel executions. This must be a
   *  host_count-square matrix, given as a flat array.
   */
  public Exec set_bytes_amounts(double[] bytes_amounts)
  {
    simgridJNI.Exec_set_bytes_amounts(getCPtr(), this, bytes_amounts);
    return this;
  }

  /**
   * Change the amount of threads that this execution uses on its host. This models a multi-threaded
   *  execution, where the given amount of flops is spread over several cores of the host, potentially speeding
   *  up the execution when several cores are available. Defaults to 1 (sequential execution). Cannot be
   *  changed once the exec started.
   */
  public Exec set_thread_count(int thread_count) {
    simgridJNI.Exec_set_thread_count(getCPtr(), this, thread_count);
    return this;
  }

  /**
   * Change the execution bound, i.e. the maximal amount of flops per second that it may consume, regardless of
   *  what the host may deliver. Currently, this cannot be changed once the exec started.
   */
  public Exec set_bound(double bound) {
    simgridJNI.Exec_set_bound(getCPtr(), this, bound);
    return this;
  }

  /**
   * Change the execution priority: an execution with twice the priority will get twice the amount of flops
   *  when the resource is shared. The default priority is 1. Currently, this cannot be changed once the exec
   *  started.
   */
  public Exec set_priority(double priority) {
    simgridJNI.Exec_set_priority(getCPtr(), this, priority);
    return this;
  }

  /** Change the execution priority while it is already running. See also set_priority(). */
  public Exec update_priority(double priority) {
    simgridJNI.Exec_update_priority(getCPtr(), this, priority);
    return this;
  }

  /**
   * Retrieve the host on which this activity takes place. If it runs on more than one host, only the first
   *  host is returned.
   */
  public Host get_host() {
    long cPtr = simgridJNI.Exec_get_host(getCPtr(), this);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  /**
   * Retrieve the amount of hosts involved in this execution: 1 for sequential executions, more for parallel
   *  ones.
   */
  public long get_host_number() { return simgridJNI.Exec_get_host_number(getCPtr(), this); }

  /** Retrieve the amount of threads used by this (sequential) execution. See set_thread_count(). */
  public int get_thread_count() { return simgridJNI.Exec_get_thread_count(getCPtr(), this); }

  /** Retrieve the amount of flops that this execution will use, as specified with set_flops_amount(). */
  public double get_cost() { return simgridJNI.Exec_get_cost(getCPtr(), this); }

  /** Returns whether this execution is a parallel one, i.e. whether it was created with several hosts. */
  public boolean is_parallel() { return simgridJNI.Exec_is_parallel(getCPtr(), this); }

  /**
   * Returns whether this execution is assigned to the host(s) that it needs to start. An unassigned execution
   *  cannot start.
   */
  public boolean is_assigned() { return simgridJNI.Exec_is_assigned(getCPtr(), this); }

  /**
   * Blocks the current actor until the execution is terminated. Raises a TimeoutException or
   *  HostFailureException on failure.
   */
  public Exec await() throws TimeoutException, HostFailureException
  {
    await_for(-1);
    return this;
  }

  /** Add a callback fired when any Exec starts (no veto) */
  public static void on_start_cb(CallbackExec cb) { simgridJNI.Exec_on_start_cb(cb); }

  /** Add a callback fired when this specific Exec starts (no veto) */
  public void on_this_start_cb(CallbackExec cb) { simgridJNI.Exec_on_this_start_cb(getCPtr(), this, cb); }

  /** Add a callback fired when any Exec completes (either normally, cancelled or failed) */
  public static void on_completion_cb(CallbackExec cb) { simgridJNI.Exec_on_completion_cb(cb); }

  /** Add a callback fired when this specific Exec completes (either normally, cancelled or failed) */
  public void on_this_completion_cb(CallbackExec cb) { simgridJNI.Exec_on_this_completion_cb(getCPtr(), this, cb); }

  /** Add a callback fired when this specific Exec is suspended */
  public void on_this_suspend_cb(CallbackExec cb) { simgridJNI.Exec_on_this_suspend_cb(getCPtr(), this, cb); }

  /** Add a callback fired when this specific Exec is resumed after being suspended */
  public void on_this_resume_cb(CallbackExec cb) { simgridJNI.Exec_on_this_resume_cb(getCPtr(), this, cb); }

  /**
   * Add a callback fired each time that any Exec fails to start because of a veto (e.g., unsolved dependency
   *  or no host assigned)
   */
  public static void on_veto_cb(CallbackExec cb) { simgridJNI.Exec_on_veto_cb(cb); }

  /** Add a callback fired each time that this specific Exec fails to start because of a veto */
  public void on_this_veto_cb(CallbackExec cb) { simgridJNI.Exec_on_this_veto_cb(getCPtr(), this, cb); }

  /** Add a dependency from this Exec to activity @a a: @a a will not start before this Exec is done. */
  public Exec add_successor(Activity a) {
    simgridJNI.Exec_add_successor(getCPtr(), this, Activity.getCPtr(a), a);
    return this;
  }

  /** Remove a dependency previously declared with add_successor(). */
  public Exec remove_successor(Activity a) {
    simgridJNI.Exec_remove_successor(getCPtr(), this, Activity.getCPtr(a), a);
    return this;
  }

  /** Set the name of this execution, for logging and tracing purposes. */
  public Exec set_name(String name) {
    simgridJNI.Exec_set_name(getCPtr(), this, name);
    return this;
  }

  /** Retrieve the name of this execution. */
  public String get_name() { return simgridJNI.Exec_get_name(getCPtr(), this); }

  /** Set a user-defined tracing category. Must be called before start(). */
  public Exec set_tracing_category(String category) {
    simgridJNI.Exec_set_tracing_category(getCPtr(), this, category);
    return this;
  }

  /**
   * Retrieve the tracing category previously set with set_tracing_category(), or an empty string if none was
   *  set.
   */
  public String get_tracing_category() { return simgridJNI.Exec_get_tracing_category(getCPtr(), this); }

  /** Start the execution, and ignore its result. It can be completely forgotten after that. */
  public Exec detach() {
    simgridJNI.Exec_detach__SWIG_0(getCPtr(), this);
    return this;
  }

  /**
   * Start the execution, and ignore its result, calling @a clean_function on the execution's data once it
   *  completes.
   */
  public Exec detach(CallbackExec clean_function)
  {
    simgridJNI.Exec_detach__SWIG_1(getCPtr(), this, clean_function);
    return this;
  }

  /** Cancel that execution. */
  public Exec cancel() {
    simgridJNI.Exec_cancel(getCPtr(), this);
    return this;
  }

  /**
   * Blocks the current actor until the execution is terminated, or until the timeout is elapsed. Raises a
   *  TimeoutException or HostFailureException on failure.
   */
  public Exec await_for(double timeout) throws TimeoutException, HostFailureException
  {
    simgridJNI.Exec_await_for(getCPtr(), this, timeout);
    return this;
  }
}
