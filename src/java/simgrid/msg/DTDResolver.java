/*
 * simgrid.msg.DTDResolver.java    1.00 07/05/01
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 */  
  package simgrid.msg;
import java.io.InputStream;
import org.xml.sax.EntityResolver;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
public class DTDResolver implements EntityResolver {
  public InputSource resolveEntity(String publicID, String systemID) 
    throws SAXException {
    if (!systemID.endsWith("surfxml.dtd")) {
      System.out.
        println("\n MSG - Warning - the platform used seams invalid\n");
      return null;
    }
    
      /* try to get the DTD from the classpath */ 
      InputStream in = getClass().getResourceAsStream("/surfxml.dtd");
    if (null == in)
      
        /* try to get the DTD from the surf dir in the jar */ 
        in = getClass().getResourceAsStream("/surf/surfxml.dtd");
    if (null == in)
      
        /* try to get the DTD from the directory Simgrid */ 
        in = getClass().getResourceAsStream("/Simgrid/surfxml.dtd");
    if (null == in)
      
        /* try to get the DTD from the directory Simgrid/msg */ 
        in = getClass().getResourceAsStream("/Simgrid/msg/surfxml.dtd");
    if (null == in) {
      System.err.println("\nMSG - XML DTD not found (" +
                          systemID.toString() +
                          ").\n\nPlease put this file in one of the following destinations :\n\n"
                          + "   - classpath;\n" +
                          "   - the directory Simgrid;\n" +
                          "   - the directory Simgrid/msg;\n" +
                          "   - the directory of you simulation.\n\n" +
                          "Once the DTD puted in one of the previouse destinations, retry you simulation.\n");
      
        /* 
         * If not founded, returning null makes process continue normally (try to get 
         * the DTD from the current directory 
         */ 
        return null;
    }
    return new InputSource(in);
  }
}


