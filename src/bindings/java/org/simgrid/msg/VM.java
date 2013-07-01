/*
 * JNI interface to virtual machine in Simgrid
 * 
 * Copyright 2006-2012 The SimGrid Team.           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 * (GNU LGPL) which comes with this package.
 */
package org.simgrid.msg;

import org.simgrid.msg.Host;
import org.simgrid.msg.Process;

public class VM extends Host{
	// Please note that we are not declaring a new bind variable 
	//(the bind variable has been inherited from the super class Host)
	
	/* Static functions */ 
	// GetByName is inherited from the super class Host
	

	 private static VM[] vms=null; 	  
    private Host currentHost; 
    
	/* Constructors / destructors */
    /**
	 * Create a `basic' VM (i.e. 1 core, 1GB of RAM, other values are not taken into account).
	 */
	public VM(Host host, String name) {
		this(host,name,1,1024, -1, null, -1,0 , 0);
	}

	/**
	 * Create a  VM
	 * @param host, Host node
	 * @param name, name of the machine
	 * @param nCore, number of core
	 * @param ramSize, size of the RAM that should be allocated (in MBytes) 
	 * @param netCap (not used for the moment)
	 * @param diskPath (not used for the moment)
	 * @param diskSize (not used for the moment)
	 * @param dpRate (dirty page rate MB/flop, if you don't know put zero ;))
	 * @param migNetSpeed (network bandwith allocated for migrations in MB/s, if you don't know put zero ;))
	 */
	
	public VM(Host host, String name, int nCore,  long ramSize, 
			long netCap, String diskPath, long diskSize, long  dpRate,long migNetSpeed){
		super();
		super.name = name; 
		this.currentHost = host; 
		create(host, name, nCore, ramSize, netCap, diskPath, diskSize, dpRate, migNetSpeed);
		VM.addVM(this);
	}

	private static void addVM(VM vm){
		VM[] vmsN=null; 
	  	int i=0;
		if(VM.vms == null)
			vmsN = new VM[1]; 
		else
			vmsN = new VM[vms.length+1]; 
		
		for (i=0; i<vmsN.length-1 ; i ++){
			vmsN[i]=vms[i];	
		} 
		vmsN[i]=vm;
		vms=vmsN;
	}
   public static VM[] all(){
		return vms;
	}
	public static VM getVMByName(String name){
		for (int i=0 ; i < vms.length ; i++){
			  if (vms[i].getName().equals(name))
					return vms[i];		
		}
		return null; 
	}
	protected void finalize() {
		destroy();
	}
	

	/* JNI / Native code */
	/* get/set property methods are inherited from the Host class. */
	
	/** Returns whether the given VM is currently suspended
	 */	
	public native int isCreated();
	
	/** Returns whether the given VM is currently running
	 */
	public native int isRunning();

	/** Returns whether the given VM is currently running
	 */
	public native int isMigrating();
	
	/** Returns whether the given VM is currently suspended
	 */	
	public native int isSuspended();
		
	/** Returns whether the given VM is currently saving
	 */
	public native int isSaving();
	
	/** Returns whether the given VM is currently saved
	 */
	public native int isSaved();

	/** Returns whether the given VM is currently restoring its state
	 */
	public native boolean isRestoring();
	
	/**
	 * Natively implemented method create the VM.
	 * @param nCore, number of core
	 * @param ramSize, size of the RAM that should be allocated (in MB) 
	 * @param netCap (not used for the moment)
	 * @param diskPath (not used for the moment)
	 * @param diskSize (not used for the moment)
	 * @param dpRate (dirty page rate in MB/flop, if you don't know put zero ;))
	 * @param migNetSpeed (network bandwith allocated for migrations in MB/s, if you don't know put zero ;))
	 */
	private native void create(Host host, String name, int nCore, long ramSize, 
			 long netCap, String diskPath, long diskSize, long dpRate, long migNetSpeed);
	
	/**
	 * start the VM
	 */
	public native void start();

	
	/**
	 * Immediately kills all processes within the given VM. Any memory that they allocated will be leaked.
	 * No extra delay occurs. If you want to simulate this too, you want to use a MSG_process_sleep() or something
	 */
	public native void shutdown();
	
	/**  
	 * Invoke native migration routine
	*/
	public native void internalmig(Host destination);

	
	/** Change the host on which all processes are running
	 * (pre-copy is implemented)
	 */	
	public void migrate(Host destination){
//		String[] argsRx = new String[5];
//		argsRx[1] = this.getName();
//		argsRx[2] = this.currentHost.getName();
//		argsRx[3] = destination.getName();
//		argsRx[0] =  "__pr_mig_rx:"+argsRx[1]+"("+argsRx[2]+"-"+argsRx[3]+")";
//		argsRx[4] = null; // TODO: Why ? 
//		
//		//Process rx = new Process(destination, argsRx[0], argsRx );
//		
//		String[] argsTx = new String[5];
//		argsTx[1] = this.getName();
//		argsTx[2] = this.currentHost.getName();
//		argsTx[3] = destination.getName();
//		argsTx[0] =  "__pr_mig_tx:"+argsTx[1]+"("+argsTx[2]+"-"+argsTx[3]+")";
//		argsTx[4] = null; // TODO: Why ? 
//		
//		//Process tx = new Process(this.currentHost, argsTx[0], argsRx ); 
//		
		this.internalmig(destination);
	}
	
	/** Immediately suspend the execution of all processes within the given VM
	 *
	 * No suspension cost occurs. If you want to simulate this too, you want to
	 * use a \ref File.write() before or after, depending on the exact semantic
	 * of VM suspend to you.
	 */	
	public native void suspend();
	
	/** Immediately resumes the execution of all processes within the given VM
	 *
	 * No resume cost occurs. If you want to simulate this too, you want to
	 * use a \ref File.read() before or after, depending on the exact semantic
	 * of VM resume to you.
	 */
	public native void resume();
	
	/** Immediately suspend the execution of all processes within the given VM 
	 *  and save its state on the persistent HDD
	 *  Not yet implemented (for the moment it behaves like suspend)
	 *  No suspension cost occurs. If you want to simulate this too, you want to
	 *  use a \ref File.write() before or after, depending on the exact semantic
	 *  of VM suspend to you.
	 */	
	public native void save();
	
	/** Immediately resumes the execution of all processes previously saved 
	 * within the given VM
	 *  Not yet implemented (for the moment it behaves like resume)
	 *
	 * No resume cost occurs. If you want to simulate this too, you want to
	 * use a \ref File.read() before or after, depending on the exact semantic
	 * of VM resume to you.
	 */
	public native void restore();
	

	/**
	 * Destroy the VM
	 */
	public native void destroy();

	

	/**
	 * Class initializer, to initialize various JNI stuff
	 */
	public static native void nativeInit();
	static {
		nativeInit();
	}
}
