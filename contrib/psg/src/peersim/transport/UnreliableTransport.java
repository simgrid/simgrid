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


/**
 * This transport protocol can be combined with other transports
 * to simulate message losses. Its behavior is the following: each message
 * can be dropped based on the configured probability, or it will be sent
 * using the underlying transport protocol. 
 * <p>
 * The memory requirements are minimal, as a single instance is created and 
 * inserted in the protocol array of all nodes (because instances have no state
 * that depends on the hosting node). 
 *
 * @author Alberto Montresor
 * @version $Revision: 1.13 $
 */
public final class UnreliableTransport implements Transport
{

//---------------------------------------------------------------------
//Parameters
//---------------------------------------------------------------------

/**
 * The name of the underlying transport protocol. This transport is
 * extended with dropping messages.
 * @config
 */
private static final String PAR_TRANSPORT = "transport";

/** 
 * String name of the parameter used to configure the probability that a 
 * message sent through this transport is lost.
 * @config
 */
private static final String PAR_DROP = "drop";


//---------------------------------------------------------------------
//Fields
//---------------------------------------------------------------------

/** Protocol identifier for the support transport protocol */
private final int transport;

/** Probability of dropping messages */
private final float loss;

//---------------------------------------------------------------------
//Initialization
//---------------------------------------------------------------------

/**
 * Reads configuration parameter.
 */
public UnreliableTransport(String prefix)
{
	transport = Configuration.getPid(prefix+"."+PAR_TRANSPORT);
	loss = (float) Configuration.getDouble(prefix+"."+PAR_DROP);
}

//---------------------------------------------------------------------

/**
* Returns <code>this</code>. This way only one instance exists in the system
* that is linked from all the nodes. This is because this protocol has no
* state that depends on the hosting node.
 */
public Object clone()
{
	return this;
}

//---------------------------------------------------------------------
//Methods
//---------------------------------------------------------------------

/** Sends the message according to the underlying transport protocol.
* With the configured probability, the message is not sent (i.e. the method does
* nothing).
*/
public void send(Node src, Node dest, Object msg, int pid)
{
	try
	{
		if (CommonState.r.nextFloat() >= loss)
		{
			// Message is not lost
			Transport t = (Transport) src.getProtocol(transport);
			t.send(src, dest, msg, pid);
		}
	}
	catch(ClassCastException e)
	{
		throw new IllegalArgumentException("Protocol " +
				Configuration.lookupPid(transport) + 
				" does not implement Transport");
	}
}

/** Returns the latency of the underlying protocol.*/
public long getLatency(Node src, Node dest)
{
	Transport t = (Transport) src.getProtocol(transport);
	return t.getLatency(src, dest);
}

}
