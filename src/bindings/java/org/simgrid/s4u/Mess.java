/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public class Mess extends Activity {
  protected Mess(long cPtr, boolean cMemoryOwn) { super(cPtr, cMemoryOwn); }

  public Mess set_queue(MessageQueue queue)
  {
    simgridJNI.Mess_set_queue(getCPtr(), this, MessageQueue.getCPtr(queue), queue);
    return this;
  }

  public MessageQueue get_queue()
  {
    long cPtr = simgridJNI.Mess_get_queue(getCPtr(), this);
    return (cPtr == 0) ? null : new MessageQueue(cPtr, false);
  }

  public Mess set_payload(Object payload)
  {
    simgridJNI.Mess_set_payload(getCPtr(), this, payload);
    return this;
  }

  public Object get_payload() { return simgridJNI.Mess_get_payload(getCPtr(), this); }

  public Actor get_sender() { return simgridJNI.Mess_get_sender(getCPtr(), this); }
  public boolean is_assigned() { return simgridJNI.Mess_is_assigned(getCPtr(), this); }

  public Actor get_receiver() { return simgridJNI.Mess_get_receiver(getCPtr(), this); }

  public Mess add_successor(Activity a)
  {
    simgridJNI.Mess_add_successor(getCPtr(), this, Activity.getCPtr(a), a);
    return this;
  }

  public Mess remove_successor(Activity a)
  {
    simgridJNI.Mess_remove_successor(getCPtr(), this, Activity.getCPtr(a), a);
    return this;
  }

  public Mess set_name(String name)
  {
    simgridJNI.Mess_set_name(getCPtr(), this, name);
    return this;
  }

  public String get_name() { return simgridJNI.Mess_get_name(getCPtr(), this); }

  public Mess set_tracing_category(String category)
  {
    simgridJNI.Mess_set_tracing_category(getCPtr(), this, category);
    return this;
  }

  public String get_tracing_category() { return simgridJNI.Mess_get_tracing_category(getCPtr(), this); }

  public Mess cancel()
  {
    simgridJNI.Mess_cancel(getCPtr(), this);
    return this;
  }

  public Mess await_for(double timeout)
  {
    simgridJNI.Mess_await_for(getCPtr(), this, timeout);
    return this;
  }

  public Mess await()
  {
    await_for(-1);
    return this;
  }
}
