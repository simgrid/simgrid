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

import org.xml.sax.*;
import org.xml.sax.helpers.*;
import java.lang.reflect.*;

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
public final class ApplicationHandler extends DefaultHandler
{
	/* the current process. */
    public simgrid.msg.Process process;
    
    public ApplicationHandler() {
        super();
    }
    
    public void startDocument() {} // NOTHING TODO
    public void endDocument() {}   // NOTHING TODO
    
    public void characters(char[] chars, int beg, int lgr) {}   // NOTHING TODO
    
   /**
    * element handlers
    */
    public void startElement(String nameSpace, String localName,String qName,Attributes attr)  {
        if(localName.equals("process"))
            onStartProcess(attr);   
	else if(localName.equals("argument"))
            onArgument(attr);
    }
    public void endElement(String nameSpace, String localName,String qName)  {
        if(localName.equals("process"))
            onEndProcess();   
    }
    
    /**
     * process element handler.
     */
    public void onStartProcess(Attributes attr) {
        String hostName = attr.getValue(0);
        String className = attr.getValue(1);
         	
        try {
        	
	    Class cls = Class.forName(className);
      		
	    process = (simgrid.msg.Process)cls.newInstance();
	    process.name = className;
	    process.id = simgrid.msg.Process.nextProcessId++;
	    
	    Host host = Host.getByName(hostName);
      		
	    Msg.processCreate(process,host);
      		
      	} catch(Exception e) {
	    e.printStackTrace();
        } 
    }
    
    public void onEndProcess() {} // NOTHING TODO   
    
    /**
     * function arguments handler.
     */
    public void onArgument(Attributes attr) {
	//Msg.info("Add "+attr.getValue(0)+" as argument to "+process.msgName());
        process.addArg(attr.getValue(0));      
    }
    
   /**
    * end of element handler.
    */
    
}
