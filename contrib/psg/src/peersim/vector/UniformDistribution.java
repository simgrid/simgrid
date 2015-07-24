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
import peersim.dynamics.*;

/**
 * Initializes the values drawing uniform random samples from the range
 * [{@value #PAR_MIN}, {@value #PAR_MAX}[.
 * @see VectControl
 * @see peersim.vector
 */
public class UniformDistribution extends VectControl implements NodeInitializer
{

//--------------------------------------------------------------------------
//Parameter names
//--------------------------------------------------------------------------

/**
 * The upper bound of the uniform random variable, exclusive.
 * @config
 */
private static final String PAR_MAX = "max";

/**
 * The lower bound of the uniform
 * random variable, inclusive. Defaults to -{@value #PAR_MAX}.
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

// --------------------------------------------------------------------------
// Initialization
// --------------------------------------------------------------------------

/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param prefix the configuration prefix for this class
 */
public UniformDistribution(String prefix)
{
	super(prefix);
	
	// Read parameters based on type
	if (setter.isInteger()) {
		max=Long.valueOf(Configuration.getLong(prefix + "." + PAR_MAX));
		min=Long.valueOf(Configuration.getLong(prefix + "." + PAR_MIN, 
				-max.longValue()));
	} else { // we know it's double or float
		max = new Double(Configuration.getDouble(prefix+"."+PAR_MAX));
		min = new Double(Configuration.getDouble(prefix+"."+PAR_MIN, 
				-max.doubleValue()));
	}
}

// --------------------------------------------------------------------------
// Methods
// --------------------------------------------------------------------------

/**
 * Initializes the values drawing uniform random samples from the range
 * [{@value #PAR_MIN}, {@value #PAR_MAX}[.
 * @return always false
 */
public boolean execute() {

	if(setter.isInteger())
	{
		long d = max.longValue() - min.longValue();
		for (int i = 0; i < Network.size(); ++i)
		{
			setter.set(i,CommonState.r.nextLong(d)+min.longValue());
		}
	}
	else
	{
		double d = max.doubleValue() - min.doubleValue();
		for (int i = 0; i < Network.size(); ++i)
		{
			setter.set(i,CommonState.r.nextDouble()*d+
			min.doubleValue());
		}
	}

	return false;
}

// --------------------------------------------------------------------------

/**
 * Initializes the value drawing a uniform random sample from the range
 * [{@value #PAR_MIN}, {@value #PAR_MAX}[.
 * @param n the node to initialize
 */
public void initialize(Node n) {

	if( setter.isInteger() )
	{
		long d = max.longValue() - min.longValue();
		setter.set(n,CommonState.r.nextLong(d) + min.longValue());
	}
	else
	{
		double d = max.doubleValue() - min.doubleValue();
		setter.set(n,CommonState.r.nextDouble()*d);
	}
}

// --------------------------------------------------------------------------

}
