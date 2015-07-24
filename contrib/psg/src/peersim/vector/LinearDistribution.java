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

/**
 * Initializes a protocol vector with values in the range [{@value #PAR_MIN}, 
 * {@value #PAR_MAX}] (inclusive both ends), linearly increasing.
 * @see VectControl
 * @see peersim.vector
 */
public class LinearDistribution extends VectControl
{

//--------------------------------------------------------------------------
//Parameters
//--------------------------------------------------------------------------

/**
 * Upper end of the interval..
 * @config
 */
private static final String PAR_MAX = "max";

/**
 * Lower end of the interval. Defaults to -{@value #PAR_MAX}.
 * @config
 */
private static final String PAR_MIN = "min";

// --------------------------------------------------------------------------
// Fields
// --------------------------------------------------------------------------

/** Minimum value */
private final Number min;

/** Maximum value */
private final Number max;

/** Step value */
private final double step;

// --------------------------------------------------------------------------
// Initialization
// --------------------------------------------------------------------------

/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param prefix the configuration prefix for this class
 */
public LinearDistribution(String prefix)
{
	super(prefix);
	
	// Read parameters based on type
	if (setter.isInteger()) {
		max=Long.valueOf(Configuration.getLong(prefix + "." + PAR_MAX));
		min=Long.valueOf(Configuration.getLong(prefix + "." + PAR_MIN, 
				-max.longValue()));
		step= (max.longValue()-min.longValue())/
			((double)(Network.size()-1));
	} else { // we know it's double or float
		max = new Double(Configuration.getDouble(prefix+"."+PAR_MAX));
		min = new Double(Configuration.getDouble(prefix+"."+PAR_MIN, 
				-max.doubleValue()));
		step= (max.doubleValue()-min.doubleValue())/(Network.size()-1);
	}
}

// --------------------------------------------------------------------------
// Methods
// --------------------------------------------------------------------------

/**
 * Initializes a protocol vector with values in the range [{@value #PAR_MIN}, 
 * {@value #PAR_MAX}] (inclusive both ends), linearly increasing.
 * @return always false
 */
public boolean execute() {
	
	if ( setter.isInteger() )
	{
		for(int i=0; i<Network.size(); ++i)
		{
			// we avoid the entire expression being cast to double
			setter.set(i,Math.round(i*step)+min.longValue());
		}
	}
	else
	{
		for(int i=0; i<Network.size(); ++i)
		{
			setter.set(i,i*step+min.doubleValue());
		}
	}

	return false;
}


}


