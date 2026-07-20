/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

/**
 * A VM represents a virtual machine (or a container) that hosts actors. The total computing power that the
 * contained actors can get is constrained to the virtual machine size.
 */
public class VirtualMachine extends Host {
  private transient long swigCPtr;

  protected VirtualMachine(long cPtr)
  {
    super(cPtr);
    swigCPtr = cPtr;
  }

  protected static long getCPtr(VirtualMachine obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  public static String to_c_str(VirtualMachine.State value) {
    return simgridJNI.VirtualMachine_to_c_str(value.swigValue());
  }

  /** Immediately boots this virtual machine, which must be in the CREATED state */
  public void start() {
    simgridJNI.VirtualMachine_start(swigCPtr, this);
  }

  /** Suspends this virtual machine. Actors running on it are not scheduled anymore until resume() is called. */
  public void suspend() {
    simgridJNI.VirtualMachine_suspend(swigCPtr, this);
  }

  /** Resumes this virtual machine, previously suspended with suspend() */
  public void resume() {
    simgridJNI.VirtualMachine_resume(swigCPtr, this);
  }

  /** Immediately shuts down this virtual machine. Actors running on it are forcefully stopped. */
  public void shutdown() {
    simgridJNI.VirtualMachine_shutdown(swigCPtr, this);
  }

  /** Immediately destroys this virtual machine, freeing its resources */
  public void destroy() {
    simgridJNI.VirtualMachine_destroy(swigCPtr, this);
  }

  /** Retrieve the physical machine (the host) on which this virtual machine currently runs */
  public Host get_pm() {
    long cPtr = simgridJNI.VirtualMachine_get_pm(swigCPtr, this);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  /** Sets the physical machine (the host) on which this virtual machine runs */
  public VirtualMachine set_pm(Host pm) {
    simgridJNI.VirtualMachine_set_pm(swigCPtr, this, Host.getCPtr(pm), pm);
    return this;
  }

  /** Retrieve the amount of RAM dedicated to this virtual machine */
  public long get_ramsize() {
    return simgridJNI.VirtualMachine_get_ramsize(swigCPtr, this);
  }

  /** Sets the amount of RAM dedicated to this virtual machine */
  public VirtualMachine set_ramsize(double ramsize) { return set_ramsize((long)ramsize); }
  /** Sets the amount of RAM dedicated to this virtual machine */
  public VirtualMachine set_ramsize(long ramsize) {
    simgridJNI.VirtualMachine_set_ramsize(swigCPtr, this, ramsize);
    return this;
  }

  /**
   * Sets the CPU utilization bound for this virtual machine, that is the max fraction of the underlying physical
   * host's CPU that this VM (and the actors it hosts) can use
   */
  public VirtualMachine set_bound(double bound) {
    simgridJNI.VirtualMachine_set_bound(swigCPtr, this, bound);
    return this;
  }

  /**
   * Migrates this virtual machine to the given host: a shortcut combining set_pm() with start_migration() and
   * end_migration().
   */
  public VirtualMachine migrate(Host newHost)
  {
    simgridJNI.VirtualMachine_migrate(swigCPtr, this, Host.getCPtr(newHost), newHost);
    return this;
  }
  /**
   * Starts the migration of this virtual machine. Call set_pm() to change its physical host, then
   * end_migration() once you are done.
   */
  public void start_migration() {
    simgridJNI.VirtualMachine_start_migration(swigCPtr, this);
  }

  /** Ends the migration of this virtual machine, previously started with start_migration() */
  public void end_migration() {
    simgridJNI.VirtualMachine_end_migration(swigCPtr, this);
  }

  /** Retrieve the current state of this virtual machine */
  public VirtualMachine.State get_state() {
    return VirtualMachine.State.swigToEnum(simgridJNI.VirtualMachine_get_state(swigCPtr, this));
  }

  /** Add a callback fired when any VM starts */
  public static void on_start_cb(CallbackVirtualMachine cb) { simgridJNI.VirtualMachine_on_start_cb(cb); }

  /** Add a callback fired when this specific VM starts */
  public void on_this_start_cb(CallbackVirtualMachine cb)
  {
    simgridJNI.VirtualMachine_on_this_start_cb(swigCPtr, this, cb);
  }

  /** Add a callback fired when any VM is actually started */
  public static void on_started_cb(CallbackVirtualMachine cb) { simgridJNI.VirtualMachine_on_started_cb(cb); }

  /** Add a callback fired when this specific VM is actually started */
  public void on_this_started_cb(CallbackVirtualMachine cb)
  {
    simgridJNI.VirtualMachine_on_this_started_cb(swigCPtr, this, cb);
  }

  /** Add a callback fired when any VM is shut down */
  public static void on_shutdown_cb(CallbackVirtualMachine cb) { simgridJNI.VirtualMachine_on_shutdown_cb(cb); }

  /** Add a callback fired when this specific VM is shut down */
  public void on_this_shutdown_cb(CallbackVirtualMachine cb)
  {
    simgridJNI.VirtualMachine_on_this_shutdown_cb(swigCPtr, this, cb);
  }

  /** Add a callback fired when this specific VM is suspended */
  public void on_this_suspend_cb(CallbackVirtualMachine cb)
  {
    simgridJNI.VirtualMachine_on_this_suspend_cb(swigCPtr, this, cb);
  }

  /** Add a callback fired when this specific VM is resumed */
  public void on_this_resume_cb(CallbackVirtualMachine cb)
  {
    simgridJNI.VirtualMachine_on_this_resume_cb(swigCPtr, this, cb);
  }

  /** Add a callback fired when any VM is destroyed */
  public static void on_destruction_cb(CallbackVirtualMachine cb) { simgridJNI.VirtualMachine_on_destruction_cb(cb); }

  /** Add a callback fired when this specific VM is destroyed */
  public void on_this_destruction_cb(CallbackVirtualMachine cb)
  {
    simgridJNI.VirtualMachine_on_this_destruction_cb(swigCPtr, this, cb);
  }

  /** Add a callback fired when any VM starts a migration */
  public static void on_migration_start_cb(CallbackVirtualMachine cb)
  {
    simgridJNI.VirtualMachine_on_migration_start_cb(cb);
  }

  /** Add a callback fired when this specific VM starts a migration */
  public void on_this_migration_start_cb(CallbackVirtualMachine cb)
  {
    simgridJNI.VirtualMachine_on_this_migration_start_cb(swigCPtr, this, cb);
  }

  /** Add a callback fired when any VM ends a migration */
  public static void on_migration_end_cb(CallbackVirtualMachine cb)
  {
    simgridJNI.VirtualMachine_on_migration_end_cb(cb);
  }

  /** Add a callback fired when this specific VM ends a migration */
  public void on_this_migration_end_cb(CallbackVirtualMachine cb)
  {
    simgridJNI.VirtualMachine_on_this_migration_end_cb(swigCPtr, this, cb);
  }

  public final static class State {
    public final static VirtualMachine.State CREATED = new VirtualMachine.State("CREATED");
    public final static VirtualMachine.State RUNNING = new VirtualMachine.State("RUNNING");
    public final static VirtualMachine.State SUSPENDED = new VirtualMachine.State("SUSPENDED");
    public final static VirtualMachine.State DESTROYED = new VirtualMachine.State("DESTROYED");

    public final int swigValue() {
      return swigValue;
    }

    public String toString() {
      return swigName;
    }

    public static State swigToEnum(int swigValue) {
      if (swigValue < swigValues.length && swigValue >= 0 && swigValues[swigValue].swigValue == swigValue)
        return swigValues[swigValue];
      for (int i = 0; i < swigValues.length; i++)
        if (swigValues[i].swigValue == swigValue)
          return swigValues[i];
      throw new IllegalArgumentException("No enum " + State.class + " with value " + swigValue);
    }

    private State(String swigName) {
      this.swigName = swigName;
      this.swigValue = swigNext++;
    }

    private State(String swigName, int swigValue) {
      this.swigName = swigName;
      this.swigValue = swigValue;
      swigNext = swigValue+1;
    }

    private State(String swigName, State swigEnum) {
      this.swigName = swigName;
      this.swigValue = swigEnum.swigValue;
      swigNext = this.swigValue+1;
    }

    private static State[] swigValues = { CREATED, RUNNING, SUSPENDED, DESTROYED };
    private static int swigNext = 0;
    private final int swigValue;
    private final String swigName;
  }
}
