/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

/** This exception is raised when an operation timeouts */
public class TimeoutException extends SimgridException {
  private static final long serialVersionUID = 1L;
  public TimeoutException() { super(); }
  public TimeoutException(String s) { super(s); }
}
