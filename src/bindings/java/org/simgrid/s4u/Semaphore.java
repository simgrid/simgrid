/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

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

  public static Semaphore create(long initial_capacity) {
    long cPtr = simgridJNI.Semaphore_create(initial_capacity);
    return (cPtr == 0) ? null : new Semaphore(cPtr, true);
  }

  public void acquire() {
    simgridJNI.Semaphore_acquire(swigCPtr, this);
  }

  public boolean acquire_timeout(double timeout) {
    return simgridJNI.Semaphore_acquire_timeout(swigCPtr, this, timeout);
  }

  public void release() {
    simgridJNI.Semaphore_release(swigCPtr, this);
  }

  public int get_capacity() {
    return simgridJNI.Semaphore_get_capacity(swigCPtr, this);
  }

  public boolean would_block() {
    return simgridJNI.Semaphore_would_block(swigCPtr, this);
  }

}
