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
import peersim.util.*;

/**
 * This class computes and reports statistics information about a vector.
 * Provided statistics include average, max, min, variance,
 * etc. Values are printed according to the string format of {@link 
 * IncrementalStats#toString}.
 * @see VectControl
 * @see peersim.vector
 */
public class VectorObserver extends VectControl {


/** The name of this observer in the configuration */
private final String prefix;


//--------------------------------------------------------------------------
//Initialization
//--------------------------------------------------------------------------

/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param prefix the configuration prefix for this class
 */
public VectorObserver(String prefix) {

	super(prefix);
	this.prefix = prefix;
}

//--------------------------------------------------------------------------
// Methods
//--------------------------------------------------------------------------

/**
 * Prints statistics information about a vector.
 * Provided statistics include average, max, min, variance,
 * etc. Values are printed according to the string format of {@link 
 * IncrementalStats#toString}.
 * @return always false
 */
public boolean execute() {

	IncrementalStats stats = new IncrementalStats();

	for (int j = 0; j < Network.size(); j++)
	{
		Number v = getter.get(j);
		stats.add( v.doubleValue() );
	}
	
	System.out.println(prefix+": "+stats);	

	return false;
}

}
