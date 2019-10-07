/* JNI interface to C code for the TRACES part of SimGrid. */

/* Copyright (c) 2012-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.trace;

import org.simgrid.NativeLib;

public final class Trace {
  /* Statically load the library which contains all native functions used in here */
  static {
    NativeLib.nativeInit();
  }

  private Trace() {
    throw new IllegalStateException("Utility class \"Trace\"");
  }

  // TODO complete the binding of the tracing API

  /**
   * Declare a new user variable associated to hosts with a color.
   *
   * @param variable
   * @param color
   */
  public static final native void hostVariableDeclareWithColor (String variable, String color);

  /**
   *  Add a value to a variable of a host.
   *
   * @param host
   * @param variable
   * @param value
   */
  public static final native void hostVariableAdd (String host, String variable, double value);

  /**
   * Subtract a value from a variable of a host.
   *
   * @param host
   * @param variable
   * @param value
   */
  public static final native void hostVariableSub (String host, String variable, double value);

  /**
   * Set the value of a variable of a host at a given timestamp.
   *
   * @param time
   * @param host
   * @param variable
   * @param value
   */
  public static final native void hostVariableSetWithTime (double time, String host, String variable, double value);

  /**
   *   Add a value to a variable of a host at a given timestamp.
   *
   * @param time
   * @param host
   * @param variable
   * @param value
   */
  public static final native void hostVariableAddWithTime (double time, String host, String variable, double value);

  /**
   * Subtract a value from a variable of a host at a given timestamp.
   *
   * @param time
   * @param host
   * @param variable
   * @param value
   */
  public static final native void hostVariableSubWithTime (double time, String host, String variable, double value);

  /**
   *  Get declared user host variables.
   *
   */
  public static final native String[]  getHostVariablesName ();

  /**
   *  Declare a new user variable associated to links.
   *
   * @param variable
   */
  public static final native void linkVariableDeclare (String variable);

  /**
   * Declare a new user variable associated to links with a color.
   * @param variable
   * @param color
   */
  public static final native void linkVariableDeclareWithColor (String variable, String color);

  /**
   *  Set the value of a variable of a link.
   *
   * @param link
   * @param variable
   * @param value
   */
  public static final native void linkVariableSet (String link, String variable, double value);

  /**
   * Add a value to a variable of a link.
   *
   * @param link
   * @param variable
   * @param value
   */
  public static final native void linkVariableAdd (String link, String variable, double value);

  /**
   * Subtract a value from a variable of a link.
   *
   * @param link
   * @param variable
   * @param value
   */
  public static final native void linkVariableSub (String link, String variable, double value);

  /**
   *  Set the value of a variable of a link at a given timestamp.
   *
   * @param time
   * @param link
   * @param variable
   * @param value
   */
  public static final native void linkVariableSetWithTime (double time, String link, String variable, double value);

  /**
   * Add a value to a variable of a link at a given timestamp.
   *
   * @param time
   * @param link
   * @param variable
   * @param value
   */
  public static final native void linkVariableAddWithTime (double time, String link, String variable, double value);

  /**
   * Subtract a value from a variable of a link at a given timestamp.
   *
   * @param time
   * @param link
   * @param variable
   * @param value
   */
  public static final native void linkVariableSubWithTime (double time, String link, String variable, double value);

  /**
   * Set the value of the variable present in the links connecting source and destination.
   *
   * @param src
   * @param dst
   * @param variable
   * @param value
   */
  public static final native void linkSrcDstVariableSet (String src, String dst, String variable, double value);

  /**
   * Add a value to the variable present in the links connecting source and destination.
   *
   * @param src
   * @param dst
   * @param variable
   * @param value
   */
  public static final native void linkSrcDstVariableAdd (String src, String dst, String variable, double value);

  /**
   * Subtract a value from the variable present in the links connecting source and destination.
   *
   * @param src
   * @param dst
   * @param variable
   * @param value
   */
  public static final native void linkSrcDstVariableSub (String src, String dst, String variable, double value);

  /**
   *  Set the value of the variable present in the links connecting source and destination at a given timestamp.
   *
   * @param time
   * @param src
   * @param dst
   * @param variable
   * @param value
   */
  public static final native void linkSrcDstVariableSetWithTime (double time, String src, String dst, String variable, double value);

  /**
   * Add a value to the variable present in the links connecting source and destination at a given timestamp.
   *
   * @param time
   * @param src
   * @param dst
   * @param variable
   * @param value
   */
  public static final native void linkSrcdstVariableAddWithTime (double time, String src, String dst, String variable, double value);

  /**
   * Subtract a value from the variable present in the links connecting source and destination at a given timestamp.
   *
   * @param time
   * @param src
   * @param dst
   * @param variable
   * @param value
   */
  public static final native void linkSrcDstVariableSubWithTime (double time, String src, String dst, String variable, double value);

  /**
   *  Get declared user link variables.
   */
  public static final native String[] getLinkVariablesName ();


  /* **** ******** WARNINGS ************** ***** */
  /* Only the following routines have been       */
  /* JNI implemented - Adrien May, 22nd          */
  /* **** ******************************** ***** */

  /**
   * Declare a user state that will be associated to hosts.
   * A user host state can be used to trace application states.
   *
   * @param name The name of the new state to be declared.
   */
  public static final native void hostStateDeclare(String name);

  /**
   * Declare a new value for a user state associated to hosts.
   * The color needs to be a string with three numbers separated by spaces in the range [0,1].
   * A light-gray color can be specified using "0.7 0.7 0.7" as color.
   *
   * @param state The name of the new state to be declared.
   * @param value The name of the value
   * @param color The color of the value
   */
  public static final native void hostStateDeclareValue (String state, String value, String color);

  /**
   *   Set the user state to the given value.
   *  (the queue is totally flushed and reinitialized with the given state).
   *
   * @param host The name of the host to be considered.
   * @param state The name of the state previously declared.
   * @param value The new value of the state.
   */
  public static final native void hostSetState (String host, String state, String value);

  /**
   * Push a new value for a state of a given host.
   *
   * @param host The name of the host to be considered.
   * @param state The name of the state previously declared.
   * @param value The value to be pushed.
   */
  public static final native void hostPushState (String host, String state, String value);

  /**
   *  Pop the last value of a state of a given host.
   *
   * @param host The name of the host to be considered.
   * @param state The name of the state to be popped.
   */
  public static final native void hostPopState (String host, String state);

  /**
   * Declare a new user variable associated to hosts.
   *
   * @param variable
   */
  public static final native void hostVariableDeclare (String variable);

  /**
   * Set the value of a variable of a host.
   *
   * @param host
   * @param variable
   * @param value
   */
  public static final native void hostVariableSet (String host, String variable, double value);

  /**
   * Declare a new user variable associated to VMs.
   *
   * @param variable
   */
  public static final native void vmVariableDeclare (String variable);

  /**
   * Set the value of a variable of a VM.
   *
   * @param vm
   * @param variable
   * @param value
   */
  public static final native void vmVariableSet (String vm, String variable, double value);
}
