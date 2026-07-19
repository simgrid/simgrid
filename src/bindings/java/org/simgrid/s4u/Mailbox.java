/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

/** Mailboxes: Network rendez-vous points. */
public class Mailbox {
  private transient long swigCPtr;

  protected Mailbox(long cPtr) { swigCPtr = cPtr; }

  protected static long getCPtr(Mailbox obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  /** Retrieves the name of this mailbox */
  public String get_name() {
    return simgridJNI.Mailbox_get_name(swigCPtr, this);
  }

  /** Returns whether this mailbox contains queued communications */
  public boolean empty() {
    return simgridJNI.Mailbox_empty(swigCPtr, this);
  }

  /** Returns the number of queued communications */
  public long size() {
    return simgridJNI.Mailbox_size(swigCPtr, this);
  }

  /** Check if there is a communication going on in this mailbox. */
  public boolean listen() {
    return simgridJNI.Mailbox_listen(swigCPtr, this);
  }

  /** Look if there is a communication going on in this mailbox and return the PID of the sender actor */
  public int listen_from() {
    return simgridJNI.Mailbox_listen_from(swigCPtr, this);
  }

  /** Check if there is a communication ready to be consumed from this mailbox. */
  public boolean ready() {
    return simgridJNI.Mailbox_ready(swigCPtr, this);
  }

  /**
   * Declare that the specified actor is a permanent receiver on this mailbox.
   *
   * It means that the communications sent to this mailbox will start flowing to its host even before it does a
   * get(). This models the real behavior of TCP and MPI communications, amongst other. It will improve the
   * accuracy of predictions, in particular if your application exhibits swarms of small messages.
   *
   * SimGrid does not enforce any kind of ownership over the mailbox. Even if a receiver was declared, any other
   * actors can still get() data from the mailbox. The timings will then probably be off tracks, so you should
   * strive on your side to not get data from someone else's mailbox.
   *
   * Note that being permanent receivers of a mailbox prevents actors from being garbage-collected. If your
   * simulation creates many short-lived actors that marked as permanent receiver, you should call
   * set_receiver(null) by the end of the actors so that their memory gets properly reclaimed. This call should be
   * at the end of the actor's function, not in an on_exit callback.
   */
  public void set_receiver(Actor actor) {
    simgridJNI.Mailbox_set_receiver(swigCPtr, this, Actor.getCPtr(actor), actor);
  }

  /** Return the actor declared as permanent receiver, or null if none */
  public Actor get_receiver() { return simgridJNI.Mailbox_get_receiver(swigCPtr, this); }

  /** Creates (but don't start) a data transmission to this mailbox */
  public Comm put_init() {
    long cPtr = simgridJNI.Mailbox_put_init__SWIG_0(swigCPtr, this);
    return (cPtr == 0) ? null : new Comm(cPtr, true);
  }

  /** Creates (but don't start) a data transmission to this mailbox.*/
  public Comm put_init(Object data, long simulatedSizeInBytes)
  {
    long cPtr = simgridJNI.Mailbox_put_init__SWIG_1(swigCPtr, this, data, simulatedSizeInBytes);
    return (cPtr == 0) ? null : new Comm(cPtr, true);
  }
  /** Creates and start a data transmission to this mailbox */
  public Comm put_async(Object data, long simulatedSizeInBytes)
  {
    long cPtr = simgridJNI.Mailbox_put_async(swigCPtr, this, data, simulatedSizeInBytes);
    return (cPtr == 0) ? null : new Comm(cPtr, true);
  }

  public static String to_c_str(Mailbox.IprobeKind value) {
    return simgridJNI.Mailbox_to_c_str(value.swigValue());
  }

  /** Creates (but don't start) a data reception onto this mailbox. */
  public Comm get_init() {
    long cPtr = simgridJNI.Mailbox_get_init(swigCPtr, this);
    return (cPtr == 0) ? null : new Comm(cPtr, true);
  }

  /**
   * Creates and start an async data reception to this mailbox. Use Comm.get_payload() once the comm terminates
   * to retrieve the received data.
   */
  public Comm get_async() {
    long cPtr = simgridJNI.Mailbox_get_async(swigCPtr, this);
    return (cPtr == 0) ? null : new Comm(cPtr, true);
  }

  /** Removes all the queued communications of this mailbox, cancelling them as needed. */
  public void clear() {
    simgridJNI.Mailbox_clear(swigCPtr, this);
  }

  /** Blocking data transmission. */
  public void put(Object payload, double simulatedSizeInBytes)
  {
    simgridJNI.Mailbox_put__SWIG_0(swigCPtr, this, payload, (long)simulatedSizeInBytes);
  }
  /** Blocking data transmission. */
  public void put(Object payload, long simulatedSizeInBytes)
  {
    simgridJNI.Mailbox_put__SWIG_0(swigCPtr, this, payload, simulatedSizeInBytes);
  }

  /** Blocking data transmission with timeout */
  public void put(Object payload, long simulatedSizeInBytes, double timeout)
  {
    simgridJNI.Mailbox_put__SWIG_1(swigCPtr, this, payload, simulatedSizeInBytes, timeout);
  }

  /** Blocking data reception */
  public java.lang.Object get() throws NetworkFailureException { return simgridJNI.Mailbox_get(swigCPtr, this); }

  public final static class IprobeKind {
    public final static Mailbox.IprobeKind SEND = new Mailbox.IprobeKind("SEND");
    public final static Mailbox.IprobeKind RECV = new Mailbox.IprobeKind("RECV");

    public final int swigValue() {
      return swigValue;
    }

    public String toString() {
      return swigName;
    }

    public static IprobeKind swigToEnum(int swigValue) {
      if (swigValue < swigValues.length && swigValue >= 0 && swigValues[swigValue].swigValue == swigValue)
        return swigValues[swigValue];
      for (int i = 0; i < swigValues.length; i++)
        if (swigValues[i].swigValue == swigValue)
          return swigValues[i];
      throw new IllegalArgumentException("No enum " + IprobeKind.class + " with value " + swigValue);
    }

    private IprobeKind(String swigName) {
      this.swigName = swigName;
      this.swigValue = swigNext++;
    }

    private IprobeKind(String swigName, int swigValue) {
      this.swigName = swigName;
      this.swigValue = swigValue;
      swigNext = swigValue+1;
    }

    private IprobeKind(String swigName, IprobeKind swigEnum) {
      this.swigName = swigName;
      this.swigValue = swigEnum.swigValue;
      swigNext = this.swigValue+1;
    }

    private static IprobeKind[] swigValues = { SEND, RECV };
    private static int swigNext = 0;
    private final int swigValue;
    private final String swigName;
  }

}
