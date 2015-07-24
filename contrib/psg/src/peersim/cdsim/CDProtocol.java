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

package peersim.cdsim;

import peersim.core.Protocol;
import peersim.core.Node;

/**
* Defines cycle driven protocols, that is, protocols that have a periodic
* activity in regular time intervals.
*/
public interface CDProtocol extends Protocol
{

/**
 * A protocol which is defined by performing an algorithm in more or less
 * regular periodic intervals.
 * This method is called by the simulator engine once in each cycle with
 * the appropriate parameters.
 * 
 * @param node
 *          the node on which this component is run
 * @param protocolID
 *          the id of this protocol in the protocol array
 */
public void nextCycle(Node node, int protocolID);

}
