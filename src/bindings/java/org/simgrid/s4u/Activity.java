/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public class Activity {
  private transient long swigCPtr;
  private transient boolean swigCMemOwnBase;

  protected Activity(long cPtr, boolean cMemoryOwn) {
    swigCMemOwnBase = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected final long getCPtr() { return swigCPtr; }

  protected static long getCPtr(Activity obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  @SuppressWarnings({"deprecation", "removal"})
  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if(swigCPtr != 0 && swigCMemOwnBase) {
      swigCMemOwnBase = false;
      simgridJNI.delete_Activity(swigCPtr);
    }
    swigCPtr = 0;
  }

  public static String to_c_str(Activity.State value) {
    return simgridJNI.Activity_to_c_str(value.swigValue());
  }

  public boolean is_assigned() {
    return simgridJNI.Activity_is_assigned(swigCPtr, this);
  }

  public boolean dependencies_solved() {
    return simgridJNI.Activity_dependencies_solved(swigCPtr, this);
  }

  public boolean has_no_successor() {
    return simgridJNI.Activity_has_no_successor(swigCPtr, this);
  }

  public Activity[] get_dependencies() { return simgridJNI.Activity_get_dependencies(swigCPtr, this); }

  public Activity[] get_successors() { return simgridJNI.Activity_get_successors(swigCPtr, this); }

  public Activity start() {
    simgridJNI.Activity_start(swigCPtr, this);
    return this;
  }

  public boolean test() {
    return simgridJNI.Activity_test(swigCPtr, this);
  }

  public Activity await_for(double timeout) throws SimgridException
  {
    simgridJNI.Activity_await_for(swigCPtr, this, timeout);
    return this;
  }

  public Activity await_for_or_cancel(double timeout) throws SimgridException
  {
    simgridJNI.Activity_await_for_or_cancel(swigCPtr, this, timeout);
    return this;
  }

  public void await_until(double time_limit) throws SimgridException
  {
    simgridJNI.Activity_await_until(swigCPtr, this, time_limit);
  }

  public Activity cancel() {
    simgridJNI.Activity_cancel(swigCPtr, this);
    return this;
  }

  public Activity.State get_state() {
    return Activity.State.swigToEnum(simgridJNI.Activity_get_state(swigCPtr, this));
  }

  public String get_state_str() {
    return simgridJNI.Activity_get_state_str(swigCPtr, this);
  }

  public boolean is_canceled() {
    return simgridJNI.Activity_is_canceled(swigCPtr, this);
  }

  public boolean is_failed() {
    return simgridJNI.Activity_is_failed(swigCPtr, this);
  }

  public boolean is_done() {
    return simgridJNI.Activity_is_done(swigCPtr, this);
  }

  public boolean is_detached() {
    return simgridJNI.Activity_is_detached(swigCPtr, this);
  }

  public Activity suspend() {
    simgridJNI.Activity_suspend(swigCPtr, this);
    return this;
  }

  public Activity resume() {
    simgridJNI.Activity_resume(swigCPtr, this);
    return this;
  }

  public boolean is_suspended() {
    return simgridJNI.Activity_is_suspended(swigCPtr, this);
  }

  public String get_name() {
    return simgridJNI.Activity_get_name(swigCPtr, this);
  }
  public Object get_data() { return simgridJNI.Activity_get_data(swigCPtr); }
  public void set_data(Object o) { simgridJNI.Activity_set_data(swigCPtr, o); }

  public double get_remaining() {
    return simgridJNI.Activity_get_remaining(swigCPtr, this);
  }

  public double get_start_time() {
    return simgridJNI.Activity_get_start_time(swigCPtr, this);
  }

  public double get_finish_time() {
    return simgridJNI.Activity_get_finish_time(swigCPtr, this);
  }

  public void mark() {
    simgridJNI.Activity_mark(swigCPtr, this);
  }

  public boolean is_marked() {
    return simgridJNI.Activity_is_marked(swigCPtr, this);
  }

  public final static class State {
    public final static Activity.State INITED = new Activity.State("INITED");
    public final static Activity.State STARTING = new Activity.State("STARTING");
    public final static Activity.State STARTED = new Activity.State("STARTED");
    public final static Activity.State FAILED = new Activity.State("FAILED");
    public final static Activity.State CANCELED = new Activity.State("CANCELED");
    public final static Activity.State FINISHED = new Activity.State("FINISHED");

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

    private static State[] swigValues = { INITED, STARTING, STARTED, FAILED, CANCELED, FINISHED };
    private static int swigNext = 0;
    private final int swigValue;
    private final String swigName;
  }

}
