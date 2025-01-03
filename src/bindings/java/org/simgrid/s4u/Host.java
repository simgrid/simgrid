/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (https://www.swig.org).
 * Version 4.2.1
 *
 * Do not make changes to this file unless you know what you are doing - modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.simgrid.s4u;

import java.util.Vector;

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

  public static Host by_name(String name)
  {
    long cPtr = simgridJNI.Host_by_name(name);
    return (cPtr == 0) ? null : new Host(cPtr);
  }
  public static Host current()
  {
    long cPtr = simgridJNI.Host_current();
    return (cPtr == 0) ? null : new Host(cPtr);
  }
  public Disk[] get_disks() { return simgridJNI.Host_get_disks(swigCPtr, this); }
}
