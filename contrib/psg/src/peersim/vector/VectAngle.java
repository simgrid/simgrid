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

/**
 * Observes the cosine angle between two vectors. The number which is output is
 * the inner product divided by the product of the length of the vectors.
 * All values are converted to double before processing.
 * <p>
 * This observer class can observe any protocol field containing a 
 * primitive value, provided that the field is associated with a getter method 
 * that reads it.
 * The methods to be used are specified through parameter {@value #PAR_METHOD1}
 * and {@value #PAR_METHOD2}.
 * <p>
 * Please refer to package {@link peersim.vector} for a detailed description of 
 * this mechanism. 
 */
public class VectAngle implements Control
{

// --------------------------------------------------------------------------
// Parameters
// --------------------------------------------------------------------------

/**
 * The first protocol to be observed.
 * @config
 */
private static final String PAR_PROT1 = "protocol1";

/**
 * The second protocol to be observed.
 * @config
 */
private static final String PAR_PROT2 = "protocol2";

/**
 * The getter method used to obtain the values of the first protocol. 
 * Defaults to <code>getValue</code> (for backward compatibility with previous 
 * implementation of this class, that were based on the 
 * {@link SingleValue} interface).
 * Refer to the {@linkplain peersim.vector vector package description} for more 
 * information about getters and setters.
 * @config
 */
private static final String PAR_METHOD1 = "getter1";

/**
 * The getter method used to obtain the values of the second protocol. 
 * Defaults to <code>getValue</code> (for backward compatibility with previous 
 * implementation of this class, that were based on the 
 * {@link SingleValue} interface).
 * Refer to the {@linkplain peersim.vector vector package description} for more 
 * information about getters and setters.
 * @config
 */
private static final String PAR_METHOD2 = "getter2";

// --------------------------------------------------------------------------
// Fields
// --------------------------------------------------------------------------

/** The prefix for this observer*/
private final String name;

private final Getter getter1;

private final Getter getter2;

// --------------------------------------------------------------------------
// Initialization
// --------------------------------------------------------------------------

/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param prefix the configuration prefix for this class
 */
public VectAngle(String prefix)
{
	name = prefix;
	getter1 = new Getter(prefix,PAR_PROT1,PAR_METHOD1);
	getter2 = new Getter(prefix,PAR_PROT2,PAR_METHOD2);
}

// --------------------------------------------------------------------------
// Methods
// --------------------------------------------------------------------------

/**
 * Observes the cosine angle between two vectors. The printed values
 * are: cosine, Eucledian norm of vect 1, Eucledian norm of vector 2,
 * angle in radians.
* @return always false
*/
public boolean execute() {

	double sqrsum1 = 0, sqrsum2 = 0, prod = 0;
	for (int i = 0; i < Network.size(); ++i)
	{
		double v1= getter1.get(i).doubleValue();
		double v2= getter2.get(i).doubleValue();
		sqrsum1 += v1 * v1;
		sqrsum2 += v2 * v2;
		prod += v2 * v1;
	}
	
	double cos = prod / Math.sqrt(sqrsum1) / Math.sqrt(sqrsum2);
	
	// deal with numeric errors
	if( cos > 1 ) cos=1;
	if( cos < -1 ) cos = -1;
	
	System.out.println(name+": " + cos + " "
			+ Math.sqrt(sqrsum1) + " " + Math.sqrt(sqrsum2) + " "
			+ Math.acos(cos));
	return false;
}

//--------------------------------------------------------------------------

}
