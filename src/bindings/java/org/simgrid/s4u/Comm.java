/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

/**
 * Represents all communications, be they asynchronous or not, that you can start, test and wait for.
 *
 * Most communications are created by exchanging data through a Mailbox. You can also create direct
 * point-to-point communications between two arbitrary hosts, bypassing the mailbox and actor mechanisms, with
 * sendto(), sendto_init() and sendto_async().
 */
public class Comm extends Activity {
  protected Comm(long cPtr, boolean cMemoryOwn) { super(cPtr, cMemoryOwn); }

  /** Add a callback fired when the send of any Comm is posted */
  public static void on_send_cb(CallbackComm cb)
  {
    if (cb != null)
      simgridJNI.Comm_on_send_cb(cb);
  }

  /** Add a callback fired when the send of this specific Comm is posted */
  public void on_this_send_cb(CallbackComm cb) { simgridJNI.Comm_on_this_send_cb(getCPtr(), this, cb); }

  /** Add a callback fired when the recv of any Comm is posted */
  public static void on_recv_cb(CallbackComm cb) { simgridJNI.Comm_on_recv_cb(cb); }

  /** Add a callback fired when the recv of this specific Comm is posted */
  public void on_this_recv_cb(CallbackComm cb) { simgridJNI.Comm_on_this_recv_cb(getCPtr(), this, cb); }

  /**
   * Creates a direct host-to-host communication, bypassing the mailbox mechanism. Source and destination hosts
   *  have to be set with set_source() and set_destination() before the communication can start.
   */
  public static Comm sendto_init() {
    long cPtr = simgridJNI.Comm_sendto_init__SWIG_0();
    return (cPtr == 0) ? null : new Comm(cPtr, true);
  }

  /** Creates a direct communication between the two given hosts, bypassing the mailbox mechanism. */
  public static Comm sendto_init(Host from, Host to) {
    long cPtr = simgridJNI.Comm_sendto_init__SWIG_1(Host.getCPtr(from), from, Host.getCPtr(to), to);
    return (cPtr == 0) ? null : new Comm(cPtr, true);
  }

  public static Comm sendto_async(Host from, Host to, double simulated_size_in_bytes) {
    return sendto_async(from, to, (long)simulated_size_in_bytes);
  }
  /**
   * Creates and starts a direct, asynchronous communication between the two given hosts, bypassing the mailbox
   *  and actor mechanisms. There is no limit on the hosts involved: in particular, the calling actor does not
   *  have to be on one of the involved hosts.
   */
  public static Comm sendto_async(Host from, Host to, long simulated_size_in_bytes) {
    long cPtr = simgridJNI.Comm_sendto_async(Host.getCPtr(from), from, Host.getCPtr(to), to, simulated_size_in_bytes);
    return (cPtr == 0) ? null : new Comm(cPtr, true);
  }

  /** Do a blocking communication between two arbitrary hosts, bypassing the mailbox and actor mechanisms. */
  public static void sendto(Host from, Host to, long simulated_size_in_bytes) {
    simgridJNI.Comm_sendto(Host.getCPtr(from), from, Host.getCPtr(to), to, simulated_size_in_bytes);
  }

  /**
   * Specify the source host of a direct host-to-host communication (see sendto()). Must be set together with
   *  set_destination(), before the communication starts.
   */
  public Comm set_source(Host from) {
    simgridJNI.Comm_set_source(getCPtr(), this, Host.getCPtr(from), from);
    return this;
  }

  /** Retrieve the source host of a direct host-to-host communication, or null for mailbox-based communications. */
  public Host get_source() {
    long cPtr = simgridJNI.Comm_get_source(getCPtr(), this);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  /**
   * Specify the destination host of a direct host-to-host communication (see sendto()). Must be set together
   *  with set_source(), before the communication starts.
   */
  public Comm set_destination(Host to) {
    simgridJNI.Comm_set_destination(getCPtr(), this, Host.getCPtr(to), to);
    return this;
  }

  /**
   * Retrieve the destination host of a direct host-to-host communication, or null for mailbox-based
   *  communications.
   */
  public Host get_destination() {
    long cPtr = simgridJNI.Comm_get_destination(getCPtr(), this);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  /**
   * Specify the mailbox on which this communication should be posted. Must be set before the communication
   *  starts; not to be mixed with direct host-to-host communications.
   */
  public Comm set_mailbox(Mailbox mailbox) {
    simgridJNI.Comm_set_mailbox(getCPtr(), this, Mailbox.getCPtr(mailbox), mailbox);
    return this;
  }

  /** Retrieve the mailbox on which this comm acts. This is null if the comm was created through sendto(). */
  public Mailbox get_mailbox() {
    long cPtr = simgridJNI.Comm_get_mailbox(getCPtr(), this);
    return (cPtr == 0) ? null : new Mailbox(cPtr);
  }

  /**
   * Specify the data to send. This is the data that will actually be copied over to the receiver: it's
   *  unrelated to the simulated size given by set_payload_size().
   */
  public Comm set_src_data(Object payload) {
    simgridJNI.Comm_set_src_data(getCPtr(), this, payload);
    return this;
  }

  /** Retrieve the message's payload. You can only call this once the comm is (gracefully) terminated. */
  public Object get_payload() { return simgridJNI.Comm_get_payload(getCPtr(), this); }

  public Comm set_payload_size(double bytes) {
    return set_payload_size((long)bytes);
  }
  /**
   * Specify the amount of bytes which exchange should be simulated (not to be mixed with the actual size of
   *  the data given to set_src_data()).
   */
  public Comm set_payload_size(long bytes) {
    simgridJNI.Comm_set_payload_size(getCPtr(), this, bytes);
    return this;
  }

  /** Sets the maximal communication rate (in byte/sec). Must be done before start */
  public Comm set_rate(double rate) {
    simgridJNI.Comm_set_rate(getCPtr(), this, rate);
    return this;
  }

  /**
   * Returns whether this communication is assigned to a mailbox, or to a source and destination host (for
   *  direct host-to-host communications). An unassigned communication cannot start.
   */
  public boolean is_assigned() { return simgridJNI.Comm_is_assigned(getCPtr(), this); }

  /**
   * Retrieve the actor which is sending this communication. Returns null if the communication has not started
   *  yet, or if it was created with sendto().
   */
  public Actor get_sender() { return simgridJNI.Comm_get_sender(getCPtr(), this); }

  /**
   * Retrieve the actor which is receiving this communication. Returns null if the communication has not
   *  started yet, or if it was created with sendto().
   */
  public Actor get_receiver() { return simgridJNI.Comm_get_receiver(getCPtr(), this); }

  /** Add a callback fired when any Comm starts (no veto) */
  public static void on_start_cb(CallbackComm cb) { simgridJNI.Comm_on_start_cb(cb); }

  /** Add a callback fired when this specific Comm starts (no veto) */
  public void on_this_start_cb(CallbackComm cb) { simgridJNI.Comm_on_this_start_cb(getCPtr(), this, cb); }

  /** Add a callback fired when any Comm completes (either normally, cancelled or failed) */
  public static void on_completion_cb(CallbackComm cb) { simgridJNI.Comm_on_completion_cb(cb); }

  /** Add a callback fired when this specific Comm completes (either normally, cancelled or failed) */
  public void on_this_completion_cb(CallbackComm cb) { simgridJNI.Comm_on_this_completion_cb(getCPtr(), this, cb); }

  /** Add a callback fired when this specific Comm is suspended */
  public void on_this_suspend_cb(CallbackComm cb) { simgridJNI.Comm_on_this_suspend_cb(getCPtr(), this, cb); }

  /** Add a callback fired when this specific Comm is resumed after being suspended */
  public void on_this_resume_cb(CallbackComm cb) { simgridJNI.Comm_on_this_resume_cb(getCPtr(), this, cb); }

  /**
   * Add a callback fired each time that any Comm fails to start because of a veto (e.g., unsolved dependency or
   *  no mailbox assigned)
   */
  public static void on_veto_cb(CallbackComm cb) { simgridJNI.Comm_on_veto_cb(cb); }

  /** Add a callback fired each time that this specific Comm fails to start because of a veto */
  public void on_this_veto_cb(CallbackComm cb) { simgridJNI.Comm_on_this_veto_cb(getCPtr(), this, cb); }

  /** Add a dependency from this Comm to activity @a a: @a a will not start before this Comm is done. */
  public Comm add_successor(Activity a) {
    simgridJNI.Comm_add_successor(getCPtr(), this, Activity.getCPtr(a), a);
    return this;
  }

  /** Remove a dependency previously declared with add_successor(). */
  public Comm remove_successor(Activity a) {
    simgridJNI.Comm_remove_successor(getCPtr(), this, Activity.getCPtr(a), a);
    return this;
  }

  /** Set the name of this communication, for logging and tracing purposes. */
  public Comm set_name(String name) {
    simgridJNI.Comm_set_name(getCPtr(), this, name);
    return this;
  }

  /** Retrieve the name of this communication. */
  public String get_name() { return simgridJNI.Comm_get_name(getCPtr(), this); }

  /** Set a user-defined tracing category. Must be called before start(). */
  public Comm set_tracing_category(String category) {
    simgridJNI.Comm_set_tracing_category(getCPtr(), this, category);
    return this;
  }

  /**
   * Retrieve the tracing category previously set with set_tracing_category(), or an empty string if none was
   *  set.
   */
  public String get_tracing_category() { return simgridJNI.Comm_get_tracing_category(getCPtr(), this); }

  /** Start the comm, and ignore its result. It can be completely forgotten after that. */
  public Comm detach() {
    simgridJNI.Comm_detach__SWIG_0(getCPtr(), this);
    return this;
  }

  /** Start the comm, and ignore its result, calling @a clean_function on the comm's data once it completes. */
  public Comm detach(CallbackComm clean_function)
  {
    simgridJNI.Comm_detach__SWIG_1(getCPtr(), this, clean_function);
    return this;
  }

  /** Cancel that communication. */
  public Comm cancel() {
    simgridJNI.Comm_cancel(getCPtr(), this);
    return this;
  }

  /**
   * Blocks the current actor until the communication is terminated, or until the timeout is elapsed. Raises a
   *  NetworkFailureException or TimeoutException on failure.
   */
  public Comm await_for(double timeout) throws NetworkFailureException, TimeoutException
  {
    simgridJNI.Comm_await_for(getCPtr(), this, timeout);
    return this;
  }

  /**
   * Blocks the current actor until the communication is terminated. Raises a NetworkFailureException or
   *  TimeoutException on failure.
   */
  public Comm await() throws NetworkFailureException, TimeoutException
  {
    await_for(-1);
    return this;
  }
}
