/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public class Exec extends Activity {
  protected Exec(long cPtr, boolean cMemoryOwn) { super(cPtr, cMemoryOwn); }

  public static Exec init() {
    long cPtr = simgridJNI.Exec_init();
    return (cPtr == 0) ? null : new Exec(cPtr, true);
  }
  public Exec start()
  {
    super.start();
    return this;
  }

  public double get_remaining() { return simgridJNI.Exec_get_remaining(getCPtr(), this); }

  public double get_remaining_ratio() { return simgridJNI.Exec_get_remaining_ratio(getCPtr(), this); }

  public Exec set_host(Host host) {
    simgridJNI.Exec_set_host(getCPtr(), this, Host.getCPtr(host), host);
    return this;
  }

  public Exec set_hosts(Host[] jhosts)
  {
    long[] chosts = new long[jhosts.length];
    for (int i = 0; i < jhosts.length; i++)
      chosts[i] = Host.getCPtr(jhosts[i]);
    simgridJNI.Exec_set_hosts(getCPtr(), this, chosts);
    return this;
  }

  public Exec unset_host() {
    simgridJNI.Exec_unset_host(getCPtr(), this);
    return this;
  }

  public Exec unset_hosts() {
    simgridJNI.Exec_unset_hosts(getCPtr(), this);
    return this;
  }

  public Exec set_flops_amount(double flops_amount) {
    simgridJNI.Exec_set_flops_amount(getCPtr(), this, flops_amount);
    return this;
  }

  public Exec set_flops_amounts(double[] flops_amounts)
  {
    simgridJNI.Exec_set_flops_amounts(getCPtr(), this, flops_amounts);
    return this;
  }

  public Exec set_bytes_amounts(double[] bytes_amounts)
  {
    simgridJNI.Exec_set_bytes_amounts(getCPtr(), this, bytes_amounts);
    return this;
  }

  public Exec set_thread_count(int thread_count) {
    simgridJNI.Exec_set_thread_count(getCPtr(), this, thread_count);
    return this;
  }

  public Exec set_bound(double bound) {
    simgridJNI.Exec_set_bound(getCPtr(), this, bound);
    return this;
  }

  public Exec set_priority(double priority) {
    simgridJNI.Exec_set_priority(getCPtr(), this, priority);
    return this;
  }

  public Exec update_priority(double priority) {
    simgridJNI.Exec_update_priority(getCPtr(), this, priority);
    return this;
  }

  public Host get_host() {
    long cPtr = simgridJNI.Exec_get_host(getCPtr(), this);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  public long get_host_number() { return simgridJNI.Exec_get_host_number(getCPtr(), this); }

  public int get_thread_count() { return simgridJNI.Exec_get_thread_count(getCPtr(), this); }

  public double get_cost() { return simgridJNI.Exec_get_cost(getCPtr(), this); }

  public boolean is_parallel() { return simgridJNI.Exec_is_parallel(getCPtr(), this); }

  public boolean is_assigned() { return simgridJNI.Exec_is_assigned(getCPtr(), this); }

  public Exec await() throws TimeoutException, HostFailureException
  {
    await_for(-1);
    return this;
  }

  public static void on_start_cb(CallbackExec cb) { simgridJNI.Exec_on_start_cb(cb); }

  public void on_this_start_cb(CallbackExec cb) { simgridJNI.Exec_on_this_start_cb(getCPtr(), this, cb); }

  public static void on_completion_cb(CallbackExec cb) { simgridJNI.Exec_on_completion_cb(cb); }

  public void on_this_completion_cb(CallbackExec cb) { simgridJNI.Exec_on_this_completion_cb(getCPtr(), this, cb); }

  public void on_this_suspend_cb(CallbackExec cb) { simgridJNI.Exec_on_this_suspend_cb(getCPtr(), this, cb); }

  public void on_this_resume_cb(CallbackExec cb) { simgridJNI.Exec_on_this_resume_cb(getCPtr(), this, cb); }

  public static void on_veto_cb(CallbackExec cb) { simgridJNI.Exec_on_veto_cb(cb); }

  public void on_this_veto_cb(CallbackExec cb) { simgridJNI.Exec_on_this_veto_cb(getCPtr(), this, cb); }

  public Exec add_successor(Activity a) {
    simgridJNI.Exec_add_successor(getCPtr(), this, Activity.getCPtr(a), a);
    return this;
  }

  public Exec remove_successor(Activity a) {
    simgridJNI.Exec_remove_successor(getCPtr(), this, Activity.getCPtr(a), a);
    return this;
  }

  public Exec set_name(String name) {
    simgridJNI.Exec_set_name(getCPtr(), this, name);
    return this;
  }

  public String get_name() { return simgridJNI.Exec_get_name(getCPtr(), this); }

  public Exec set_tracing_category(String category) {
    simgridJNI.Exec_set_tracing_category(getCPtr(), this, category);
    return this;
  }

  public String get_tracing_category() { return simgridJNI.Exec_get_tracing_category(getCPtr(), this); }

  public Exec detach() {
    simgridJNI.Exec_detach__SWIG_0(getCPtr(), this);
    return this;
  }

  public Exec detach(CallbackExec clean_function)
  {
    simgridJNI.Exec_detach__SWIG_1(getCPtr(), this, clean_function);
    return this;
  }

  public Exec cancel() {
    simgridJNI.Exec_cancel(getCPtr(), this);
    return this;
  }

  public Exec await_for(double timeout) throws TimeoutException, HostFailureException
  {
    simgridJNI.Exec_await_for(getCPtr(), this, timeout);
    return this;
  }
}
