/* Copyright (c) 2006-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

/**
 * Used when an exception fails in the C++ world.
 */

public class AssertionError extends Error {
  private static final long serialVersionUID = 1L;
  public AssertionError(String s) { super(s); }
}
