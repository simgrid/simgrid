/* Copyright (c) 2024-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
package org.simgrid.s4u;

public class Host {
  private transient long swigCPtr;

  protected Host(long cPtr) { swigCPtr = cPtr; }

  protected static long getCPtr(Host obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  public String get_name() {
    return simgridJNI.Host_get_name(swigCPtr, this);
  }

  public double get_speed() {
    return simgridJNI.Host_get_speed(swigCPtr, this);
  }

  public boolean is_on() { return simgridJNI.Host_is_on(swigCPtr, this); }

  public static Host current()
  {
    long cPtr = simgridJNI.Host_current();
    return (cPtr == 0) ? null : new Host(cPtr);
  }
  public Disk create_disk(String name, double read_bandwidth, double write_bandwidth)
  {
    long cPtr = simgridJNI.Host_create_disk(swigCPtr, this, name, read_bandwidth, write_bandwidth);
    return (cPtr == 0) ? null : new Disk(cPtr);
  }
  public String[] get_properties_names() { return simgridJNI.Host_get_properties_names(swigCPtr, this); }
  public String get_property(String name) { return simgridJNI.Host_get_property(swigCPtr, this, name); }

  public Disk[] get_disks() { return simgridJNI.Host_get_disks(swigCPtr, this); }

  public VirtualMachine create_vm(String name, int core_amount)
  {
    long cPtr = simgridJNI.Host_create_vm(swigCPtr, this, name, core_amount);
    return (cPtr == 0) ? null : new VirtualMachine(cPtr);
  }
}
