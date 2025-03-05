/* Copyright (c) 2006-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

/**
 * Used internally to interrupt the user code when the actor gets killed.
 *
 * \beginrst
 * You can catch it for cleanups or to debug, but DO NOT BLOCK IT, or your simulation will segfault!
 *
 * .. code-block:: java
 *
 *    try {
 *      getHost().off();
 *    } catch (ForcefulKillException e) {
 *      e.printStackTrace();
 *      throw e;
 *    }
 *
 * \endrst
 */

public class ForcefulKillException extends Error {
  private static final long serialVersionUID = 1L;
  public ForcefulKillException(String s) { super(s); }
}
