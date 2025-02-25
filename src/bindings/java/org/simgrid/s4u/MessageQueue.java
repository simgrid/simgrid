/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public class MessageQueue {
  private transient long swigCPtr;
  protected transient boolean swigCMemOwn;

  protected MessageQueue(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(MessageQueue obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected static long swigRelease(MessageQueue obj) {
    long ptr = 0;
    if (obj != null) {
      if (!obj.swigCMemOwn)
        throw new RuntimeException("Cannot release ownership as memory is not owned");
      ptr = obj.swigCPtr;
      obj.swigCMemOwn = false;
      obj.delete();
    }
    return ptr;
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        throw new UnsupportedOperationException("C++ destructor does not have public access");
      }
      swigCPtr = 0;
    }
  }

  public String get_name() {
    return simgridJNI.MessageQueue_get_name(swigCPtr, this);
  }

  public boolean empty() {
    return simgridJNI.MessageQueue_empty(swigCPtr, this);
  }

  public long size() {
    return simgridJNI.MessageQueue_size(swigCPtr, this);
  }

  public Mess put_init() { return new Mess(simgridJNI.MessageQueue_put_init__SWIG_0(swigCPtr, this), true); }

  public Mess put_init(Object payload)
  {
    return new Mess(simgridJNI.MessageQueue_put_init__SWIG_1(swigCPtr, this, payload), true);
  }

  public Mess put_async(Object payload)
  {
    return new Mess(simgridJNI.MessageQueue_put_async(swigCPtr, this, payload), true);
  }

  public void put(Object payload) {
    simgridJNI.MessageQueue_put__SWIG_0(swigCPtr, this, payload);
  }

  public void put(Object payload, double timeout) {
    simgridJNI.MessageQueue_put__SWIG_1(swigCPtr, this, payload, timeout);
  }

  public Mess get_init() { return new Mess(simgridJNI.MessageQueue_get_init(swigCPtr, this), true); }

  public Mess get_async() { return new Mess(simgridJNI.MessageQueue_get_async(swigCPtr, this), true); }
}
