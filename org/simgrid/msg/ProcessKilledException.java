package org.simgrid.msg;

/** This exception class is only used to interrupt the java user code 
 * when the process gets killed by an external event */

public class ProcessKilledException extends RuntimeException {
	private static final long serialVersionUID = 1L;
	public ProcessKilledException(String s) {
		super(s);
	}
}
