/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

/**
 * An Activity is the ancestor of every activity that an actor can undertake, i.e., anything that takes time in
 * the simulated world. You cannot create an Activity directly: use the relevant subclasses (Comm, Exec, Io)
 * instead.
 */
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

  /**
   * Returns whether this activity is assigned to the resource(s) that it needs to start (e.g., some Links for a
   *  Comm, a Host for an Exec, a Disk for an Io). An activity cannot start before it is assigned.
   */
  public boolean is_assigned() {
    return simgridJNI.Activity_is_assigned(swigCPtr, this);
  }

  /**
   * Returns whether all activities that this one depends on are completed, meaning that this activity can now
   *  start (once it is also assigned).
   */
  public boolean dependencies_solved() {
    return simgridJNI.Activity_dependencies_solved(swigCPtr, this);
  }

  /** Returns whether no other activity depends on this one. */
  public boolean has_no_successor() {
    return simgridJNI.Activity_has_no_successor(swigCPtr, this);
  }

  /**
   * Retrieve the set of activities that this activity depends on, i.e., the ones that must complete before this
   *  activity can start.
   */
  public Activity[] get_dependencies() { return simgridJNI.Activity_get_dependencies(swigCPtr, this); }

  /** Retrieve the list of activities that depend on this one. */
  public Activity[] get_successors() { return simgridJNI.Activity_get_successors(swigCPtr, this); }

  /**
   * Starts a previously created activity. This function is optional: you can call await_for() even if you
   *  didn't call start()
   */
  public Activity start() {
    simgridJNI.Activity_start(swigCPtr, this);
    return this;
  }

  /** Tests whether this activity is terminated yet. This call does not block. */
  public boolean test() {
    return simgridJNI.Activity_test(swigCPtr, this);
  }

  /**
   * Blocks the current actor until the activity is terminated, or until the timeout is elapsed. Raises a
   *  SimgridException on timeout.
   */
  public Activity await_for(double timeout) throws SimgridException
  {
    simgridJNI.Activity_await_for(swigCPtr, this, timeout);
    return this;
  }

  /** Just like await_for(), but the activity is first canceled if a timeout exception is raised. */
  public Activity await_for_or_cancel(double timeout) throws SimgridException
  {
    simgridJNI.Activity_await_for_or_cancel(swigCPtr, this, timeout);
    return this;
  }

  /**
   * Blocks the current actor until the activity is terminated, or until the time limit is reached. Raises a
   *  SimgridException on timeout.
   */
  public void await_until(double time_limit) throws SimgridException
  {
    simgridJNI.Activity_await_until(swigCPtr, this, time_limit);
  }

  /** Cancel that activity */
  public Activity cancel() {
    simgridJNI.Activity_cancel(swigCPtr, this);
    return this;
  }

  /** Retrieve the current state of the activity */
  public Activity.State get_state() {
    return Activity.State.swigToEnum(simgridJNI.Activity_get_state(swigCPtr, this));
  }

  /**
   * Return a string representation of the activity's state (one of INITED, STARTING, STARTED, FAILED, CANCELED,
   *  FINISHED)
   */
  public String get_state_str() {
    return simgridJNI.Activity_get_state_str(swigCPtr, this);
  }

  /** Returns whether this activity was canceled with cancel(). */
  public boolean is_canceled() {
    return simgridJNI.Activity_is_canceled(swigCPtr, this);
  }

  /** Returns whether this activity has failed, e.g. because of a resource failure. */
  public boolean is_failed() {
    return simgridJNI.Activity_is_failed(swigCPtr, this);
  }

  /** Returns whether this activity is successfully completed. */
  public boolean is_done() {
    return simgridJNI.Activity_is_done(swigCPtr, this);
  }

  /**
   * Returns whether this activity was detached, i.e. started with detach() rather than start(), meaning that no
   *  actor is going to wait for its completion.
   */
  public boolean is_detached() {
    return simgridJNI.Activity_is_detached(swigCPtr, this);
  }

  /** Blocks the progression of this activity until it gets resumed with resume(). */
  public Activity suspend() {
    simgridJNI.Activity_suspend(swigCPtr, this);
    return this;
  }

  /** Unblock the progression of this activity if it was suspended previously with suspend(). */
  public Activity resume() {
    simgridJNI.Activity_resume(swigCPtr, this);
    return this;
  }

  /**
   * Returns whether the progression of this activity is currently blocked, i.e. whether it was suspended with
   *  suspend() and not yet resumed.
   */
  public boolean is_suspended() {
    return simgridJNI.Activity_is_suspended(swigCPtr, this);
  }

  /** Retrieve the name of this activity */
  public String get_name() {
    return simgridJNI.Activity_get_name(swigCPtr, this);
  }
  /** Retrieve the user data that was attached to this activity with set_data(), if any. */
  public Object get_data() { return simgridJNI.Activity_get_data(swigCPtr); }
  /**
   * Attach arbitrary user data to this activity. SimGrid does not touch this data, but you can retrieve it
   *  later with get_data().
   */
  public void set_data(Object o) { simgridJNI.Activity_set_data(swigCPtr, o); }

  /** Get the remaining amount of work that this Activity entails. When it's 0, it's done. */
  public double get_remaining() {
    return simgridJNI.Activity_get_remaining(swigCPtr, this);
  }

  /** Retrieve the simulated timestamp at which this activity started. */
  public double get_start_time() {
    return simgridJNI.Activity_get_start_time(swigCPtr, this);
  }

  /** Retrieve the simulated timestamp at which this activity finished. */
  public double get_finish_time() {
    return simgridJNI.Activity_get_finish_time(swigCPtr, this);
  }

  /**
   * Mark this activity, so that it can be recognized later on with is_marked(). This has no effect on the
   *  simulation, it is only meant to help users track activities of interest, e.g. after retrieving them from an
   *  ActivitySet.
   */
  public void mark() {
    simgridJNI.Activity_mark(swigCPtr, this);
  }

  /** Returns whether this activity was previously marked with mark(). */
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
