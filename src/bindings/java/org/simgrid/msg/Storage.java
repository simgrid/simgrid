/* Bindings to the MSG storage */

/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;

public class Storage {

	/**
	 * This attribute represents a bind between a java storage object and
	 * a native storage. Even if this attribute is public you must never
	 * access to it.
	 */ 
	private long bind;

	/** Storage name */
	protected String name;

	/** User data. */ 
	private Object data;
	protected Storage() {
		this.bind = 0;
		this.data = null;
	};

	@Override
	public String toString (){
		return this.name; 

	}

	/**
	 * This static method gets a storage instance associated with a native
	 * storage of your platform. This is the best way to get a java storage object.
	 *
	 * @param name		The name of the storage to get.
	 *
	 * @return		The storage object with the given name.
	 * @exception		StorageNotFoundException if the name of the storage is not valid.
	 * @exception		NativeException if the native version of this method failed.
	 */ 
	public native static Storage getByName(String name) 
			throws HostNotFoundException, NullPointerException, NativeException, StorageNotFoundException;

	/**
	 * This method returns the name of a storage.
	 * @return			The name of the storage.
	 *
	 */ 
	public String getName() {
		return name;
	}

	/**
	 * This method returns the size (in bytes) of a storage element.
	 *
	 * @return	The size (in bytes) of the storage element.
	 *
	 */ 
	public native long getSize();

	/**
	 * This method returns the free size (in bytes) of a storage element.
	 *
	 * @return	The free size (in bytes) of the storage element.
	 *
	 */ 
	public native long getFreeSize();

	/**
	 * This method returns the used size (in bytes) of a storage element.
	 *
	 * @return	The used size (in bytes) of the storage element.
	 *
	 */ 
	public native long getUsedSize();

	/**
	 * Returns the value of a given storage property. 
	 */
	public native String getProperty(String name);

	/**
	 * Change the value of a given storage property. 
	 */
	public native void setProperty(String name, String value);


	/** 
	 *
	 * Returns the host name the storage is attached to
	 *
	 * @return	the host name the storage is attached to
	 */
	public native String getHost();

	/**
	 * This static method returns all of the storages of the installed platform.
	 *
	 * @return			An array containing all the storages installed.
	 *
	 */ 
	public native static Storage[] all();

	/**
	 * Class initializer, to initialize various JNI stuff
	 */
	public static native void nativeInit();
	static {
		nativeInit();
	}		

}
