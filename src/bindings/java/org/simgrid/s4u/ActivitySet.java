/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public class ActivitySet {
  private transient long swigCPtr;
  private transient boolean swigCMemOwnBase;

  public ActivitySet()
  {
    swigCMemOwnBase = true;
    swigCPtr        = simgridJNI.new_ActivitySet();
  }

  protected ActivitySet(long cPtr, boolean cMemoryOwn)
  {
    swigCMemOwnBase = cMemoryOwn;
    swigCPtr        = cPtr;
  }

  protected static long getCPtr(ActivitySet obj) { return (obj == null) ? 0 : obj.swigCPtr; }

  @SuppressWarnings({"deprecation", "removal"}) protected void finalize() { delete(); }

  public synchronized void delete()
  {
    if (swigCPtr != 0 && swigCMemOwnBase) {
      swigCMemOwnBase = false;
      simgridJNI.delete_ActivitySet(swigCPtr);
    }
    swigCPtr = 0;
  }

  /** Add an activity to the set */
  public void push(Activity a) { simgridJNI.ActivitySet_push(swigCPtr, this, Activity.getCPtr(a), a); }
  /** Remove that activity from the set (no-op if the activity is not in the set) */
  public void erase(Activity a) { simgridJNI.ActivitySet_erase(swigCPtr, this, Activity.getCPtr(a), a); }

  /** Get the amount of activities in the set. Failed activities (if any) are not counted */
  public int size() { return simgridJNI.ActivitySet_size(swigCPtr, this); }
  /** Return whether the set is empty. Failed activities (if any) are not counted */
  public boolean empty() { return simgridJNI.ActivitySet_empty(swigCPtr, this); }
  public void clear() { simgridJNI.ActivitySet_clear(swigCPtr, this); }

  /** Access to one specific activity in the set */
  public Activity at(int index) { return simgridJNI.ActivitySet_at(swigCPtr, this, index); }

  /**
   * Wait for the completion of all activities in the set, but not longer than the provided timeout
   *
   * On timeout, an exception is raised.
   *
   * In any case, the completed activities remain in the set. Use test_any() to retrieve them.
   */
  public void await_all_for(double timeout) throws SimgridException
  {
    simgridJNI.ActivitySet_await_all_for(swigCPtr, this, timeout);
  }
  /** Wait for the completion of all activities in the set. The set is NOT emptied afterward. */
  public void await_all() throws SimgridException { await_all_for(-1); }
  /** Returns the first terminated activity if any, or ActivityPtr(nullptr) if no activity is terminated */
  public Activity test_any() { return simgridJNI.ActivitySet_test_any(swigCPtr, this); }

  /**
   * Wait for the completion of one activity from the set, but not longer than the provided timeout.
   *
   *  See wait_any() for details.
   *
   * @return the first terminated activity, which is automatically removed from the set.
   */

  public Activity await_any_for(double timeout) throws SimgridException
  {
    return simgridJNI.ActivitySet_await_any_for(swigCPtr, this, timeout);
  }
  /**
   * Wait for the completion of one activity from the set.
   *
   * If an activity fails during that time, an exception is raised, and the failed exception is marked as failed in the
   * set. Use get_failed_activity() to retrieve it.
   *
   * If more than one activity failed, the other ones are also removed from the set. Use get_failed_activity() several
   * time to retrieve them all.
   *
   * @return the first terminated activity, which is automatically removed from the set. If more than one activity
   * terminated at the same timestamp, then the other ones are still in the set. Use either test_any() or await_any() to
   * retrieve the other ones.
   */
  public Activity await_any() throws SimgridException { return await_any_for(-1); }

  /**
   * Return one of the failed activity of the set that was revealed during the previous await operation, or
   * ActivityPtr() if no failed activity exist in the set.
   */
  public Activity get_failed_activity() { return simgridJNI.ActivitySet_get_failed_activity(swigCPtr, this); }
  /** Return whether the set contains any failed activity. */
  public boolean has_failed_activities() { return simgridJNI.ActivitySet_has_failed_activity(swigCPtr, this); }
}
