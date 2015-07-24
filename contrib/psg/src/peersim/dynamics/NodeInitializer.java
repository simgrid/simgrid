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

package peersim.dynamics;

import peersim.core.Node;

/**
 * Generic interface to initialize a node before inserting it into the
 * simulation. Other components like {@link DynamicNetwork} can use a
 * NodeInitializer. It is designed to allow maximal flexibility therefore poses
 * virtually no restrictions on the implementation. It can even be used to
 * implement initializations that require global knowledge of the system.
 */
public interface NodeInitializer
{

/**
 * Performs arbitrary initializations on the given node. It is guaranteed that
 * this is called <em>before</em> inserting the node into the network.
 */
public void initialize(Node n);

}
