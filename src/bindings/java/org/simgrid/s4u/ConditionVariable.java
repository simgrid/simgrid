/* Copyright (c) 2024-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public class ConditionVariable {
  private transient long swigCPtr;
  private transient boolean swigCMemOwnBase;

  protected ConditionVariable(long cPtr, boolean cMemoryOwn) {
    swigCMemOwnBase = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(ConditionVariable obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  @SuppressWarnings({"deprecation", "removal"})
  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if(swigCPtr != 0 && swigCMemOwnBase) {
      swigCMemOwnBase = false;
      simgridJNI.delete_ConditionVariable(swigCPtr);
    }
    swigCPtr = 0;
  }

  public static ConditionVariable create() {
    long cPtr = simgridJNI.ConditionVariable_create();
    return (cPtr == 0) ? null : new ConditionVariable(cPtr, true);
  }

  /** Blocks until awaken, no timeout possible */
  public void await(Mutex lock)
  {
    simgridJNI.ConditionVariable_await_for(swigCPtr, this, Mutex.getCPtr(lock), lock, -1);
  }

  /** Returns true if the wait timeouted */
  public boolean await_until(Mutex lock, double timeout_time)
  {
    return simgridJNI.ConditionVariable_await_until(swigCPtr, this, Mutex.getCPtr(lock), lock, timeout_time);
  }

  /** Returns true if the wait timeouted */
  public boolean wait_for(Mutex lock, double duration)
  {
    return simgridJNI.ConditionVariable_await_for(swigCPtr, this, Mutex.getCPtr(lock), lock, duration);
  }

  public void notify_one() {
    simgridJNI.ConditionVariable_notify_one(swigCPtr, this);
  }

  public void notify_all() {
    simgridJNI.ConditionVariable_notify_all(swigCPtr, this);
  }

}
