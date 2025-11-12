/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public class Barrier {
  private transient long swigCPtr;
  private transient boolean swigCMemOwnBase;

  protected Barrier(long cPtr, boolean cMemoryOwn) {
    swigCMemOwnBase = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(Barrier obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  @SuppressWarnings({"deprecation", "removal"})
  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if(swigCPtr != 0 && swigCMemOwnBase) {
      swigCMemOwnBase = false;
      simgridJNI.delete_Barrier(swigCPtr);
    }
    swigCPtr = 0;
  }

  public static Barrier create(long expected_actors) {
    long cPtr = simgridJNI.Barrier_create(expected_actors);
    return (cPtr == 0) ? null : new Barrier(cPtr, true);
  }

  public String to_string() {
    return simgridJNI.Barrier_to_string(swigCPtr, this);
  }

  public boolean await() { return simgridJNI.Barrier_await(swigCPtr, this); }
}
