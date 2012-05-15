package org.simgrid.msg;

/** This exception class is only used to interrupt the java user code 
 * when the process gets killed by an external event */

public class ProcessKilledError extends Error {
	private static final long serialVersionUID = 1L;
	public ProcessKilledError(String s) {
		super(s);
	}
}
