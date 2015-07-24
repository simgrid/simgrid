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

import peersim.core.*;


/**
 * This interface represents a generic transport protocol, used to
 * send messages through the underlying network. Generally, transport
 * protocols use {@link peersim.edsim.EDSimulator} to schedule the delivery of
 * messages with some appropriate delay. They can also model message omission
 * failure as well.
 * 
 * @author Alberto Montresor
 * @version $Revision: 1.7 $
 */
public interface Transport extends Protocol
{
	
/**
 * Sends message <code>msg</code>	from node <code>src</code> to protocol
 * <code>pid</code> of node <code>dst</code>.
 * 
 * @param src sender node
 * @param dest destination node
 * @param msg message to be sent
 * @param pid protocol identifier
 */
public void send(Node src, Node dest, Object msg, int pid);


/**
 * Return a latency estimate from node <code>src</code> to protocol
 * <code>pid</code> of node <code>dst</code>. 
 * 
 * @param src sender node
 * @param dest destination node
 */
public long getLatency(Node src, Node dest);


}
