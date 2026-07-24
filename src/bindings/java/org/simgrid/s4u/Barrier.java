/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

/**
 * A classical barrier, but blocking in the simulation world.
 *
 * A fixed number of actors block in await() until all of them reached that point, after which they are all unblocked
 * together.
 */
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

  /** Creates a barrier for the given amount of actors */
  public static Barrier create(long expected_actors) {
    long cPtr = simgridJNI.Barrier_create(expected_actors);
    return (cPtr == 0) ? null : new Barrier(cPtr, true);
  }

  /** Returns some debug information about the barrier */
  public String to_string() {
    return simgridJNI.Barrier_to_string(swigCPtr, this);
  }

  /**
   * Blocks into the barrier. Every waiting actor is unblocked once the expected amount of actors reaches the barrier.
   *
   *  @return false for all actors but one: exactly one actor (picked arbitrarily) gets true, so that it can be elected
   * to do some serial work before the others resume, if needed.
   */
  public boolean await() { return simgridJNI.Barrier_await(swigCPtr, this); }
}
