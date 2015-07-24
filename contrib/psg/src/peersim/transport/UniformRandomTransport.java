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

package peersim.transport;

import peersim.config.*;
import peersim.core.*;
import peersim.edsim.*;


/**
 * Implement a transport layer that reliably delivers messages with a random
 * delay, that is drawn from the configured interval according to the uniform
 * distribution.
 *
 * @author Alberto Montresor
 * @version $Revision: 1.14 $
 */
public final class UniformRandomTransport implements Transport
{

//---------------------------------------------------------------------
//Parameters
//---------------------------------------------------------------------

/** 
 * String name of the parameter used to configure the minimum latency.
 * @config
 */	
private static final String PAR_MINDELAY = "mindelay";	
	
/** 
 * String name of the parameter used to configure the maximum latency.
 * Defaults to {@value #PAR_MINDELAY}, which results in a constant delay.
 * @config 
 */	
private static final String PAR_MAXDELAY = "maxdelay";	
	
//---------------------------------------------------------------------
//Fields
//---------------------------------------------------------------------

/** Minimum delay for message sending */
private final long min;
	
/** Difference between the max and min delay plus one. That is, max delay is
* min+range-1.
*/
private final long range;

	
//---------------------------------------------------------------------
//Initialization
//---------------------------------------------------------------------

/**
 * Reads configuration parameter.
 */
public UniformRandomTransport(String prefix)
{
	min = Configuration.getLong(prefix + "." + PAR_MINDELAY);
	long max = Configuration.getLong(prefix + "." + PAR_MAXDELAY,min);
	if (max < min) 
	   throw new IllegalParameterException(prefix+"."+PAR_MAXDELAY, 
	   "The maximum latency cannot be smaller than the minimum latency");
	range = max-min+1;
}

//---------------------------------------------------------------------

/**
* Returns <code>this</code>. This way only one instance exists in the system
* that is linked from all the nodes. This is because this protocol has no
* node specific state.
*/
public Object clone()
{
	return this;
}

//---------------------------------------------------------------------
//Methods
//---------------------------------------------------------------------

/**
 * Delivers the message with a random
 * delay, that is drawn from the configured interval according to the uniform
 * distribution.
*/
public void send(Node src, Node dest, Object msg, int pid)
{
	// avoid calling nextLong if possible
	long delay = (range==1?min:min + CommonState.r.nextLong(range));
	EDSimulator.add(delay, msg, dest, pid);
}

/**
 * Returns a random
 * delay, that is drawn from the configured interval according to the uniform
 * distribution.
*/
public long getLatency(Node src, Node dest)
{
	return (range==1?min:min + CommonState.r.nextLong(range));
}


}
