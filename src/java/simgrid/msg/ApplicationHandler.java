/*
 * simgrid.msg.ApplicationHandler.java	1.00 07/05/01
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 */

package simgrid.msg;

import java.util.Vector;
import java.util.Hashtable;

/**
 * The handler used to parse the deployment file which contains 
 * the description of the application (simulation).
 *
 * @author  Abdelmalek Cherier
 * @author  Martin Quinson
 * @version 1.00, 07/05/01
 * @see Host
 * @see Process
 * @see Simulation
 * @since SimGrid 3.3
 * @since JDK1.5011 
 */
public final class ApplicationHandler {

	/* 
	 * This class is used to create the processes descibed in the deployment file.
	 */
	static class ProcessFactory {
		/**
		 * The vector which contains the arguments of the main function 
		 * of the process object.
		 */
		public Vector<String> args;

		public Hashtable<String,String> properties;

		/**
		 * The name of the host of the process.
		 */
		private String hostName;

		/**
		 * The function of the process.
		 */
		private String function;


		/**
		 * Default constructor.
		 */
		public ProcessFactory() {
			this.args = new Vector<String>();
			this.properties = new Hashtable<String,String>();
			this.hostName = null;
			this.function = null;
		}
		/**
		 * This method is called by the start element handler.
		 * It sets the host and the function of the process to create,
		 * and clear the vector containing the arguments of the 
		 * previouse process function if needed.
		 *
		 * @host				The host of the process to create.
		 * @function			The function of the process to create.
		 *
		 */
		public void setProcessIdentity(String hostName, String function) {
			this.hostName = hostName;
			this.function = function;

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
		 * @arg					The argument to add.
		 *
		 */ public void registerProcessArg(String arg) {
			 args.add(arg);
		 }

		 public void setProperty(String id, String value)
		 {
			 properties.put(id,value);	
		 }

		 public String getHostName()
		 {
			 return hostName;
		 }

		 @SuppressWarnings("unchecked")
		 public void createProcess() {
			 try {
				 Class<simgrid.msg.Process> cls = (Class<Process>) Class.forName(this.function);

				 simgrid.msg.Process process = cls.newInstance();
				 process.name = this.function;
				 process.id = simgrid.msg.Process.nextProcessId++;
				 Host host = Host.getByName(this.hostName);

				 MsgNative.processCreate(process, host);
				 Vector args = processFactory.args;
				 int size = args.size();

				 for (int index = 0; index < size; index++)
					 process.args.add(args.get(index));

				 process.properties = this.properties;
				 this.properties = new Hashtable();

			 } catch(JniException e) {
				 System.out.println(e.toString());
				 e.printStackTrace();

			 } catch(NativeException e) {
				 System.out.println(e.toString());
				 e.printStackTrace();

			 } catch(HostNotFoundException e) {
				 System.out.println(e.toString());
				 e.printStackTrace();

			 } catch(ClassNotFoundException e) {
				 System.out.println(this.function +
				 " class not found\n The attribut function of the element process  of your deployment file\n must correspond to the name of a Msg Proces class)");
				 e.printStackTrace();

			 } catch(InstantiationException e) {
				 System.out.println("instantiation exception");
				 e.printStackTrace();
			 } catch(IllegalAccessException e) {
				 System.out.println("illegal access exception");
				 e.printStackTrace();
			 } catch(IllegalArgumentException e) {
				 System.out.println("illegal argument exception");
				 e.printStackTrace();
			 }

		 }
	}

	/* 
	 * the ProcessFactory object used to create the processes.
	 */
	public static ProcessFactory processFactory;

	/**
	 * instanciates the process factory 
	 */
	public static void onStartDocument() {
		processFactory = new ProcessFactory();
	}

	public static void onBeginProcess(String hostName, String function) {
		processFactory.setProcessIdentity(hostName, function);
	}
	public static void onProperty(String id, String value) {
		processFactory.setProperty(id, value);
	}

	public static void onProcessArg(String arg) {
		processFactory.registerProcessArg(arg);
	}

	public static void onEndProcess() {
		processFactory.createProcess();
	}        

	public static void onEndDocument() {
	}  
}
