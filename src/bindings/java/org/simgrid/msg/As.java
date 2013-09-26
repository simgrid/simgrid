/*
 * Bindings to the MSG hosts
 *
 * Copyright (c) 2006-2013. The SimGrid Team.
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */  
package org.simgrid.msg;

public class As {

    private long bind;
    
    protected As() {
	};

    public String toString (){
	return this.getName(); 
    }
    public native String getName();

    public native As[] getSons();

    public native String getProperty(String name);

    public native String getModel();

    public native Host[] getHosts();

    /**
      * Class initializer, to initialize various JNI stuff
      */
    public static native void nativeInit();
    static {
	nativeInit();
    }
}
