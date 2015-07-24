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

import java.lang.reflect.*;
import peersim.config.*;
import peersim.core.*;
import java.util.ArrayList;

/**
 * This {@link Control} invokes a specified method on a protocol.
 * The method is defined by config parameter {@value #PAR_METHOD} and
 * the protocol is by {@value #PAR_PROT}. The method must not have any
 * parameters and must return void. If no protocol is specified, then the
 * method will be invoked on all protocol in the protocol stack that define
 * it.
 * <p>
 * Although the method cannot have any parameters, it can of course read
 * {@link CommonState}. It is guaranteed that the state is up-to-date,
 * inlcuding eg method {@link CommonState#getNode}.
 */
public class MethodInvoker implements Control, NodeInitializer {


// --------------------------------------------------------------------------
// Parameter names
// --------------------------------------------------------------------------

/**
 * The protocol to operate on.
 * If not defined, the given method will be invoked on all protocols that
 * define it. In all cases, at least one protocol has to define it.
 * @config
 */
private static final String PAR_PROT = "protocol";

/**
 * The method to be invoked. It must return void and should not have any
 * parameters specified.
 * @config
 */
private static final String PAR_METHOD = "method";


// --------------------------------------------------------------------------
// Fields
// --------------------------------------------------------------------------

/** Identifiers of the protocols to be initialized */
private final int pid[];

/** Method name */
private final String methodName;

/** Methods corresponding to the protocols in {@link #pid}. */
private final Method method[];


// --------------------------------------------------------------------------
// Initialization
// --------------------------------------------------------------------------

/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param prefix the configuration prefix for this class
 */
public MethodInvoker(String prefix) {

	methodName = Configuration.getString(prefix+"."+PAR_METHOD);
	if(!Configuration.contains(prefix+"."+PAR_PROT))
	{
		// find protocols that implement method
		ArrayList<Integer> pids = new ArrayList<Integer>();
		ArrayList<Method> methods = new ArrayList<Method>();
		for(int i=0; i<Network.prototype.protocolSize(); ++i)
		{
			Method m = null;
			try
			{
				m = MethodInvoker.getMethod(
			  	  Network.prototype.getProtocol(i).getClass(),
			  	  methodName );
			}
			catch(NoSuchMethodException e) {}
			
			if(m!=null)
			{
				pids.add(i);
				methods.add(m);
			}
		}

		if( pids.isEmpty() )
		{
			throw new IllegalParameterException(prefix + "." +
			PAR_METHOD,
			"No protocols found that implement 'void "+
			methodName+"()'");
		}

		pid=new int[pids.size()];
		int j=0;
		for(int i: pids) pid[j++]=i;
		method=methods.toArray(new Method[methods.size()]);
	}
	else
	{
		pid = new int[1];
		pid[0] = Configuration.getPid(prefix+"."+PAR_PROT);
		try
		{
			method = new Method[1];
			method[0]=MethodInvoker.getMethod(
			  Network.prototype.getProtocol(pid[0]).getClass(),
			  methodName );
		}
		catch (NoSuchMethodException e)
		{
			throw new IllegalParameterException(prefix + "." +
			PAR_METHOD, e+"");
		}
	}
}

// --------------------------------------------------------------------------
// Methods
// --------------------------------------------------------------------------

private static Method getMethod(Class cl, String methodName)
throws NoSuchMethodException {

	Method[] methods = cl.getMethods();
	ArrayList<Method> list = new ArrayList<Method>();
	for(Method m: methods)
	{
		if(m.getName().equals(methodName))
		{
			Class[] pars = m.getParameterTypes();
			Class ret = m.getReturnType();
			if( pars.length == 0 && ret==void.class )
				list.add(m);
		}
	}
	
	if(list.size() == 0)
	{
		throw new NoSuchMethodException("Method "
		+ methodName + " in class " + cl.getName());
	}
	
	return list.get(0);
}

//--------------------------------------------------------------------------

/** Invokes method on all the nodes. */
public boolean execute() {

	for(int i=0; i<Network.size(); ++i)
	{
		initialize(Network.get(i));
	}

	return false;
}

//--------------------------------------------------------------------------

/** Invokes method on given node. */
public void initialize(Node n) {
	
	try
	{
		for(int i=0; i<pid.length; ++i)
		{
			CommonState.setNode(n);
			CommonState.setPid(pid[i]);
			method[i].invoke(n.getProtocol(pid[i]));
		}
	}
	catch(Exception e)
	{
		e.printStackTrace();
		System.exit(1);
	}
}
}
