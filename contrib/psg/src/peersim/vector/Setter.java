/*
 * Copyright (c) 2003-2005 The BISON Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

package peersim.vector;

import java.lang.reflect.*;
import peersim.config.*;
import peersim.core.*;

/**
 * Vectors can be written through this class. Typically {@link Control} classes
 * use this to manipulate vectors.
 * <p>
 * The method to be used is specified at construction time.
 * For backward compatibility, if no method is specified, the method
 * <code>setValue</code> is used. In this way, protocols
 * implementing the {@link SingleValue} interface can be manipulated using the
 * old configuration syntax (i.e., without specifying the method).
 * <p>
 * Please refer to package {@link peersim.vector} for a detailed description of 
 * the concept of protocol vector and the role of getters and setters. 
 */
public class Setter {

// ============================ fields ===================================
// =======================================================================

private final String protocol;
private final String methodn;
private final String prefix;

/** Identifier of the protocol that defines the vector */
private int pid;

/** Setter method name */
private String methodName;

/** Setter method */
private Method method=null;

/** Parameter type of setter method */
private Class type;


// ========================== initialization =============================
// =======================================================================


/**
 * Constructs a Setter class based on the configuration.
 * Note that the
 * actual initialization is delayed until the first access to the class,
 * so that if a class is not used, no unnecessary error messages and exceptions
 * are generated.
 * @param prefix the configuration prefix to use when reading the configuration
 * @param protocol the configuration parameter name that contains
 * the protocol we want to manipulate using a setter method.
 * The parameter <code>prefix + "." + protocol</code> is read.
 * @param methodn the configuration parameter name that contains the setter
 * method name.
 * The parameter <code>prefix + "." + methodn</code> is read, with the default
 * value <code>setValue</code>.
 */
public Setter(String prefix, String protocol, String methodn) {
	
	this.prefix=prefix;
	this.protocol=protocol;
	this.methodn=methodn;
}

// --------------------------------------------------------------------------

private void init() {

	if( method!=null) return;

	// Read configuration parameter
	pid = Configuration.getPid(prefix + "." + protocol);
	methodName = Configuration.getString(prefix+"."+methodn,"setValue");
	// Search the method
	Class clazz = Network.prototype.getProtocol(pid).getClass();
	try {
		method = GetterSetterFinder.getSetterMethod(clazz, methodName);
	} catch (NoSuchMethodException e) {
		throw new IllegalParameterException(prefix + "." +
		methodn, e+"");
	}
	// Obtain the type of the field
	type = GetterSetterFinder.getSetterType(method);
}


// =============================== methods =============================
// =====================================================================


/**
* @return type of parameter of setter method
*/
public Class getType() {

	init();
	return type;
}

// --------------------------------------------------------------------------

/**
* @return true if the setter type is long or int
*/
public boolean isInteger() {

	init();
	return type==long.class || type==int.class;
}

// --------------------------------------------------------------------------

/**
* Sets the given integer value.
* @param n The node to set the value on. The protocol is defined
* by {@link #pid}.
* @param val the value to set.
*/
public void set(Node n, long val) {
	
	init();
	
	try 
	{
		if(type==long.class)
		{
			method.invoke(n.getProtocol(pid),val);
			return;
		}
		if(type==int.class)
		{
			method.invoke(n.getProtocol(pid),(int)val);
			return;
		}
	}
	catch (Exception e)
	{
		throw new RuntimeException("While using setter "+methodName,e);
		
	}
	
	throw new RuntimeException("type has to be int or long");
}

// --------------------------------------------------------------------------

/**
* Sets the given real value.
* @param n The node to set the value on. The protocol is defined
* by {@link #pid}.
* @param val the value to set.
*/
public void set(Node n, double val) {
	
	init();
	
	try
	{
		if(type==double.class)
		{
			method.invoke(n.getProtocol(pid),val);
			return;
		}
		if(type==float.class)
		{
			method.invoke(n.getProtocol(pid),(float)val);
			return;
		}
	}
	catch (Exception e)
	{
		throw new RuntimeException("While using setter "+methodName,e);
	}
	
	throw new RuntimeException("type has to be double or float");
}

// --------------------------------------------------------------------------

/**
* Sets the given integer value.
* @param i The index of the node to set the value on in the network.
* The protocol is defined
* by {@link #pid}.
* @param val the value to set.
*/
public void set(int i, long val) { set(Network.get(i),val); }

// --------------------------------------------------------------------------

/**
* Sets the given real value.
* @param i The index of the node to set the value on in the network.
* The protocol is defined
* by {@link #pid}.
* @param val the value to set.
*/
public void set(int i, double val) { set(Network.get(i),val); }

}

