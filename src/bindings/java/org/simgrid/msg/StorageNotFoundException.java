/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;

/** Exception raised when looking for a non-existing storage. */
public class StorageNotFoundException extends MsgException {
	private static final long serialVersionUID = 1L;

	/** Constructs an <code>StorageNotFoundException</code> without a detail message. */
	public StorageNotFoundException() {
		super();
	}
	/** Constructs an <code>StorageNotFoundException</code> with a detail message. */
	public StorageNotFoundException(String s) {
		super(s);
	}
}
