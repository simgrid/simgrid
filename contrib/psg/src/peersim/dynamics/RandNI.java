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

import peersim.core.*;
import peersim.config.Configuration;

/**
 * Initializes the neighbor list of a node with random links.
 */
public class RandNI implements NodeInitializer {


//--------------------------------------------------------------------------
//Parameters
//--------------------------------------------------------------------------

/**
 * The protocol to operate on.
 * @config
 */
private static final String PAR_PROT = "protocol";

/**
 * The number of samples (with replacement) to draw to initialize the
 * neighbor list of the node.
 * @config
 */
private static final String PAR_DEGREE = "k";

/**
 * If this config property is defined, method {@link Linkable#pack()} is 
 * invoked on the specified protocol at the end of the wiring phase. 
 * Default to false.
 * @config
 */
private static final String PAR_PACK = "pack";

//--------------------------------------------------------------------------
//Fields
//--------------------------------------------------------------------------

/**
 * The protocol we want to wire
 */
private final int pid;

/**
 * The degree of the regular graph
 */
private final int k;

/**
 * If true, method pack() is invoked on the initialized protocol
 */
private final boolean pack;


//--------------------------------------------------------------------------
//Initialization
//--------------------------------------------------------------------------

/**
 * Standard constructor that reads the configuration parameters. Invoked by the
 * simulation engine.
 * @param prefix the configuration prefix for this class
 */
public RandNI(String prefix)
{
	pid = Configuration.getPid(prefix + "." + PAR_PROT);
	k = Configuration.getInt(prefix + "." + PAR_DEGREE);
	pack = Configuration.contains(prefix + "." + PAR_PACK);
}

//--------------------------------------------------------------------------
//Methods
//--------------------------------------------------------------------------

/**
 * Takes {@value #PAR_DEGREE} random samples with replacement from the nodes of
 * the overlay network. No loop edges are added.
 */
public void initialize(Node n)
{
	if (Network.size() == 0) return;

	Linkable linkable = (Linkable) n.getProtocol(pid);
	for (int j = 0; j < k; ++j)
	{
		int r = CommonState.r.nextInt(Network.size());
		linkable.addNeighbor(Network.get(r));
	}

	if (pack) linkable.pack();
}

}

