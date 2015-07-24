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

package peersim.dynamics;

import peersim.graph.*;
import peersim.core.*;
import peersim.config.*;
import java.lang.reflect.*;
import java.util.ArrayList;

/**
 * Takes a {@link Linkable} protocol and adds connections using an arbitrary
 * method.
 * No connections are removed.
 * The connections are added by an arbitrary method that can be specified
 * in the configuration. 
 * The properties the method has to fulfill are the following
 <ul>
 <li>It MUST be static</li>
 <li>It MUST have a first argument that can be assigned from a class
 that implements {@link Graph}.</li>
 <li>It MAY contain zero or more arguments following the first one.</li>
 <li>All the arguments after the first one MUST be of primitive type int,
 long or double, except
 the last one, which MAY be of type that can be assigned from
 <code>java.util.Random</code></li>
 </ul>
 The arguments are initialized using the configuration as follows.
 <ul>
 <li>The first argument is the {@link Graph} that is passed to
 {@link #wire}.</li>
 <li>The arguments after the first one (indexed as 1,2,etc) are initialized
 from configuration parameters of the form {@value #PAR_ARG}i, where i is the
 index.
 <li>If the last argument can be assigned from 
 <code>java.util.Random</code> then it is initialized with
 {@link CommonState#r}, the central source of randomness for the
 simulator.</li>
 </ul>
 For example, the class {@link WireWS} can be emulated by configuring
 <pre>
 init.0 WireByMethod
 init.0.class GraphFactory
 init.0.method wireWS
 init.0.arg1 10
 init.0.arg2 0.1
 ...
 </pre>
 Note that the {@value #PAR_CLASS} parameter defaults to {@link GraphFactory},
 and {@value #PAR_METHOD} defaults to "wire".
 */
public class WireByMethod extends WireGraph {

//--------------------------------------------------------------------------
//Parameters
//--------------------------------------------------------------------------

/**
 * The prefix for the configuration properties that describe parameters.
 * @config
 */
private static final String PAR_ARG = "arg";

/**
* The class that has the method we want to use. Defaults to
* {@link GraphFactory}.
* @config
*/
private static final String PAR_CLASS = "class";

/**
* The name of the method for wiring the graph. Defaults to <code>wire</code>.
* @config
*/
private static final String PAR_METHOD = "method";

//--------------------------------------------------------------------------
//Fields
//--------------------------------------------------------------------------

private final Object[] args;

private final Method method;

//--------------------------------------------------------------------------
//Initialization
//--------------------------------------------------------------------------

/**
 * Loads configuration. It verifies the constraints defined
 * in {@link WireByMethod}.
 * @param prefix the configuration prefix for this class
 */
public WireByMethod(String prefix)
{
	super(prefix);
	
	// get the method
	try
	{
		final Class wire =
			Configuration.getClass(prefix + "." + PAR_CLASS,
		Class.forName("peersim.graph.GraphFactory"));
		method = WireByMethod.getMethod(
			wire,
			Configuration.getString(prefix+"."+PAR_METHOD,"wire"));
	}
	catch( Exception e )
	{
		throw new RuntimeException(e);
	}
	
	// set the constant args (those other than 0th)
	Class[] argt = method.getParameterTypes();
	args = new Object[argt.length];
	for(int i=1; i<args.length; ++i)
	{
		
		if( argt[i]==int.class )
			args[i]=Configuration.getInt(prefix+"."+PAR_ARG+i);
		else if( argt[i]==long.class )
			args[i]=Configuration.getLong(prefix+"."+PAR_ARG+i);
		else if( argt[i]==double.class )
			args[i]=Configuration.getDouble(prefix+"."+PAR_ARG+i);
		else if(i==args.length-1 && argt[i].isInstance(CommonState.r))
			args[i]=CommonState.r;
		else
		{
			// we should neve get here
			throw new RuntimeException("Unexpected error, please "+
			"report this problem to the peersim team");
		}
	}
}

//--------------------------------------------------------------------------

/**
 * Search a wiring method in the specified class.
 * @param cl
 *          the class where to find the method
 * @param methodName
 *          the method to be searched
 * @return the requested method, if it fully conforms to the definition of
 * the wiring methods.
 */
private static Method getMethod(Class cl, String methodName)
		throws NoSuchMethodException, ClassNotFoundException {
		
	// Search methods
	Method[] methods = cl.getMethods();
	ArrayList<Method> list = new ArrayList<Method>();
	for (Method m: methods) {
		if (m.getName().equals(methodName)) {
			list.add(m);
		}
	}
	
	if (list.size() == 0) {
		throw new NoSuchMethodException("No method "
		+ methodName + " in class " + cl.getSimpleName());
	} else if (list.size() > 1) {
		throw new NoSuchMethodException("Multiple methods called "
		+ methodName + " in class " + cl.getSimpleName());
	}
	
	// Found a single method with the right name; check if
	// it is a setter.
	final Class graphClass = Class.forName("peersim.graph.Graph");
	final Class randomClass = Class.forName("java.util.Random");
	Method method = list.get(0);
	Class[] pars = method.getParameterTypes();
	if( pars.length < 1 || !pars[0].isAssignableFrom(graphClass) )
		throw new NoSuchMethodException(method.getName() + " of class "
		+ cl.getSimpleName() + " is not a valid graph wiring method,"+
		" it has to have peersim.graph.Graph as first argument type");
	for(int i=1; i<pars.length; ++i)
		if( !( pars[i]==int.class || pars[i]==long.class ||
		pars[i]==double.class ||
		(i==pars.length-1 && pars[i].isAssignableFrom(randomClass)) ) )
			throw new NoSuchMethodException(method.getName() +
			" of class "+ cl.getSimpleName()
			+ " is not a valid graph wiring method");
			
	if(method.toString().indexOf("static")<0)
		throw new NoSuchMethodException(method.getName() +
		" of class "+ cl.getSimpleName()
		+ " is not a valid graph wiring method; it is not static");
	
	return method;
}


//--------------------------------------------------------------------------
//Methods
//--------------------------------------------------------------------------

/** Invokes the method passing g to it.*/
public void wire(Graph g) {

	args[0]=g;
	try { method.invoke(null,args); }
	catch( Exception e ) { throw new RuntimeException(e); }
}

}

