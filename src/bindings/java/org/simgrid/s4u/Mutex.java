/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public class Mutex {
  private transient long swigCPtr;
  private transient boolean swigCMemOwnBase;

  protected Mutex(long cPtr, boolean cMemoryOwn) {
    swigCMemOwnBase = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(Mutex obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  @SuppressWarnings({"deprecation", "removal"})
  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if(swigCPtr != 0 && swigCMemOwnBase) {
      swigCMemOwnBase = false;
      simgridJNI.delete_Mutex(swigCPtr);
    }
    swigCPtr = 0;
  }

  public static Mutex create(boolean recursive) {
    long cPtr = simgridJNI.Mutex_create(recursive);
    return (cPtr == 0) ? null : new Mutex(cPtr, true);
  }

  public static Mutex create() { return create(false); }

  public void lock() {
    simgridJNI.Mutex_lock(swigCPtr, this);
  }

  public void unlock() {
    simgridJNI.Mutex_unlock(swigCPtr, this);
  }

  public boolean try_lock() {
    return simgridJNI.Mutex_try_lock(swigCPtr, this);
  }

  public Actor get_owner() { return simgridJNI.Mutex_get_owner(swigCPtr, this); }
}
