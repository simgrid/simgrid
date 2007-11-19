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
import org.xml.sax.*;
import org.xml.sax.helpers.*;

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
public final class ApplicationHandler extends DefaultHandler {

  /* 
   * This class is used to create the processes descibed in the deployment file.
   */
  class ProcessFactory {
                /**
		 * The vector which contains the arguments of the main function 
		 * of the process object.
		 */
    public Vector args;

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
      this.args = new Vector();
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
    }
                /**
		 * This method is called by the startElement() handler.
		 * It stores the argument of the function of the next
		 * process to create in the vector of arguments.
		 *
		 * @arg					The argument to add.
		 *
		 */ public void registerProcessArg(String arg) {
      this.args.add(arg);
    }

    public void createProcess() {
      try {

        Class cls = Class.forName(this.function);

        simgrid.msg.Process process = (simgrid.msg.Process) cls.newInstance();
        process.name = process.getName();       //this.function;
        process.id = simgrid.msg.Process.nextProcessId++;
        Host host = Host.getByName(this.hostName);

        MsgNative.processCreate(process, host);
        Vector args = processFactory.args;
        int size = args.size();

        for (int index = 0; index < size; index++)
          process.args.add(args.get(index));

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
  private ProcessFactory processFactory;

  public ApplicationHandler() {
    super();
  }
    /**
     * instanciates the process factory 
     */
  public void startDocument() {
    this.processFactory = new ProcessFactory();
  }

  public void characters(char[]caracteres, int debut, int longueur) {
  }                             // NOTHING TODO

   /**
    * element handlers
    */
  public void startElement(String nameSpace, String localName, String qName,
                           Attributes attr) {
    if (localName.equals("process"))
      onProcessIdentity(attr);
    else if (localName.equals("argument"))
      onProcessArg(attr);
  }

     /**
     * process attributs handler.
     */
  public void onProcessIdentity(Attributes attr) {
    processFactory.setProcessIdentity(attr.getValue(0), attr.getValue(1));
  }

        /**
     * process arguments handler.
     */
  public void onProcessArg(Attributes attr) {
    processFactory.registerProcessArg(attr.getValue(0));
  }

    /**
     * creates the process
     */
  public void endElement(String nameSpace, String localName, String qName) {
    if (localName.equals("process")) {
      processFactory.createProcess();
    }
  }

  public void endDocument() {
  }                             // NOTHING TODO
}
