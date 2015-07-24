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

import peersim.config.*;
import peersim.core.*;
import peersim.util.*;

/**
* Print statistics over a vector. The vector is defined by a protocol,
* specified by {@value #PAR_PROT}, that has to  implement
* {@link SingleValue}.
* Statistics printed are: min, max, number of samples, average, variance,
* number of minimal instances, number of maximal instances (using
* {@link IncrementalStats#toString}).
* @see IncrementalStats
*/
public class SingleValueObserver implements Control {


//--------------------------------------------------------------------------
//Parameters
//--------------------------------------------------------------------------

/** 
 *  The parameter used to determine the accuracy
 *  (standard deviation) before stopping the simulation. If not 
 *  defined, a negative value is used which makes sure the observer 
 *  does not stop the simulation.
 * @see #execute
 *  @config
 */
private static final String PAR_ACCURACY = "accuracy";

/**
 * The protocol to operate on.
 * @config
 */
private static final String PAR_PROT = "protocol";


//--------------------------------------------------------------------------
// Fields
//--------------------------------------------------------------------------

/** The name of this observer in the configuration */
private final String name;

/** Accuracy for standard deviation used to stop the simulation */
private final double accuracy;

/** Protocol identifier */
private final int pid;


//--------------------------------------------------------------------------
// Constructor
//--------------------------------------------------------------------------

/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param name the configuration prefix for this class
 */
public SingleValueObserver(String name)
{
	this.name = name;
	accuracy = Configuration.getDouble(name + "." + PAR_ACCURACY, -1);
	pid = Configuration.getPid(name + "." + PAR_PROT);
}


//--------------------------------------------------------------------------
// Methods
//--------------------------------------------------------------------------

/**
* Print statistics over a vector. The vector is defined by a protocol,
* specified by {@value #PAR_PROT}, that has to  implement
* {@link SingleValue}.
* Statistics printed are: min, max, number of samples, average, variance,
* number of minimal instances, number of maximal instances (using 
* {@link IncrementalStats#toString}).
* @return true if the standard deviation is below the value of
 * {@value #PAR_ACCURACY}, and the time of the simulation is larger then zero
 * (i.e. it has started).
 */
public boolean execute()
{
	IncrementalStats stats = new IncrementalStats();
	
	/* Compute max, min, average */
	for (int i = 0; i < Network.size(); i++)
	{
		SingleValue v = (SingleValue)Network.get(i).getProtocol(pid);
		stats.add( v.getValue() );
	}

	/* Printing statistics */
	System.out.println(name+": "+stats);

	/* Terminate if accuracy target is reached */
	return (stats.getStD()<=accuracy && CommonState.getTime()>0);
}

//--------------------------------------------------------------------------

}
