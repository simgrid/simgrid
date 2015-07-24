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

import peersim.graph.*;
import peersim.config.Configuration;

/**
 * Takes a {@link peersim.core.Linkable} protocol and adds edges that
 * define a ring lattice.
 * Note that no connections are removed, they are only added. So it can be used
 * in combination with other initializers.
 * @see  GraphFactory#wireRingLattice
 */
public class WireRingLattice extends WireGraph {

// --------------------------------------------------------------------------
// Parameters
// --------------------------------------------------------------------------

/**
 * The "lattice parameter" of the graph. The out-degree of the graph is equal to
 * 2k. See {@link GraphFactory#wireRingLattice} (to which this parameter is
 * passed) for further details.
 * @config
 */
private static final String PAR_K = "k";

// --------------------------------------------------------------------------
// Fields
// --------------------------------------------------------------------------

/**
 */
private final int k;

// --------------------------------------------------------------------------
// Initialization
// --------------------------------------------------------------------------

/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param prefix the configuration prefix for this class
 */
public WireRingLattice(String prefix)
{
	super(prefix);
	k = Configuration.getInt(prefix + "." + PAR_K);
}

//--------------------------------------------------------------------------
//Public methods
//--------------------------------------------------------------------------

/** calls {@link GraphFactory#wireRingLattice}. */
public void wire(Graph g)
{
	GraphFactory.wireRingLattice(g, k);
}

//--------------------------------------------------------------------------

}
