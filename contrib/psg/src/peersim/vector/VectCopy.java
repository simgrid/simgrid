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

import peersim.core.*;
import peersim.dynamics.*;

/**
 * Sets values in a protocol vector by copying the values of another 
 * protocol vector.
 * The source is defined by {@value #PAR_SOURCE},
 * and getter method {@value peersim.vector.VectControl#PAR_GETTER}.
 * <p>
 * This dynamics class can copy any primitive field in the source
 * protocol to any primitive field in the destination protocol,
 * provided that the former field is associated with a getter method,
 * while the latter is associated with a setter method.
 * @see VectControl
 * @see peersim.vector
 */
public class VectCopy extends VectControl implements  NodeInitializer {


//--------------------------------------------------------------------------
//Parameters
//--------------------------------------------------------------------------

/**
 * The identifier of the protocol to be copied.
 * The vector values are copied from this vector.
 * @config
 */
private static final String PAR_SOURCE = "source";


// --------------------------------------------------------------------------
// Variables
// --------------------------------------------------------------------------

/** Source getter */
private final Getter source;

//--------------------------------------------------------------------------
//Initialization
//--------------------------------------------------------------------------

/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param prefix the configuration prefix for this class
 */
public VectCopy(String prefix)
{
	super(prefix);
	source = new Getter(prefix,PAR_SOURCE,PAR_GETTER);
}

//--------------------------------------------------------------------------
//Method
//--------------------------------------------------------------------------

/**
 * Sets values in a protocol vector by copying the values of another 
 * protocol vector. The source is defined by {@value #PAR_SOURCE},
 * and getter method {@value peersim.vector.VectControl#PAR_GETTER}.
 * @return always false
 */
public boolean execute() {

	int size = Network.size();
	for (int i = 0; i < size; i++) {
		Number ret = source.get(i);
		if(setter.isInteger()) setter.set(i,ret.longValue());
		else setter.set(i,ret.doubleValue());
	}

	return false;
}

//--------------------------------------------------------------------------

/**
 * Sets the value by copying the value of another 
 * protocol. The source is  defined by {@value #PAR_SOURCE},
 * and getter method {@value peersim.vector.VectControl#PAR_GETTER}.
 * @param n the node to initialize
 */
public void initialize(Node n) {

	Number ret = source.get(n);
	if(setter.isInteger()) setter.set(n,ret.longValue());
	else setter.set(n,ret.doubleValue());
}

//--------------------------------------------------------------------------

}
