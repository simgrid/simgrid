/* Copyright (c) 2024-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public class Comm extends Activity {
  private transient long swigCPtr;
  private transient boolean swigCMemOwnDerived;

  protected Comm(long cPtr, boolean cMemoryOwn) {
    super(cPtr, true);
    swigCMemOwnDerived = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(Comm obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  @SuppressWarnings({"deprecation", "removal"})
  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if(swigCPtr != 0 && swigCMemOwnDerived) {
      swigCMemOwnDerived = false;
      simgridJNI.delete_Comm(swigCPtr);
    }
    swigCPtr = 0;
    super.delete();
  }

  public static void on_send_cb(CallbackComm cb) { simgridJNI.Comm_on_send_cb(cb); }

  public void on_this_send_cb(CallbackComm cb) { simgridJNI.Comm_on_this_send_cb(swigCPtr, this, cb); }

  public static void on_recv_cb(CallbackComm cb) { simgridJNI.Comm_on_recv_cb(cb); }

  public void on_this_recv_cb(CallbackComm cb) { simgridJNI.Comm_on_this_recv_cb(swigCPtr, this, cb); }

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
    simgridJNI.Comm_set_source(swigCPtr, this, Host.getCPtr(from), from);
    return this;
  }

  public Host get_source() {
    long cPtr = simgridJNI.Comm_get_source(swigCPtr, this);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  public Comm set_destination(Host to) {
    long cPtr = simgridJNI.Comm_set_destination(swigCPtr, this, Host.getCPtr(to), to);
    return (cPtr == 0) ? null : new Comm(cPtr, true);
  }

  public Host get_destination() {
    long cPtr = simgridJNI.Comm_get_destination(swigCPtr, this);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  public Comm set_mailbox(Mailbox mailbox) {
    long cPtr = simgridJNI.Comm_set_mailbox(swigCPtr, this, Mailbox.getCPtr(mailbox), mailbox);
    return (cPtr == 0) ? null : new Comm(cPtr, true);
  }

  public Mailbox get_mailbox() {
    long cPtr = simgridJNI.Comm_get_mailbox(swigCPtr, this);
    return (cPtr == 0) ? null : new Mailbox(cPtr);
  }

  public Comm set_src_data(Object payload) {
    simgridJNI.Comm_set_src_data(swigCPtr, this, payload);
    return this;
  }

  public Object get_payload() {
    return simgridJNI.Comm_get_payload(swigCPtr, this);
  }

  public Comm set_payload_size(double bytes) {
    return set_payload_size((long)bytes);
  }
  public Comm set_payload_size(long bytes) {
      simgridJNI.Comm_set_payload_size(swigCPtr, this, bytes);
    return this;
  }

  public Comm set_rate(double rate) {
    simgridJNI.Comm_set_rate(swigCPtr, this, rate);
    return this;
  }

  public boolean is_assigned() {
    return simgridJNI.Comm_is_assigned(swigCPtr, this);
  }

  public Actor get_sender() {
    long cPtr = simgridJNI.Comm_get_sender(swigCPtr, this);
    return (cPtr == 0) ? null : new Actor(cPtr, true);
  }

  public Actor get_receiver() {
    long cPtr = simgridJNI.Comm_get_receiver(swigCPtr, this);
    return (cPtr == 0) ? null : new Actor(cPtr, true);
  }

  public static void on_start_cb(CallbackComm cb) { simgridJNI.Comm_on_start_cb(cb); }

  public void on_this_start_cb(CallbackComm cb) { simgridJNI.Comm_on_this_start_cb(swigCPtr, this, cb); }

  public static void on_completion_cb(CallbackComm cb) { simgridJNI.Comm_on_completion_cb(cb); }

  public void on_this_completion_cb(CallbackComm cb) { simgridJNI.Comm_on_this_completion_cb(swigCPtr, this, cb); }

  public void on_this_suspend_cb(CallbackComm cb) { simgridJNI.Comm_on_this_suspend_cb(swigCPtr, this, cb); }

  public void on_this_resume_cb(CallbackComm cb) { simgridJNI.Comm_on_this_resume_cb(swigCPtr, this, cb); }

  public static void on_veto_cb(CallbackComm cb) { simgridJNI.Comm_on_veto_cb(cb); }

  public void on_this_veto_cb(CallbackComm cb) { simgridJNI.Comm_on_this_veto_cb(swigCPtr, this, cb); }

  public Comm add_successor(Activity a) {
    simgridJNI.Comm_add_successor(swigCPtr, this, Activity.getCPtr(a), a);
    return this;
  }

  public Comm remove_successor(Activity a) {
    simgridJNI.Comm_remove_successor(swigCPtr, this, Activity.getCPtr(a), a);
    return this;
  }

  public Comm set_name(String name) {
    simgridJNI.Comm_set_name(swigCPtr, this, name);
    return this;
  }

  public String get_name() {
    return simgridJNI.Comm_get_name(swigCPtr, this);
  }

  public Comm set_tracing_category(String category) {
    simgridJNI.Comm_set_tracing_category(swigCPtr, this, category);
    return this;
  }

  public String get_tracing_category() {
    return simgridJNI.Comm_get_tracing_category(swigCPtr, this);
  }

  public Comm detach() {
    simgridJNI.Comm_detach__SWIG_0(swigCPtr, this);
    return this;
  }

  public Comm detach(CallbackComm clean_function)
  {
    simgridJNI.Comm_detach__SWIG_1(swigCPtr, this, clean_function);
    return this;
  }

  public Comm cancel() {
    simgridJNI.Comm_cancel(swigCPtr, this);
    return this;
  }

  public Comm await_for(double timeout)
  {
    simgridJNI.Comm_await_for(swigCPtr, this, timeout);
    return this;
  }

  public Comm await() {
    await_for(-1);
    return this;
  }
}
