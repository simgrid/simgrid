/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

/**
 * SimGrid's condition variables are meant to be drop-in replacements of Java's own condition variables, but blocking in
 * the simulation world instead of blocking real threads.
 */
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

  /** Create a new condition variable */
  public static ConditionVariable create() {
    long cPtr = simgridJNI.ConditionVariable_create();
    return (cPtr == 0) ? null : new ConditionVariable(cPtr, true);
  }

  /** Blocks until awaken, no timeout possible */
  public void await(Mutex lock)
  {
    try {
      simgridJNI.ConditionVariable_await_for(swigCPtr, this, Mutex.getCPtr(lock), lock, -1);
    } catch (TimeoutException ex) {
      throw new RuntimeException(
          "ConditionVariable.await() is not supposed to timeout, but it just did. Please report that bug.");
    }
  }

  /** Wait until the given instant. Raises a TimeoutException on timeout. */
  public void await_until(Mutex lock, double timeout_time) throws TimeoutException
  {
    simgridJNI.ConditionVariable_await_until(swigCPtr, this, Mutex.getCPtr(lock), lock, timeout_time);
  }

  /** Wait for the given amount of seconds. Raises a TimeoutException on timeout. */
  public void await_for(Mutex lock, double duration) throws TimeoutException
  {
    simgridJNI.ConditionVariable_await_for(swigCPtr, this, Mutex.getCPtr(lock), lock, duration);
  }

  /** Unblock one actor blocked on that condition variable. If none was blocked, nothing happens. */
  public void notify_one() {
    simgridJNI.ConditionVariable_notify_one(swigCPtr, this);
  }

  /** Unblock all actors blocked on that condition variable. If none was blocked, nothing happens. */
  public void notify_all() {
    simgridJNI.ConditionVariable_notify_all(swigCPtr, this);
  }

}
