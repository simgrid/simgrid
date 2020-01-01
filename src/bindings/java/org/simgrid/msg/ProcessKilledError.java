/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;

/** Used internally to interrupt the user code when the process gets killed.
 *
 * \rst
 * You can catch it for cleanups or to debug, but DO NOT BLOCK IT, or your simulation will segfault!
 *
 * .. code-block:: java
 *
 *    try {
 *      getHost().off();
 *    } catch (ProcessKilledError e) {
 *      e.printStackTrace();
 *      throw e;
 *    }
 *
 * \endrst
 */

public class ProcessKilledError extends Error {
	private static final long serialVersionUID = 1L;
	public ProcessKilledError(String s) {
		super(s);
	}
}
