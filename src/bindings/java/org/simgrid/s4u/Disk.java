/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public class Disk {
  private transient long swigCPtr;

  protected Disk(long cPtr) { swigCPtr = cPtr; }

  protected static long getCPtr(Disk obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  public String get_name() {
    return simgridJNI.Disk_get_name(swigCPtr, this);
  }

  public Disk set_read_bandwidth(double read_bw) {
    simgridJNI.Disk_set_read_bandwidth(swigCPtr, this, read_bw);
    return this;
  }

  public double get_read_bandwidth() {
    return simgridJNI.Disk_get_read_bandwidth(swigCPtr, this);
  }

  public Disk set_write_bandwidth(double write_bw) {
    simgridJNI.Disk_set_write_bandwidth(swigCPtr, this, write_bw);
    return this;
  }

  public double get_write_bandwidth() {
    return simgridJNI.Disk_get_write_bandwidth(swigCPtr, this);
  }

  public Disk set_readwrite_bandwidth(double bw) {
    simgridJNI.Disk_set_readwrite_bandwidth(swigCPtr, this, bw);
    return this;
  }

  public String get_property(String key) {
    return simgridJNI.Disk_get_property(swigCPtr, this, key);
  }

  public Disk set_property(String arg0, String value) {
    simgridJNI.Disk_set_property(swigCPtr, this, arg0, value);
    return this;
  }

  public Disk set_host(Host host) {
    simgridJNI.Disk_set_host(swigCPtr, this, Host.getCPtr(host), host);
    return this;
  }

  public Host get_host() {
    long cPtr = simgridJNI.Disk_get_host(swigCPtr, this);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  public Disk set_concurrency_limit(int limit) {
    simgridJNI.Disk_set_concurrency_limit(swigCPtr, this, limit);
    return this;
  }

  public int get_concurrency_limit() {
    return simgridJNI.Disk_get_concurrency_limit(swigCPtr, this);
  }

  public Io io_init(int size, Io.OpType type) {
    long cPtr = simgridJNI.Disk_io_init(swigCPtr, this, size, type.swigValue());
    return (cPtr == 0) ? null : new Io(cPtr, true);
  }

  public Io read_async(double size) { return read_async((long)size); }
  public Io read_async(long size) {
    long cPtr = simgridJNI.Disk_read_async(swigCPtr, this, size);
    return (cPtr == 0) ? null : new Io(cPtr, true);
  }

  public long read(long size) {
    return simgridJNI.Disk_read__SWIG_0(swigCPtr, this, size);
  }

  public long read(long size, double priority) {
    return simgridJNI.Disk_read__SWIG_1(swigCPtr, this, size, priority);
  }

  public Io write_async(long size) {
    long cPtr = simgridJNI.Disk_write_async(swigCPtr, this, size);
    return (cPtr == 0) ? null : new Io(cPtr, true);
  }

  public long write(long size) {
    return simgridJNI.Disk_write__SWIG_0(swigCPtr, this, size);
  }

  public long write(long size, double priority) {
    return simgridJNI.Disk_write__SWIG_1(swigCPtr, this, size, priority);
  }

  public Disk.SharingPolicy get_sharing_policy(Disk.Operation op) {
    return Disk.SharingPolicy.swigToEnum(simgridJNI.Disk_get_sharing_policy(swigCPtr, this, op.swigValue()));
  }

  public Object get_data() { return simgridJNI.Disk_get_data(swigCPtr, this); }
  public void set_data(Object data) { simgridJNI.Disk_set_data(swigCPtr, this, data); }

  public Disk seal() {
    simgridJNI.Disk_seal(swigCPtr, this);
    return this;
  }

  public static void on_onoff_cb(CallbackDisk cb) { simgridJNI.Disk_on_onoff_cb(cb); }

  public void on_this_onoff_cb(CallbackDisk cb) { simgridJNI.Disk_on_this_onoff_cb(swigCPtr, this, cb); }

  public static void on_read_bandwidth_change_cb(CallbackDisk cb) { simgridJNI.Disk_on_read_bandwidth_change_cb(cb); }

  public void on_this_read_bandwidth_change_cb(CallbackDisk cb)
  {
    simgridJNI.Disk_on_this_read_bandwidth_change_cb(swigCPtr, this, cb);
  }

  public static void on_write_bandwidth_change_cb(CallbackDisk cb) { simgridJNI.Disk_on_write_bandwidth_change_cb(cb); }

  public void on_this_write_bandwidth_change_cb(CallbackDisk cb)
  {
    simgridJNI.Disk_on_this_write_bandwidth_change_cb(swigCPtr, this, cb);
  }

  public static void on_destruction_cb(CallbackDisk cb) { simgridJNI.Disk_on_destruction_cb(cb); }

  public void on_this_destruction_cb(CallbackDisk cb) { simgridJNI.Disk_on_this_destruction_cb(swigCPtr, this, cb); }

  public final static class SharingPolicy {
    public final static Disk.SharingPolicy NONLINEAR = new Disk.SharingPolicy("NONLINEAR", simgridJNI.Disk_SharingPolicy_NONLINEAR_get());
    public final static Disk.SharingPolicy LINEAR = new Disk.SharingPolicy("LINEAR", simgridJNI.Disk_SharingPolicy_LINEAR_get());

    public final int swigValue() {
      return swigValue;
    }

    public String toString() {
      return swigName;
    }

    public static SharingPolicy swigToEnum(int swigValue) {
      if (swigValue < swigValues.length && swigValue >= 0 && swigValues[swigValue].swigValue == swigValue)
        return swigValues[swigValue];
      for (int i = 0; i < swigValues.length; i++)
        if (swigValues[i].swigValue == swigValue)
          return swigValues[i];
      throw new IllegalArgumentException("No enum " + SharingPolicy.class + " with value " + swigValue);
    }

    private SharingPolicy(String swigName) {
      this.swigName = swigName;
      this.swigValue = swigNext++;
    }

    private SharingPolicy(String swigName, int swigValue) {
      this.swigName = swigName;
      this.swigValue = swigValue;
      swigNext = swigValue+1;
    }

    private SharingPolicy(String swigName, SharingPolicy swigEnum) {
      this.swigName = swigName;
      this.swigValue = swigEnum.swigValue;
      swigNext = this.swigValue+1;
    }

    private static SharingPolicy[] swigValues = { NONLINEAR, LINEAR };
    private static int swigNext = 0;
    private final int swigValue;
    private final String swigName;
  }

  public final static class Operation {
    public final static Disk.Operation READ = new Disk.Operation("READ", simgridJNI.Disk_Operation_READ_get());
    public final static Disk.Operation WRITE = new Disk.Operation("WRITE", simgridJNI.Disk_Operation_WRITE_get());
    public final static Disk.Operation READWRITE = new Disk.Operation("READWRITE", simgridJNI.Disk_Operation_READWRITE_get());

    public final int swigValue() {
      return swigValue;
    }

    public String toString() {
      return swigName;
    }

    public static Operation swigToEnum(int swigValue) {
      if (swigValue < swigValues.length && swigValue >= 0 && swigValues[swigValue].swigValue == swigValue)
        return swigValues[swigValue];
      for (int i = 0; i < swigValues.length; i++)
        if (swigValues[i].swigValue == swigValue)
          return swigValues[i];
      throw new IllegalArgumentException("No enum " + Operation.class + " with value " + swigValue);
    }

    private Operation(String swigName) {
      this.swigName = swigName;
      this.swigValue = swigNext++;
    }

    private Operation(String swigName, int swigValue) {
      this.swigName = swigName;
      this.swigValue = swigValue;
      swigNext = swigValue+1;
    }

    private Operation(String swigName, Operation swigEnum) {
      this.swigName = swigName;
      this.swigValue = swigEnum.swigValue;
      swigNext = this.swigValue+1;
    }

    private static Operation[] swigValues = { READ, WRITE, READWRITE };
    private static int swigNext = 0;
    private final int swigValue;
    private final String swigName;
  }

}
