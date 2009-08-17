/*
 * simgrid.msg.Msg.java    1.00 07/05/01
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 */

package simgrid.msg;
import org.xml.sax.*;

/** 
 * MSG was the first distributed programming environment 
 * provided within SimGrid. While almost realistic, it 
 * remains quite simple.
 * This class contains all the declaration of the natives methods
 * of the MSG API.
 * All of these methods are statics. You can't directly use these methods
 * but it is recommanded to use the classes Process, Host and Task
 * of the package to build your application.
 *
 * @author  Abdelmalek Cherier
 * @author  Martin Quinson
 * @since SimGrid 3.3
 */
public final class Msg {
  /*
   * Statically load the library which contains all native functions used in here
   */
  static {
    try {
      System.loadLibrary("simgrid");
    } catch(UnsatisfiedLinkError e) {
      System.err.println("Cannot load simgrid library : ");
      e.printStackTrace();
      System.err.println("Please check your LD_LIBRARY_PATH, " +
                         "or copy the library to the current directory");
      System.exit(1);
    }
  }

    /** Returns the last error code of the simulation */
  public final static native int getErrCode();

    /** Everything is right. Keep on goin the way ! */
  public static final int SUCCESS = 0;

    /** Something must be not perfectly clean (but I may be paranoid freak...) */
  public static final int WARNING = 1;

    /** There has been a problem during you task treansfer.
     *  Either the network is  down or the remote host has been shutdown */
  public static final int TRANSFERT_FAILURE = 2;

    /** System shutdown. 
     *  The host on which you are running has just been rebooted.
     *  Free your datastructures and return now ! */
  public static final int HOST_FAILURE = 3;

    /** Cancelled task. This task has been cancelled by somebody ! */
  public static final int TASK_CANCELLLED = 4;

    /** You've done something wrong. You'd better look at it... */
  public static final int FATAL_ERROR = 5;



   /** Retrieve the simulation time */
  public final static native double getClock();

  public final static native void pajeOutput(String pajeFile);


   /** Issue an information logging message */
  public final static native void info(String s);

    /*********************************************************************************
     * Deployment and initialization related functions                               *
     *********************************************************************************/

    /**
     * The natively implemented method to initialize a MSG simulation.
     *
     * @param args            The arguments of the command line of the simulation.
     *
     * @see                    Msg.init()
     */
  public final static native void init(String[]args);

    /**
     * Run the MSG simulation, and cleanup everything afterward.
     *
     * If you want to chain simulations in the same process, you
     * should call again createEnvironment and deployApplication afterward.
     *
     * @see                    MSG_run, MSG_clean
     */
  public final static native void run() throws NativeException;

    /**
     * The native implemented method to create the environment of the simulation.
     *
     * @param platformFile    The XML file which contains the description of the environment of the simulation
     *
     */
  public final static native void createEnvironment(String platformFile)
  throws NativeException;
  
   /**
     * The method to deploy the simulation.
     *
     * @param platformFile    The XML file which contains the description of the application to deploy.
     */
  public final static native void deployApplication(String deploymentFile)
  throws NativeException;
 
  /* The launcher */
  static public void main(String[]args) throws MsgException {
    /* initialize the MSG simulation. Must be done before anything else (even logging). */
    Msg.init(args);

    if (args.length < 2) {
      Msg.info("Usage: Msg platform_file deployment_file");
      System.exit(1);
    }

    /* Load the platform and deploy the application */
    Msg.createEnvironment(args[0]);
    Msg.deployApplication(args[1]);

    /* Execute the simulation */
    Msg.run();
  }
}
