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
 * This transport protocol is based on the {@link E2ENetwork} class.
 * Each instance
 * of this transport class is assigned to one of the routers contained in
 * the (fully static singleton) {@link E2ENetwork},
 * and subsequently the {@link E2ENetwork} class is used to obtain the
 * latency for messages sending based on the router assignment.
 *
 * @author Alberto Montresor
 * @version $Revision: 1.11 $
 */
public class E2ETransport implements Transport, RouterInfo
{

//---------------------------------------------------------------------
//Parameters
//---------------------------------------------------------------------

/**
 * The delay that corresponds to the time spent on the source (and destination)
 * nodes. In other words, full latency is calculated by fetching the latency
 * that belongs to communicating between two routers, incremented by
 * twice this delay. Defaults to 0.
 * @config
 */
private static final String PAR_LOCAL = "local";
	
//---------------------------------------------------------------------
//Static fields
//---------------------------------------------------------------------

/** Identifier of this transport protocol */
private static int tid;
	
/** Local component of latency */
private static long local;

//---------------------------------------------------------------------
//Fields
//---------------------------------------------------------------------

/** Identifier of the internal node */
private int router = -1;
	
//---------------------------------------------------------------------
//Initialization
//---------------------------------------------------------------------

/**
 * Reads configuration parameters.
 */
public E2ETransport(String prefix)
{
	tid = CommonState.getPid();
	local = Configuration.getLong(prefix + "." + PAR_LOCAL, 0);
}

//---------------------------------------------------------------------

/**
 * Clones the object.
 */
public Object clone()
{
	E2ETransport e2e=null;
	try { e2e=(E2ETransport)super.clone(); }
	catch( CloneNotSupportedException e ) {} // never happens
	return e2e;
}

//---------------------------------------------------------------------
//Methods inherited by Transport
//---------------------------------------------------------------------

/**
* Delivers the message reliably, with the latency calculated by
* {@link #getLatency}.
*/
public void send(Node src, Node dest, Object msg, int pid)
{
	/* Assuming that the sender corresponds to the source node */
	E2ETransport sender = (E2ETransport) src.getProtocol(tid);
	E2ETransport receiver = (E2ETransport) dest.getProtocol(tid);
	long latency =
	   E2ENetwork.getLatency(sender.router, receiver.router) + local*2;
	EDSimulator.add(latency, msg, dest, pid);
}

//---------------------------------------------------------------------

/**
* Calculates latency using the static singleton {@link E2ENetwork}.
* It looks up which routers the given nodes are assigned to, then
* looks up the corresponding latency. Finally it increments this value
* by adding twice the local delay configured by {@value #PAR_LOCAL}.
*/
public long getLatency(Node src, Node dest)
{
	/* Assuming that the sender corresponds to the source node */
	E2ETransport sender = (E2ETransport) src.getProtocol(tid);
	E2ETransport receiver = (E2ETransport) dest.getProtocol(tid);
	return E2ENetwork.getLatency(sender.router, receiver.router) + local*2;
}


//---------------------------------------------------------------------
//Methods inherited by RouterInfo
//---------------------------------------------------------------------

/**
 * Associates the node hosting this transport protocol instance with
 * a router in the router network.
 * 
 * @param router the numeric index of the router 
 */
public void setRouter(int router)
{
	this.router = router;
}

//---------------------------------------------------------------------

/**
 * @return the router associated to this transport protocol.
 */
public int getRouter()
{
	return router;
}

}
