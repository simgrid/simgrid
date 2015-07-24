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

/**
 * Generic interface to be implemented by protocols that need to be assigned to
 * routers. The idea is that each node is assigned to a router, by
 * invoking {@link #setRouter(int)} method. Routers are identified by
 * integer indexes (starting from 0), based on the assumption that the
 * router network
 * is static. The router information is then used by different 
 * implementations to compute latency, congestion, etc.
 *
 * @author Alberto Montresor
 * @version $Revision: 1.4 $
 */
public interface RouterInfo
{

/**
 * Associates the node hosting this transport protocol instance with
 * a router in the router network.
 * 
 * @param router the numeric index of the router 
 */
public void setRouter(int router);

/**
 * @return the router associated to this transport protocol.
 */
public int getRouter();

}
