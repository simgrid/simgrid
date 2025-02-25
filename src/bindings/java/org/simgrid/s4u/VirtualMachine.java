/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

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

  public void start() {
    simgridJNI.VirtualMachine_start(swigCPtr, this);
  }

  public void suspend() {
    simgridJNI.VirtualMachine_suspend(swigCPtr, this);
  }

  public void resume() {
    simgridJNI.VirtualMachine_resume(swigCPtr, this);
  }

  public void shutdown() {
    simgridJNI.VirtualMachine_shutdown(swigCPtr, this);
  }

  public void destroy() {
    simgridJNI.VirtualMachine_destroy(swigCPtr, this);
  }

  public Host get_pm() {
    long cPtr = simgridJNI.VirtualMachine_get_pm(swigCPtr, this);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  public VirtualMachine set_pm(Host pm) {
    simgridJNI.VirtualMachine_set_pm(swigCPtr, this, Host.getCPtr(pm), pm);
    return this;
  }

  public long get_ramsize() {
    return simgridJNI.VirtualMachine_get_ramsize(swigCPtr, this);
  }

  public VirtualMachine set_ramsize(double ramsize) { return set_ramsize((long)ramsize); }
  public VirtualMachine set_ramsize(long ramsize) {
    simgridJNI.VirtualMachine_set_ramsize(swigCPtr, this, ramsize);
    return this;
  }

  public VirtualMachine set_bound(double bound) {
    simgridJNI.VirtualMachine_set_bound(swigCPtr, this, bound);
    return this;
  }

  public VirtualMachine migrate(Host newHost)
  {
    simgridJNI.VirtualMachine_migrate(swigCPtr, this, Host.getCPtr(newHost), newHost);
    return this;
  }
  public void start_migration() {
    simgridJNI.VirtualMachine_start_migration(swigCPtr, this);
  }

  public void end_migration() {
    simgridJNI.VirtualMachine_end_migration(swigCPtr, this);
  }

  public VirtualMachine.State get_state() {
    return VirtualMachine.State.swigToEnum(simgridJNI.VirtualMachine_get_state(swigCPtr, this));
  }

  public static void on_start_cb(CallbackVirtualMachine cb) { simgridJNI.VirtualMachine_on_start_cb(cb); }

  public void on_this_start_cb(CallbackVirtualMachine cb)
  {
    simgridJNI.VirtualMachine_on_this_start_cb(swigCPtr, this, cb);
  }

  public static void on_started_cb(CallbackVirtualMachine cb) { simgridJNI.VirtualMachine_on_started_cb(cb); }

  public void on_this_started_cb(CallbackVirtualMachine cb)
  {
    simgridJNI.VirtualMachine_on_this_started_cb(swigCPtr, this, cb);
  }

  public static void on_shutdown_cb(CallbackVirtualMachine cb) { simgridJNI.VirtualMachine_on_shutdown_cb(cb); }

  public void on_this_shutdown_cb(CallbackVirtualMachine cb)
  {
    simgridJNI.VirtualMachine_on_this_shutdown_cb(swigCPtr, this, cb);
  }

  public void on_this_suspend_cb(CallbackVirtualMachine cb)
  {
    simgridJNI.VirtualMachine_on_this_suspend_cb(swigCPtr, this, cb);
  }

  public void on_this_resume_cb(CallbackVirtualMachine cb)
  {
    simgridJNI.VirtualMachine_on_this_resume_cb(swigCPtr, this, cb);
  }

  public static void on_destruction_cb(CallbackVirtualMachine cb) { simgridJNI.VirtualMachine_on_destruction_cb(cb); }

  public void on_this_destruction_cb(CallbackVirtualMachine cb)
  {
    simgridJNI.VirtualMachine_on_this_destruction_cb(swigCPtr, this, cb);
  }

  public static void on_migration_start_cb(CallbackVirtualMachine cb)
  {
    simgridJNI.VirtualMachine_on_migration_start_cb(cb);
  }

  public void on_this_migration_start_cb(CallbackVirtualMachine cb)
  {
    simgridJNI.VirtualMachine_on_this_migration_start_cb(swigCPtr, this, cb);
  }

  public static void on_migration_end_cb(CallbackVirtualMachine cb)
  {
    simgridJNI.VirtualMachine_on_migration_end_cb(cb);
  }

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
