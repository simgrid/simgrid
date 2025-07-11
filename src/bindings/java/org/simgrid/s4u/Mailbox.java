/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public class Mailbox {
  private transient long swigCPtr;

  protected Mailbox(long cPtr) { swigCPtr = cPtr; }

  protected static long getCPtr(Mailbox obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  public String get_name() {
    return simgridJNI.Mailbox_get_name(swigCPtr, this);
  }

  public boolean empty() {
    return simgridJNI.Mailbox_empty(swigCPtr, this);
  }

  public long size() {
    return simgridJNI.Mailbox_size(swigCPtr, this);
  }

  public boolean listen() {
    return simgridJNI.Mailbox_listen(swigCPtr, this);
  }

  public int listen_from() {
    return simgridJNI.Mailbox_listen_from(swigCPtr, this);
  }

  public boolean ready() {
    return simgridJNI.Mailbox_ready(swigCPtr, this);
  }

  public void set_receiver(Actor actor) {
    simgridJNI.Mailbox_set_receiver(swigCPtr, this, Actor.getCPtr(actor), actor);
  }

  public Actor get_receiver() { return simgridJNI.Mailbox_get_receiver(swigCPtr, this); }

  public Comm put_init() {
    long cPtr = simgridJNI.Mailbox_put_init__SWIG_0(swigCPtr, this);
    return (cPtr == 0) ? null : new Comm(cPtr, true);
  }

  public Comm putInit(Object data, long simulatedSizeInBytes)
  {
    long cPtr = simgridJNI.Mailbox_put_init__SWIG_1(swigCPtr, this, data, simulatedSizeInBytes);
    return (cPtr == 0) ? null : new Comm(cPtr, true);
  }
  public Comm put_init(Object data, double simulatedSizeInBytes) { return putInit(data, (long)simulatedSizeInBytes); }

  public Comm put_async(Object data, long simulatedSizeInBytes)
  {
    long cPtr = simgridJNI.Mailbox_put_async(swigCPtr, this, data, simulatedSizeInBytes);
    return (cPtr == 0) ? null : new Comm(cPtr, true);
  }

  public static String to_c_str(Mailbox.IprobeKind value) {
    return simgridJNI.Mailbox_to_c_str(value.swigValue());
  }

  public Comm get_init() {
    long cPtr = simgridJNI.Mailbox_get_init(swigCPtr, this);
    return (cPtr == 0) ? null : new Comm(cPtr, true);
  }

  public Comm get_async() {
    long cPtr = simgridJNI.Mailbox_get_async(swigCPtr, this);
    return (cPtr == 0) ? null : new Comm(cPtr, true);
  }

  public void clear() {
    simgridJNI.Mailbox_clear(swigCPtr, this);
  }

  public void put(Object payload, double simulatedSizeInBytes)
  {
    simgridJNI.Mailbox_put__SWIG_0(swigCPtr, this, payload, (long)simulatedSizeInBytes);
  }
  public void put(Object payload, long simulatedSizeInBytes)
  {
    simgridJNI.Mailbox_put__SWIG_0(swigCPtr, this, payload, simulatedSizeInBytes);
  }

  public void put(Object payload, long simulatedSizeInBytes, double timeout)
  {
    simgridJNI.Mailbox_put__SWIG_1(swigCPtr, this, payload, simulatedSizeInBytes, timeout);
  }

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
