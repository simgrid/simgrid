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
 * Initializes the values so that {@value #PAR_PEAKS} nodes have value
 * {@value #PAR_VALUE}/{@value #PAR_PEAKS}, the rest {@value #PAR_LVALUE}
 * (zero by default).
 * @see VectControl
 * @see peersim.vector
 */
public class PeakDistribution extends VectControl
{

// --------------------------------------------------------------------------
// Parameters
// --------------------------------------------------------------------------

/** 
 * The sum of values in the system, to be equally distributed between peak 
 * nodes.
 * @config
 */
private static final String PAR_VALUE = "value";

/** 
 * The value for the nodes that are not peaks. This parameter is optional,
 * by default, the nodes that are
 * not peaks are set to zero. This value overrides that behavior.
 * @config
 */
private static final String PAR_LVALUE = "background";

/** 
 * The number of peaks in the system. If this value is greater than or equal to
 * 1, it is interpreted as the actual number of peaks. If it is included in 
 * the range [0, 1] it is interpreted as a percentage with respect to the
 * current network size. Defaults to 1. 
 * @config
 */
private static final String PAR_PEAKS = "peaks";


// --------------------------------------------------------------------------
// Fields
// --------------------------------------------------------------------------

/** Total load */
private final Number lvalue;

/** Total load */
private final Number value;

/** Number of peaks */
private final double peaks;

// --------------------------------------------------------------------------
// Initialization
// --------------------------------------------------------------------------

/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param prefix the configuration prefix for this class
 */
public PeakDistribution(String prefix)
{
	super(prefix);
	
	peaks = Configuration.getDouble(prefix+"."+PAR_PEAKS, 1);
	
	if( setter.isInteger() )
	{
		value=Long.valueOf(Configuration.getLong(prefix+"."+PAR_VALUE));
		lvalue=Long.valueOf(Configuration.getLong(prefix+"."+PAR_LVALUE,0));
	}
	else
	{
		value = new Double(Configuration.getDouble(prefix + "." +
		PAR_VALUE));
		lvalue = new Double(Configuration.getDouble(prefix + "." +
		PAR_LVALUE,0));
	}
}

// --------------------------------------------------------------------------
// Methods
// --------------------------------------------------------------------------

/**
 * Initializes the values so that {@value #PAR_PEAKS} nodes have value
 * {@value #PAR_VALUE}/{@value #PAR_PEAKS}, the rest zero.
 * @return always false
 */
public boolean execute()
{
	int pn = (peaks < 1 ? (int) (peaks*Network.size()) : (int) peaks);
	
	if( setter.isInteger() )
	{
		long v = value.longValue()/pn;
		long lv = lvalue.longValue();
		for (int i=0; i < pn; i++) setter.set(i, v);
		for (int i=pn; i < Network.size(); i++) setter.set(i,lv);
	}
	else
	{
		double v = value.doubleValue()/pn;
		double lv = lvalue.doubleValue();
		for (int i=0; i < pn; i++) setter.set(i, v);
		for (int i=pn; i < Network.size(); i++) setter.set(i,lv);
	}

	return false;
}

// --------------------------------------------------------------------------

}
