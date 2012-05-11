/*
 * These are the upcalls used by the FleXML parser for application files
 *
 * Copyright 2006,2007,2010,2011 The SimGrid team.           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 * (GNU LGPL) which comes with this package.
 */

package org.simgrid.msg;
import java.util.Hashtable;
import java.util.Vector;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;

public final class ApplicationHandler {


		/**
		 * The vector which contains the arguments of the main function 
		 * of the process object.
		 */
		public  static Vector<String> args;

        /**
         *
         */
        public  static Hashtable<String,String> properties;

		/**
		 * The name of the host of the process.
		 */
		private  static String hostName;

		/**
		 * The function of the process.
		 */
		private  static String function;
		
		/**
		 * This method is called by the start element handler.
		 * It sets the host and the function of the process to create,
		 * and clear the vector containing the arguments of the 
		 * previouse process function if needed.
		 *
         * @param hostName_
         * @param function_
         * @host				The host of the process to create.
		 * @function			The function of the process to create.
		 *
		 */
		public  static void setProcessIdentity(String hostName_, String function_) {
			hostName = hostName_;    
			function = function_;

			if (!args.isEmpty())
				args.clear();

			if(!properties.isEmpty())
				properties.clear();
		}
		/**
		 * This method is called by the startElement() handler.
		 * It stores the argument of the function of the next
		 * process to create in the vector of arguments.
		 *
         * @param arg
         * @arg					The argument to add.
		 *
		 */ public  static void registerProcessArg(String arg) {
			 args.add(arg);
		 }

         /**
          *
          * @param id
          * @param value
          */
         public  static void setProperty(String id, String value)
		 {
			 properties.put(id,value);	
		 }

         /**
          *
          * @return
          */
         public  static String getHostName()
		 {
			 return hostName;
		 }

         /**
          * Method called to create the process
          */
         @SuppressWarnings("unchecked")
		 public  static void createProcess() {
			 try {
				 Class<Process> cls = (Class<Process>) Class.forName(function);
				 Constructor<Process> constructor = cls.getConstructor(new Class [] {Host.class, java.lang.String.class, java.lang.String[].class});
				 String[] args_ = args.toArray(new String[args.size()]);
				 Process process = constructor.newInstance(Host.getByName(hostName), function, args_);
				 process.start();
			 } 
			 catch (NoSuchMethodException e) {
				 throw new RuntimeException("Can't find the correct constructor for the class " + function + ". \n" +
				 "Is there a constructor with the signature: \"Host host, String name, String[]args\" in the class ?");
			 }
			 catch (InvocationTargetException e) {
				 e.printStackTrace();
			 }
			 catch(HostNotFoundException e) {
				 System.out.println(e.toString());
				 e.printStackTrace();

			 } catch(ClassNotFoundException e) {
				 System.out.println(function +
				 " class not found\n The attribut function of the element process  of your deployment file\n must correspond to the name of a Msg Proces class)");
				 e.printStackTrace();

			 } catch(InstantiationException e) {
				 System.out.println("Unable to create the process. I got an instantiation exception");
				 e.printStackTrace();
			 } catch(IllegalAccessException e) {
				 System.out.println("Unable to create the process. I got an illegal access exception");
				 e.printStackTrace();
			 } 

		 }
	

         /**
          *
          */
         public  static void onStartDocument() {
			args = new Vector<String>();
			properties = new Hashtable<String,String>();
			hostName = null;
			function = null;
	}

         /**
          *
          * @param hostName
          * @param function
          */
         public  static void onBeginProcess(String hostName, String function) {
		setProcessIdentity(hostName, function);
		
	}
    /**
     *
     * @param id
     * @param value
     */
    public  static void onProperty(String id, String value) {
		setProperty(id, value);
	}

    /**
     *
     * @param arg
     */
    public  static void onProcessArg(String arg) {
		registerProcessArg(arg);
	}

    /**
     *
     */
    public  static void onEndProcess() {
		createProcess();
	}        

    /**
     *
     */
    public static void onEndDocument() {
	}  
}
