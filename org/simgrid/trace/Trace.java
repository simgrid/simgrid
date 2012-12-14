/*
 * JNI interface to C code for the TRACES part of SimGrid.
 * 
 * Copyright 2012 The SimGrid Team.           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 * (GNU LGPL) which comes with this package.
 */
package org.simgrid.trace;

import org.simgrid.msg.Msg;

public final class Trace {
	/* Statically load the library which contains all native functions used in here */
	static {
		Msg.nativeInit();
		try {
			System.loadLibrary("SG_java_tracing");
		} catch(UnsatisfiedLinkError e) {
			System.err.println("Cannot load the bindings to the simgrid library: ");
			e.printStackTrace();
			System.err.println(
					"Please check your LD_LIBRARY_PATH, or copy the simgrid and SG_java_tracing libraries to the current directory");
			System.exit(1);
		}
	}

	// TODO complete the binding of the tracing API 
	
	/**
	 * Declare a new user variable associated to hosts. 
	 * 
	 * @param variable
	 */
	public final static native	void hostVariableDeclare (String variable);
 
	/**
	 * Declare a new user variable associated to hosts with a color. 
	 *  
	 * @param variable
	 * @param color
	 */
	public final static native	void hostVariableDeclareWithColor (String variable, String color);
	
	/**
	 * Set the value of a variable of a host. 
	 * 
	 * @param host
	 * @param variable
	 * @param value
	 */
	public final static native	void hostVariableSet (String host, String variable, double value);
 
	/**
	 *  Add a value to a variable of a host. 
	 *  
	 * @param host
	 * @param variable
	 * @param value
	 */
	public final static native	void hostVariableAdd (String host, String variable, double value);

	/**
	 * Subtract a value from a variable of a host. 
	 *  
	 * @param host
	 * @param variable
	 * @param value
	 */
	public final static native	void hostVariableSub (String host, String variable, double value);

	/**
 	 * Set the value of a variable of a host at a given timestamp. 
 	 * 
	 * @param time
	 * @param host
	 * @param variable
	 * @param value
	 */
	public final static native	void hostVariableSetWithTime (double time, String host, String variable, double value);

	/**
	 * 	Add a value to a variable of a host at a given timestamp. 
	 * 
	 * @param time
	 * @param host
	 * @param variable
	 * @param value
	 */
 	public final static native	void hostVariableAddWithTime (double time, String host, String variable, double value);
 
 	/**
 	 * Subtract a value from a variable of a host at a given timestamp.  
 	 * 
 	 * @param time
 	 * @param host
 	 * @param variable
 	 * @param value
 	 */
 	public final static native	void hostVariableSubWithTime (double time, String host, String variable, double value);

 	/**
 	 *  Get declared user host variables. 
 	 * 
 	 */
 	public final static native String[]  getHostVariablesName ();

 	/**
 	 *  Declare a new user variable associated to links. 
 	 *  
 	 * @param variable
 	 */
 	public final static native	void linkVariableDeclare (String variable);

 	/**
 	 * Declare a new user variable associated to links with a color. 
 	 * @param variable
 	 * @param color
 	 */
 	public final static native	void linkVariableDeclareWithColor (String variable, String color);

 	/**
 	 *  Set the value of a variable of a link. 
 	 *   
 	 * @param link
 	 * @param variable
 	 * @param value
 	 */
 	public final static native	void linkVariableSet (String link, String variable, double value);

 	/**
 	 * Add a value to a variable of a link. 
 	 * 
 	 * @param link
 	 * @param variable
 	 * @param value
 	 */
 	public final static native	void linkVariableAdd (String link, String variable, double value);
 
 	/**
 	 * Subtract a value from a variable of a link. 
 	 * 
 	 * @param link
 	 * @param variable
 	 * @param value
 	 */
 	public final static native	void linkVariableSub (String link, String variable, double value);

 	/**
 	 *  Set the value of a variable of a link at a given timestamp. 
 	 *  
 	 * @param time
 	 * @param link
 	 * @param variable
 	 * @param value
 	 */
 	public final static native	void linkVariableSetWithTime (double time, String link, String variable, double value);

 	/**
 	 * Add a value to a variable of a link at a given timestamp.
 	 * 
	 * @param time
 	 * @param link
 	 * @param variable
 	 * @param value
 	 */
 	public final static native	void linkVariableAddWithTime (double time, String link, String variable, double value);
 

 	/**
 	 * Subtract a value from a variable of a link at a given timestamp. 
 	 *   
 	 * @param time
 	 * @param link
 	 * @param variable
 	 * @param value
 	 */
 	public final static native	void linkVariableSubWithTime (double time, String link, String variable, double value);

 	/**
 	 * Set the value of the variable present in the links connecting source and destination. 
 	 * 
 	 * @param src
 	 * @param dst
 	 * @param variable
 	 * @param value
 	 */
 	public final static native	void linkSrcDstVariableSet (String src, String dst, String variable, double value);
 
 	/**
 	 * Add a value to the variable present in the links connecting source and destination. 
 	 *  
 	 * @param src
 	 * @param dst
 	 * @param variable
 	 * @param value
 	 */
 	public final static native	void linkSrcDstVariableAdd (String src, String dst, String variable, double value);

 	/**
 	 * Subtract a value from the variable present in the links connecting source and destination. 
 	 *   
 	 * @param src
 	 * @param dst
 	 * @param variable
 	 * @param value
 	 */
 	public final static native	void linkSrcDstVariableSub (String src, String dst, String variable, double value);

 	/**
 	 *  Set the value of the variable present in the links connecting source and destination at a given timestamp. 
 	 *   
 	 * @param time
 	 * @param src
 	 * @param dst
 	 * @param variable
 	 * @param value
 	 */
 	public final static native	void linkSrcDstVariableSetWithTime (double time, String src, String dst, String variable, double value);

 	/**
 	 * Add a value to the variable present in the links connecting source and destination at a given timestamp. 
 	 * 
 	 * @param time
 	 * @param src
 	 * @param dst
 	 * @param variable
 	 * @param value
 	 */
 	public final static native	void linkSrcdstVariableAddWithTime (double time, String src, String dst, String variable, double value);
 
 	/**
 	 * Subtract a value from the variable present in the links connecting source and destination at a given timestamp. 
 	 *  
 	 * @param time
 	 * @param src
 	 * @param dst
 	 * @param variable
 	 * @param value
 	 */
 	public final static native	void linkSrcDstVariableSubWithTime (double time, String src, String dst, String variable, double value);

 	/**
 	 *  Get declared user link variables.  
 	 */
	public final static native String[] getLinkVariablesName ();

 	
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
	public final static native void hostStateDeclare(String name);
	
	/**
	 * Declare a new value for a user state associated to hosts.
	 * The color needs to be a string with three numbers separated by spaces in the range [0,1]. 
	 * A light-gray color can be specified using "0.7 0.7 0.7" as color. 
	 * 
	 * @param state The name of the new state to be declared.
	 * @param value The name of the value
	 * @param color The color of the value
	 */
	public final static native void hostStateDeclareValue (String state, String value, String color);

	/**
	 * 	Set the user state to the given value.
	 *  (the queue is totally flushed and reinitialized with the given state).
	 *  
	 * @param host The name of the host to be considered.
	 * @param state The name of the state previously declared.
	 * @param value The new value of the state.
	 */
 	public final static native void hostSetState (String host, String state, String value);
 
 	/**
 	 * Push a new value for a state of a given host. 
 	 * 
 	 * @param host The name of the host to be considered.
 	 * @param state The name of the state previously declared.
 	 * @param value The value to be pushed.
 	 */
 	public final static native void hostPushState (String host, String state, String value);
 	
 	/**
 	 *  Pop the last value of a state of a given host. 
 	 *   
 	 * @param host The name of the host to be considered.
 	 * @param state The name of the state to be popped.
 	 */
 	public final static native void hostPopState (String host, String state);

	
}
