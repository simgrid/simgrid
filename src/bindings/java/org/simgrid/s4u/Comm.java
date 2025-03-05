/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public class Comm extends Activity {
  protected Comm(long cPtr, boolean cMemoryOwn) { super(cPtr, cMemoryOwn); }

  public static void on_send_cb(CallbackComm cb)
  {
    if (cb != null)
      simgridJNI.Comm_on_send_cb(cb);
  }

  public void on_this_send_cb(CallbackComm cb) { simgridJNI.Comm_on_this_send_cb(getCPtr(), this, cb); }

  public static void on_recv_cb(CallbackComm cb) { simgridJNI.Comm_on_recv_cb(cb); }

  public void on_this_recv_cb(CallbackComm cb) { simgridJNI.Comm_on_this_recv_cb(getCPtr(), this, cb); }

  public static Comm sendto_init() {
    long cPtr = simgridJNI.Comm_sendto_init__SWIG_0();
    return (cPtr == 0) ? null : new Comm(cPtr, true);
  }

  public static Comm sendto_init(Host from, Host to) {
    long cPtr = simgridJNI.Comm_sendto_init__SWIG_1(Host.getCPtr(from), from, Host.getCPtr(to), to);
    return (cPtr == 0) ? null : new Comm(cPtr, true);
  }

  public static Comm sendto_async(Host from, Host to, double simulated_size_in_bytes) {
    return sendto_async(from, to, (long)simulated_size_in_bytes);
  }
  public static Comm sendto_async(Host from, Host to, long simulated_size_in_bytes) {
    long cPtr = simgridJNI.Comm_sendto_async(Host.getCPtr(from), from, Host.getCPtr(to), to, simulated_size_in_bytes);
    return (cPtr == 0) ? null : new Comm(cPtr, true);
  }

  public static void sendto(Host from, Host to, long simulated_size_in_bytes) {
    simgridJNI.Comm_sendto(Host.getCPtr(from), from, Host.getCPtr(to), to, simulated_size_in_bytes);
  }

  public Comm set_source(Host from) {
    simgridJNI.Comm_set_source(getCPtr(), this, Host.getCPtr(from), from);
    return this;
  }

  public Host get_source() {
    long cPtr = simgridJNI.Comm_get_source(getCPtr(), this);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  public Comm set_destination(Host to) {
    simgridJNI.Comm_set_destination(getCPtr(), this, Host.getCPtr(to), to);
    return this;
  }

  public Host get_destination() {
    long cPtr = simgridJNI.Comm_get_destination(getCPtr(), this);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  public Comm set_mailbox(Mailbox mailbox) {
    simgridJNI.Comm_set_mailbox(getCPtr(), this, Mailbox.getCPtr(mailbox), mailbox);
    return this;
  }

  public Mailbox get_mailbox() {
    long cPtr = simgridJNI.Comm_get_mailbox(getCPtr(), this);
    return (cPtr == 0) ? null : new Mailbox(cPtr);
  }

  public Comm set_src_data(Object payload) {
    simgridJNI.Comm_set_src_data(getCPtr(), this, payload);
    return this;
  }

  public Object get_payload() { return simgridJNI.Comm_get_payload(getCPtr(), this); }

  public Comm set_payload_size(double bytes) {
    return set_payload_size((long)bytes);
  }
  public Comm set_payload_size(long bytes) {
    simgridJNI.Comm_set_payload_size(getCPtr(), this, bytes);
    return this;
  }

  public Comm set_rate(double rate) {
    simgridJNI.Comm_set_rate(getCPtr(), this, rate);
    return this;
  }

  public boolean is_assigned() { return simgridJNI.Comm_is_assigned(getCPtr(), this); }

  public Actor get_sender() { return simgridJNI.Comm_get_sender(getCPtr(), this); }

  public Actor get_receiver() { return simgridJNI.Comm_get_receiver(getCPtr(), this); }

  public static void on_start_cb(CallbackComm cb) { simgridJNI.Comm_on_start_cb(cb); }

  public void on_this_start_cb(CallbackComm cb) { simgridJNI.Comm_on_this_start_cb(getCPtr(), this, cb); }

  public static void on_completion_cb(CallbackComm cb) { simgridJNI.Comm_on_completion_cb(cb); }

  public void on_this_completion_cb(CallbackComm cb) { simgridJNI.Comm_on_this_completion_cb(getCPtr(), this, cb); }

  public void on_this_suspend_cb(CallbackComm cb) { simgridJNI.Comm_on_this_suspend_cb(getCPtr(), this, cb); }

  public void on_this_resume_cb(CallbackComm cb) { simgridJNI.Comm_on_this_resume_cb(getCPtr(), this, cb); }

  public static void on_veto_cb(CallbackComm cb) { simgridJNI.Comm_on_veto_cb(cb); }

  public void on_this_veto_cb(CallbackComm cb) { simgridJNI.Comm_on_this_veto_cb(getCPtr(), this, cb); }

  public Comm add_successor(Activity a) {
    simgridJNI.Comm_add_successor(getCPtr(), this, Activity.getCPtr(a), a);
    return this;
  }

  public Comm remove_successor(Activity a) {
    simgridJNI.Comm_remove_successor(getCPtr(), this, Activity.getCPtr(a), a);
    return this;
  }

  public Comm set_name(String name) {
    simgridJNI.Comm_set_name(getCPtr(), this, name);
    return this;
  }

  public String get_name() { return simgridJNI.Comm_get_name(getCPtr(), this); }

  public Comm set_tracing_category(String category) {
    simgridJNI.Comm_set_tracing_category(getCPtr(), this, category);
    return this;
  }

  public String get_tracing_category() { return simgridJNI.Comm_get_tracing_category(getCPtr(), this); }

  public Comm detach() {
    simgridJNI.Comm_detach__SWIG_0(getCPtr(), this);
    return this;
  }

  public Comm detach(CallbackComm clean_function)
  {
    simgridJNI.Comm_detach__SWIG_1(getCPtr(), this, clean_function);
    return this;
  }

  public Comm cancel() {
    simgridJNI.Comm_cancel(getCPtr(), this);
    return this;
  }

  public Comm await_for(double timeout) throws NetworkFailureException, TimeoutException
  {
    simgridJNI.Comm_await_for(getCPtr(), this, timeout);
    return this;
  }

  public Comm await() throws NetworkFailureException, TimeoutException
  {
    await_for(-1);
    return this;
  }
}
