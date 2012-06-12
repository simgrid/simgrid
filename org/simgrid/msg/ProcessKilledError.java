package org.simgrid.msg;

/** This error class is only used to interrupt the java user code 
 * when the process gets killed by an external event.
 * Don't catch it.
 */

public class ProcessKilledError extends Error {
	private static final long serialVersionUID = 1L;
	public ProcessKilledError(String s) {
		super(s);
	}
}
