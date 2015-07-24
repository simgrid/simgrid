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
 * Normalizes the values of a protocol vector.
 * The normalization is based on the L1 norm, that is, the sum of the
 * absolute values of the vector elements. Parameter {@value #PAR_L1} defines
 * the L1 norm that the vector will have after normalization.
 * @see VectControl
 * @see peersim.vector
 */
public class Normalizer extends VectControl
{

// --------------------------------------------------------------------------
// Parameters
// --------------------------------------------------------------------------

/**
 * The L1 norm (sum of absolute values) to normalize to. After the operation the
 * L1 norm will be the value given here. Defaults to 1.
 * @config
 */
private static final String PAR_L1 = "l1";


// --------------------------------------------------------------------------
// Fields
// --------------------------------------------------------------------------

/** L1 norm */
private final double l1;


// --------------------------------------------------------------------------
// Initialization
// --------------------------------------------------------------------------

/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param prefix the configuration prefix for this class
 */
public Normalizer(String prefix)
{
	super(prefix);
	l1 = Configuration.getDouble(prefix + "." + PAR_L1, 1);
	
	if( setter.isInteger() ) 
		throw new IllegalParameterException(prefix + "." + PAR_METHOD,
			"setter value must be floating point, instead of "+
			setter.getType());
			
	if( setter.getType() !=  getter.getType() )
		throw new IllegalParameterException(prefix + "." + PAR_GETTER,
		"getter and setter must have the same numeric type, "+
		"but we have "+setter.getType()+" and "+getter.getType());
}

//--------------------------------------------------------------------------
//Methods
//--------------------------------------------------------------------------

/**
 * Makes the sum of the absolute values (L1 norm) equal to the value
 * given in the configuration parameter {@value #PAR_L1}. If the value is
 * negative, the L1 norm will be the absolute value and the vector elements
 * change sign.
 * @return always false
 */
public boolean execute() {
	
	double sum = 0.0;
	for (int i = 0; i < Network.size(); ++i)
	{
		sum += getter.getDouble(i);
	}
	if (sum == 0.0)
	{
		throw new
		RuntimeException("Attempted to normalize all zero vector.");
	}
	double factor = l1 / sum;
	for (int i = 0; i < Network.size(); ++i)
	{
		double val = getter.getDouble(i)*factor;
		setter.set(i,val);
	}
	return false;
}

//--------------------------------------------------------------------------

}
