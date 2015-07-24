/*
 * Copyright (c) 2006 The BISON Project
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
import java.util.*;

import peersim.core.*;

/**
 * This class provides a generic implementation of the
 * <code>java.lang.Comparator<code> interface specialized
 * for {@link Node} objects. Nodes are compared based
 * on one of their protocols, on which a configurable
 * method is invoked. Both the protocol id and the
 * method are specified in the constructor.
 * <br>
 * This comparator can be used, for example, to sort
 * an array of nodes based on method <code>getValue</code>
 * associated to the protocol <code>pid</code>:
 * <PRE>
 * Comparator c = new VectorComparator(pid, "getValue");
 * Array.sort(Node[] array, c);
 * </PRE>
 * Note that differently from other classes in this package,
 * VectorComparator is declared programmatically in the code
 * and not in the configuration file. It is included in this
 * package because it shares the same philosophy of the other
 * classes.
 *
 * @author Alberto Montresor
 * @version $Revision: 1.1 $
 */
public class VectorComparator implements Comparator
{

//--------------------------------------------------------------------------
//Fields
//--------------------------------------------------------------------------

/** Protocol identifier of the protocol to be observed */
private final int pid;

/** The getter to be used to obtain comparable values */
private final Method method;

//--------------------------------------------------------------------------
//Initialization
//--------------------------------------------------------------------------

public VectorComparator(int pid, String methodName)
{
	this.pid = pid;
	Node n = Network.prototype;
	if (n == null) {
		throw new IllegalStateException("No prototype node can be used to search methods");
	}
	Object p = n.getProtocol(pid);
	Class c = p.getClass();
	try {
		method = GetterSetterFinder.getGetterMethod(c, methodName);
	} catch (NoSuchMethodException e) {
		throw new IllegalArgumentException(e.getMessage());
	}
}


public int compare(Object o1, Object o2)
{
	try {
	Comparable c1 = (Comparable) method.invoke(((Node) o1).getProtocol(pid));
	Comparable c2 = (Comparable) method.invoke(((Node) o2).getProtocol(pid));
	return c1.compareTo(c2);
	} catch (InvocationTargetException e) {
		throw new RuntimeException(e.getCause().getMessage());
	} catch (IllegalAccessException e) {
		throw new RuntimeException(e.getCause().getMessage());
	}
}

}
