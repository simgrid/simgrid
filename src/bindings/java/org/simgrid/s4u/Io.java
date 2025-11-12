/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public class Io extends Activity {
  protected Io(long cPtr, boolean cMemoryOwn) { super(cPtr, cMemoryOwn); }

  public static Io init() {
    long cPtr = simgridJNI.Io_init();
    return (cPtr == 0) ? null : new Io(cPtr, true);
  }

  public double get_remaining() { return simgridJNI.Io_get_remaining(getCPtr(), this); }

  public int get_performed_ioops() { return simgridJNI.Io_get_performed_ioops(getCPtr(), this); }

  public Io set_disk(Disk disk) {
    simgridJNI.Io_set_disk(getCPtr(), this, Disk.getCPtr(disk), disk);
    return this;
  }

  public Io set_priority(double priority) {
    simgridJNI.Io_set_priority(getCPtr(), this, priority);
    return this;
  }

  public Io set_size(double size) { return set_size((int)size); }
  public Io set_size(int size) {
    simgridJNI.Io_set_size(getCPtr(), this, size);
    return this;
  }

  public Io set_op_type(Io.OpType type) {
    simgridJNI.Io_set_op_type(getCPtr(), this, type.swigValue());
    return this;
  }

  public static Io streamto_init(Host from, Disk from_disk, Host to, Disk to_disk) {
    long cPtr = simgridJNI.Io_streamto_init(Host.getCPtr(from), from, Disk.getCPtr(from_disk), from_disk, Host.getCPtr(to), to, Disk.getCPtr(to_disk), to_disk);
    return (cPtr == 0) ? null : new Io(cPtr, true);
  }

  public static Io streamto_async(Host from, Disk from_disk, Host to, Disk to_disk, long simulated_size_in_bytes) {
    long cPtr = simgridJNI.Io_streamto_async(Host.getCPtr(from), from, Disk.getCPtr(from_disk), from_disk, Host.getCPtr(to), to, Disk.getCPtr(to_disk), to_disk, simulated_size_in_bytes);
    return (cPtr == 0) ? null : new Io(cPtr, true);
  }

  public static void streamto(Host from, Disk from_disk, Host to, Disk to_disk, long simulated_size_in_bytes) {
    simgridJNI.Io_streamto(Host.getCPtr(from), from, Disk.getCPtr(from_disk), from_disk, Host.getCPtr(to), to, Disk.getCPtr(to_disk), to_disk, simulated_size_in_bytes);
  }

  public Io set_source(Host from, Disk from_disk) {
    simgridJNI.Io_set_source(getCPtr(), this, Host.getCPtr(from), from, Disk.getCPtr(from_disk), from_disk);
    return this;
  }

  public Io set_destination(Host to, Disk to_disk) {
    simgridJNI.Io_set_destination(getCPtr(), this, Host.getCPtr(to), to, Disk.getCPtr(to_disk), to_disk);
    return this;
  }

  public Io update_priority(double priority) {
    simgridJNI.Io_update_priority(getCPtr(), this, priority);
    return this;
  }

  public boolean is_assigned() { return simgridJNI.Io_is_assigned(getCPtr(), this); }

  public static void on_start_cb(CallbackIo cb) { simgridJNI.Io_on_start_cb(cb); }

  public void on_this_start_cb(CallbackIo cb) { simgridJNI.Io_on_this_start_cb(getCPtr(), this, cb); }

  public static void on_completion_cb(CallbackIo cb) { simgridJNI.Io_on_completion_cb(cb); }

  public void on_this_completion_cb(CallbackIo cb) { simgridJNI.Io_on_this_completion_cb(getCPtr(), this, cb); }

  public void on_this_suspend_cb(CallbackIo cb) { simgridJNI.Io_on_this_suspend_cb(getCPtr(), this, cb); }

  public void on_this_resume_cb(CallbackIo cb) { simgridJNI.Io_on_this_resume_cb(getCPtr(), this, cb); }

  public static void on_veto_cb(CallbackIo cb) { simgridJNI.Io_on_veto_cb(cb); }

  public void on_this_veto_cb(CallbackIo cb) { simgridJNI.Io_on_this_veto_cb(getCPtr(), this, cb); }

  public Io add_successor(Activity a) {
    simgridJNI.Io_add_successor(getCPtr(), this, Activity.getCPtr(a), a);
    return this;
  }

  public Io remove_successor(Activity a) {
    simgridJNI.Io_remove_successor(getCPtr(), this, Activity.getCPtr(a), a);
    return this;
  }

  public Io set_name(String name) {
    simgridJNI.Io_set_name(getCPtr(), this, name);
    return this;
  }

  public String get_name() { return simgridJNI.Io_get_name(getCPtr(), this); }

  public Io set_tracing_category(String category) {
    simgridJNI.Io_set_tracing_category(getCPtr(), this, category);
    return this;
  }

  public String get_tracing_category() { return simgridJNI.Io_get_tracing_category(getCPtr(), this); }

  public Io detach() {
    simgridJNI.Io_detach__SWIG_0(getCPtr(), this);
    return this;
  }

  public Io detach(CallbackIo clean_function)
  {
    simgridJNI.Io_detach__SWIG_1(getCPtr(), this, clean_function);
    return this;
  }

  public Io cancel() {
    simgridJNI.Io_cancel(getCPtr(), this);
    return this;
  }

  public Io await() throws TimeoutException { return await_for(-1); }
  public Io await_for(double timeout) throws TimeoutException
  {
    simgridJNI.Io_await_for(getCPtr(), this, timeout);
    return this;
  }

  public final static class OpType {
    public final static Io.OpType READ = new Io.OpType("READ");
    public final static Io.OpType WRITE = new Io.OpType("WRITE");

    public final int swigValue() {
      return swigValue;
    }

    public String toString() {
      return swigName;
    }

    public static OpType swigToEnum(int swigValue) {
      if (swigValue < swigValues.length && swigValue >= 0 && swigValues[swigValue].swigValue == swigValue)
        return swigValues[swigValue];
      for (int i = 0; i < swigValues.length; i++)
        if (swigValues[i].swigValue == swigValue)
          return swigValues[i];
      throw new IllegalArgumentException("No enum " + OpType.class + " with value " + swigValue);
    }

    private OpType(String swigName) {
      this.swigName = swigName;
      this.swigValue = swigNext++;
    }

    private OpType(String swigName, int swigValue) {
      this.swigName = swigName;
      this.swigValue = swigValue;
      swigNext = swigValue+1;
    }

    private OpType(String swigName, OpType swigEnum) {
      this.swigName = swigName;
      this.swigValue = swigEnum.swigValue;
      swigNext = this.swigValue+1;
    }

    private static OpType[] swigValues = { READ, WRITE };
    private static int swigNext = 0;
    private final int swigValue;
    private final String swigName;
  }
}
