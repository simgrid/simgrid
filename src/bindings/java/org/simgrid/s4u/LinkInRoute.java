/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public class LinkInRoute {
  private transient long swigCPtr;
  protected transient boolean swigCMemOwn;

  protected LinkInRoute(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(LinkInRoute obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected static long swigRelease(LinkInRoute obj) {
    long ptr = 0;
    if (obj != null) {
      if (!obj.swigCMemOwn)
        throw new RuntimeException("Cannot release ownership as memory is not owned");
      ptr = obj.swigCPtr;
      obj.swigCMemOwn = false;
      obj.delete();
    }
    return ptr;
  }

  @SuppressWarnings({"deprecation", "removal"})
  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        simgridJNI.delete_LinkInRoute(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  public LinkInRoute(Link link) {
    this(simgridJNI.new_LinkInRoute__SWIG_0(Link.getCPtr(link), link), true);
  }

  public LinkInRoute(Link link, LinkInRoute.Direction d) {
    this(simgridJNI.new_LinkInRoute__SWIG_1(Link.getCPtr(link), link, d.swigValue()), true);
  }

  public LinkInRoute.Direction get_direction() {
    return LinkInRoute.Direction.swigToEnum(simgridJNI.LinkInRoute_get_direction(swigCPtr, this));
  }

  public Link get_link() {
    long cPtr = simgridJNI.LinkInRoute_get_link(swigCPtr, this);
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }

  public final static class Direction {
    public final static LinkInRoute.Direction UP = new LinkInRoute.Direction("UP", simgridJNI.LinkInRoute_Direction_UP_get());
    public final static LinkInRoute.Direction DOWN = new LinkInRoute.Direction("DOWN", simgridJNI.LinkInRoute_Direction_DOWN_get());
    public final static LinkInRoute.Direction NONE = new LinkInRoute.Direction("NONE", simgridJNI.LinkInRoute_Direction_NONE_get());

    public final int swigValue() {
      return swigValue;
    }

    public String toString() {
      return swigName;
    }

    public static Direction swigToEnum(int swigValue) {
      if (swigValue < swigValues.length && swigValue >= 0 && swigValues[swigValue].swigValue == swigValue)
        return swigValues[swigValue];
      for (int i = 0; i < swigValues.length; i++)
        if (swigValues[i].swigValue == swigValue)
          return swigValues[i];
      throw new IllegalArgumentException("No enum " + Direction.class + " with value " + swigValue);
    }

    private Direction(String swigName) {
      this.swigName = swigName;
      this.swigValue = swigNext++;
    }

    private Direction(String swigName, int swigValue) {
      this.swigName = swigName;
      this.swigValue = swigValue;
      swigNext = swigValue+1;
    }

    private Direction(String swigName, Direction swigEnum) {
      this.swigName = swigName;
      this.swigValue = swigEnum.swigValue;
      swigNext = this.swigValue+1;
    }

    private static Direction[] swigValues = { UP, DOWN, NONE };
    private static int swigNext = 0;
    private final int swigValue;
    private final String swigName;
  }

}
