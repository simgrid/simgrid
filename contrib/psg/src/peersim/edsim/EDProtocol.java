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
		
package peersim.edsim;

import peersim.core.*;

/**
 * The interface to be implemented by protocols run under the event-driven
 * model. A single method is provided, to deliver events to the protocol.
 *
 * @author Alberto Montresor
 * @version $Revision: 1.5 $
 */
public interface EDProtocol 
extends Protocol 
{

	/**
	* This method is invoked by the scheduler to deliver events to the
	* protocol. Apart from the event object, information about the node
	* and the protocol identifier are also provided. Additional information
	* can be accessed through the {@link CommonState} class.
	* 
	* @param node the local node
	* @param pid the identifier of this protocol
	* @param event the delivered event
	*/
	public void processEvent( Node node, int pid, Object event );

}

