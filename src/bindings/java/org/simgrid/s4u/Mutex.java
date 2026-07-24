/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

/**
 * A classical mutex, but blocking in the simulation world.
 *
 * By default, mutexes are not recursive: if an actor tries to lock the same object twice, it deadlocks with itself.
 * Pass true to create() to get a recursive mutex instead.
 */
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

  /**
   * Constructs a new mutex. Pass recursive=true to get a mutex that the same actor can lock several times in a row (it
   * then has to be unlocked as many times before another actor can lock it).
   */
  public static Mutex create(boolean recursive) {
    long cPtr = simgridJNI.Mutex_create(recursive);
    return (cPtr == 0) ? null : new Mutex(cPtr, true);
  }

  /** Constructs a new, non-recursive mutex. */
  public static Mutex create() { return create(false); }

  /** Locks the mutex, blocking the current actor until it becomes available if needed. */
  public void lock() {
    simgridJNI.Mutex_lock(swigCPtr, this);
  }

  /** Unlocks the mutex. This must be called by the actor that currently owns the lock, or the behavior is undefined. */
  public void unlock() {
    simgridJNI.Mutex_unlock(swigCPtr, this);
  }

  /** Tries to lock the mutex and returns whether it succeeded, without blocking the current actor. */
  public boolean try_lock() {
    return simgridJNI.Mutex_try_lock(swigCPtr, this);
  }

  /** Retrieve the actor that currently owns this mutex, or null if it is not locked. */
  public Actor get_owner() { return simgridJNI.Mutex_get_owner(swigCPtr, this); }
}
