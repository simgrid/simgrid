/* Copyright (c) 2024-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public class Mess extends Activity {
  private transient long swigCPtr;
  private transient boolean swigCMemOwnDerived;

  protected Mess(long cPtr, boolean cMemoryOwn)
  {
    super(cPtr, true);
    swigCMemOwnDerived = cMemoryOwn;
    swigCPtr           = cPtr;
  }

  protected static long getCPtr(Mess obj) { return (obj == null) ? 0 : obj.swigCPtr; }

  @SuppressWarnings({"deprecation", "removal"}) protected void finalize() { delete(); }

  public synchronized void delete()
  {
    if (swigCPtr != 0 && swigCMemOwnDerived) {
      swigCMemOwnDerived = false;
      simgridJNI.delete_Mess(swigCPtr);
    }
    swigCPtr = 0;
    super.delete();
  }

  public Mess set_queue(MessageQueue queue)
  {
    simgridJNI.Mess_set_queue(swigCPtr, this, MessageQueue.getCPtr(queue), queue);
    return this;
  }

  public MessageQueue get_queue()
  {
    long cPtr = simgridJNI.Mess_get_queue(swigCPtr, this);
    return (cPtr == 0) ? null : new MessageQueue(cPtr, false);
  }

  public Mess set_payload(Object payload)
  {
    simgridJNI.Mess_set_payload(swigCPtr, this, payload);
    return this;
  }

  public Object get_payload() { return simgridJNI.Mess_get_payload(swigCPtr, this); }

  public Actor get_sender() { return simgridJNI.Mess_get_sender(swigCPtr, this); }
  public boolean is_assigned() { return simgridJNI.Mess_is_assigned(swigCPtr, this); }

  public Actor get_receiver() { return simgridJNI.Mess_get_receiver(swigCPtr, this); }

  public Mess add_successor(Activity a)
  {
    simgridJNI.Mess_add_successor(swigCPtr, this, Activity.getCPtr(a), a);
    return this;
  }

  public Mess remove_successor(Activity a)
  {
    simgridJNI.Mess_remove_successor(swigCPtr, this, Activity.getCPtr(a), a);
    return this;
  }

  public Mess set_name(String name)
  {
    simgridJNI.Mess_set_name(swigCPtr, this, name);
    return this;
  }

  public String get_name() { return simgridJNI.Mess_get_name(swigCPtr, this); }

  public Mess set_tracing_category(String category)
  {
    simgridJNI.Mess_set_tracing_category(swigCPtr, this, category);
    return this;
  }

  public String get_tracing_category() { return simgridJNI.Mess_get_tracing_category(swigCPtr, this); }

  public Mess cancel()
  {
    simgridJNI.Mess_cancel(swigCPtr, this);
    return this;
  }

  public Mess await_for(double timeout)
  {
    simgridJNI.Mess_await_for(swigCPtr, this, timeout);
    return this;
  }

  public Mess await()
  {
    await_for(-1);
    return this;
  }
}
