/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

/**
 * A classical semaphore, but blocking in the simulation world. It is strictly impossible to use a real semaphore here,
 * because it would block the whole simulation. Instead, you should use the present class, that offers a very similar
 * interface.
 */
public class Semaphore {
  private transient long swigCPtr;
  private transient boolean swigCMemOwnBase;

  protected Semaphore(long cPtr, boolean cMemoryOwn) {
    swigCMemOwnBase = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(Semaphore obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  @SuppressWarnings({"deprecation", "removal"})
  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if(swigCPtr != 0 && swigCMemOwnBase) {
      swigCMemOwnBase = false;
      simgridJNI.delete_Semaphore(swigCPtr);
    }
    swigCPtr = 0;
  }

  /** Constructs a new semaphore, with the given initial capacity */
  public static Semaphore create(long initial_capacity) {
    long cPtr = simgridJNI.Semaphore_create(initial_capacity);
    return (cPtr == 0) ? null : new Semaphore(cPtr, true);
  }

  /**
   * Acquire the semaphore, blocking the current actor until it is available if needed (i.e. until its capacity is
   * positive again).
   */
  public void acquire() {
    simgridJNI.Semaphore_acquire(swigCPtr, this);
  }

  /**
   * Just like acquire(), but with a timeout. Returns true if there was a timeout, false if the semaphore was acquired
   *  normally.
   */
  public boolean acquire_timeout(double timeout) {
    return simgridJNI.Semaphore_acquire_timeout(swigCPtr, this, timeout);
  }

  /** Release the semaphore, increasing its capacity by one, and waking up a blocked actor if any. */
  public void release() {
    simgridJNI.Semaphore_release(swigCPtr, this);
  }

  /**
   * Retrieve the current capacity of the semaphore, i.e. the amount of times it could still be acquired before
   * blocking.
   */
  public int get_capacity() {
    return simgridJNI.Semaphore_get_capacity(swigCPtr, this);
  }

  /** Returns whether calling acquire() would block the current actor, i.e. whether the capacity is not positive. */
  public boolean would_block() { return simgridJNI.Semaphore_would_block(swigCPtr, this); }
}
