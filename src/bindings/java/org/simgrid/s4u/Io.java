/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

/**
 * I/O Activity, representing an asynchronous disk access.
 *
 * They are generated from the read/write methods of Disk. You can also create direct host-to-host I/O streams
 * (bypassing the mailbox and actor mechanisms) with streamto(), streamto_init() and streamto_async().
 */
public class Io extends Activity {
  protected Io(long cPtr, boolean cMemoryOwn) { super(cPtr, cMemoryOwn); }

  /** Initiate the creation of an I/O. Setters have to be called afterwards */
  public static Io init() {
    long cPtr = simgridJNI.Io_init();
    return (cPtr == 0) ? null : new Io(cPtr, true);
  }

  /** Get the remaining amount of bytes to transfer for this I/O. When it's 0, it's done. */
  public double get_remaining() { return simgridJNI.Io_get_remaining(getCPtr(), this); }

  /** Retrieve the amount of I/O operations (e.g. individual disk blocks) already performed by this activity. */
  public int get_performed_ioops() { return simgridJNI.Io_get_performed_ioops(getCPtr(), this); }

  /**
   * Specify the disk on which this I/O operation must take place. Not to be used for direct host-to-host
   *  streams: use set_source()/set_destination() instead.
   */
  public Io set_disk(Disk disk) {
    simgridJNI.Io_set_disk(getCPtr(), this, Disk.getCPtr(disk), disk);
    return this;
  }

  /**
   * Change the I/O priority: an I/O with twice the priority will get twice the amount of bytes transferred
   *  when the resource is shared. The default priority is 1. Currently, this cannot be changed once the I/O
   *  started. See also update_priority().
   */
  public Io set_priority(double priority) {
    simgridJNI.Io_set_priority(getCPtr(), this, priority);
    return this;
  }

  public Io set_size(double size) { return set_size((int)size); }
  /** Specify the amount of bytes to read or write during this I/O. */
  public Io set_size(int size) {
    simgridJNI.Io_set_size(getCPtr(), this, size);
    return this;
  }

  /** Specify whether this I/O is a read or a write operation. */
  public Io set_op_type(Io.OpType type) {
    simgridJNI.Io_set_op_type(getCPtr(), this, type.swigValue());
    return this;
  }

  /**
   * Creates a direct I/O stream between two arbitrary hosts and their disks, bypassing the mailbox and actor
   *  mechanisms. Source and destination have to be set with set_source() and set_destination() before the I/O
   *  can start.
   */
  public static Io streamto_init(Host from, Disk from_disk, Host to, Disk to_disk) {
    long cPtr = simgridJNI.Io_streamto_init(Host.getCPtr(from), from, Disk.getCPtr(from_disk), from_disk, Host.getCPtr(to), to, Disk.getCPtr(to_disk), to_disk);
    return (cPtr == 0) ? null : new Io(cPtr, true);
  }

  /**
   * Creates and starts a direct, asynchronous I/O stream between the disk @a from_disk of host @a from and the
   *  disk @a to_disk of host @a to, bypassing the mailbox and actor mechanisms.
   */
  public static Io streamto_async(Host from, Disk from_disk, Host to, Disk to_disk, long simulated_size_in_bytes) {
    long cPtr = simgridJNI.Io_streamto_async(Host.getCPtr(from), from, Disk.getCPtr(from_disk), from_disk, Host.getCPtr(to), to, Disk.getCPtr(to_disk), to_disk, simulated_size_in_bytes);
    return (cPtr == 0) ? null : new Io(cPtr, true);
  }

  /**
   * Do a blocking I/O stream between two arbitrary hosts and their disks, bypassing the mailbox and actor
   *  mechanisms.
   */
  public static void streamto(Host from, Disk from_disk, Host to, Disk to_disk, long simulated_size_in_bytes) {
    simgridJNI.Io_streamto(Host.getCPtr(from), from, Disk.getCPtr(from_disk), from_disk, Host.getCPtr(to), to, Disk.getCPtr(to_disk), to_disk, simulated_size_in_bytes);
  }

  /**
   * Specify the source host and disk of a direct host-to-host I/O stream (see streamto()). Must be set
   *  together with set_destination(), before the I/O starts.
   */
  public Io set_source(Host from, Disk from_disk) {
    simgridJNI.Io_set_source(getCPtr(), this, Host.getCPtr(from), from, Disk.getCPtr(from_disk), from_disk);
    return this;
  }

  /**
   * Specify the destination host and disk of a direct host-to-host I/O stream (see streamto()). Must be set
   *  together with set_source(), before the I/O starts.
   */
  public Io set_destination(Host to, Disk to_disk) {
    simgridJNI.Io_set_destination(getCPtr(), this, Host.getCPtr(to), to, Disk.getCPtr(to_disk), to_disk);
    return this;
  }

  /** Change the I/O priority while it is already running. See also set_priority(). */
  public Io update_priority(double priority) {
    simgridJNI.Io_update_priority(getCPtr(), this, priority);
    return this;
  }

  /**
   * Returns whether this I/O is assigned to the disk(s) that it needs to start (or to a source and destination
   *  host, for direct I/O streams). An unassigned I/O cannot start.
   */
  public boolean is_assigned() { return simgridJNI.Io_is_assigned(getCPtr(), this); }

  /** Add a callback fired when any Io starts (no veto) */
  public static void on_start_cb(CallbackIo cb) { simgridJNI.Io_on_start_cb(cb); }

  /** Add a callback fired when this specific Io starts (no veto) */
  public void on_this_start_cb(CallbackIo cb) { simgridJNI.Io_on_this_start_cb(getCPtr(), this, cb); }

  /** Add a callback fired when any Io completes (either normally, cancelled or failed) */
  public static void on_completion_cb(CallbackIo cb) { simgridJNI.Io_on_completion_cb(cb); }

  /** Add a callback fired when this specific Io completes (either normally, cancelled or failed) */
  public void on_this_completion_cb(CallbackIo cb) { simgridJNI.Io_on_this_completion_cb(getCPtr(), this, cb); }

  /** Add a callback fired when this specific Io is suspended */
  public void on_this_suspend_cb(CallbackIo cb) { simgridJNI.Io_on_this_suspend_cb(getCPtr(), this, cb); }

  /** Add a callback fired when this specific Io is resumed after being suspended */
  public void on_this_resume_cb(CallbackIo cb) { simgridJNI.Io_on_this_resume_cb(getCPtr(), this, cb); }

  /**
   * Add a callback fired each time that any Io fails to start because of a veto (e.g., unsolved dependency or
   *  no disk assigned)
   */
  public static void on_veto_cb(CallbackIo cb) { simgridJNI.Io_on_veto_cb(cb); }

  /** Add a callback fired each time that this specific Io fails to start because of a veto */
  public void on_this_veto_cb(CallbackIo cb) { simgridJNI.Io_on_this_veto_cb(getCPtr(), this, cb); }

  /** Add a dependency from this Io to activity @a a: @a a will not start before this Io is done. */
  public Io add_successor(Activity a) {
    simgridJNI.Io_add_successor(getCPtr(), this, Activity.getCPtr(a), a);
    return this;
  }

  /** Remove a dependency previously declared with add_successor(). */
  public Io remove_successor(Activity a) {
    simgridJNI.Io_remove_successor(getCPtr(), this, Activity.getCPtr(a), a);
    return this;
  }

  /** Set the name of this I/O, for logging and tracing purposes. */
  public Io set_name(String name) {
    simgridJNI.Io_set_name(getCPtr(), this, name);
    return this;
  }

  /** Retrieve the name of this I/O. */
  public String get_name() { return simgridJNI.Io_get_name(getCPtr(), this); }

  /** Set a user-defined tracing category. Must be called before start(). */
  public Io set_tracing_category(String category) {
    simgridJNI.Io_set_tracing_category(getCPtr(), this, category);
    return this;
  }

  /**
   * Retrieve the tracing category previously set with set_tracing_category(), or an empty string if none was
   *  set.
   */
  public String get_tracing_category() { return simgridJNI.Io_get_tracing_category(getCPtr(), this); }

  /** Start the I/O, and ignore its result. It can be completely forgotten after that. */
  public Io detach() {
    simgridJNI.Io_detach__SWIG_0(getCPtr(), this);
    return this;
  }

  /** Start the I/O, and ignore its result, calling @a clean_function on the I/O's data once it completes. */
  public Io detach(CallbackIo clean_function)
  {
    simgridJNI.Io_detach__SWIG_1(getCPtr(), this, clean_function);
    return this;
  }

  /** Cancel that I/O operation. */
  public Io cancel() {
    simgridJNI.Io_cancel(getCPtr(), this);
    return this;
  }

  /** Blocks the current actor until the I/O is terminated. Raises a TimeoutException on failure. */
  public Io await() throws TimeoutException { return await_for(-1); }
  /**
   * Blocks the current actor until the I/O is terminated, or until the timeout is elapsed. Raises a
   *  TimeoutException on failure.
   */
  public Io await_for(double timeout) throws TimeoutException
  {
    simgridJNI.Io_await_for(getCPtr(), this, timeout);
    return this;
  }

  /** The kind of I/O operation: reading from, or writing to, a disk. */
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
