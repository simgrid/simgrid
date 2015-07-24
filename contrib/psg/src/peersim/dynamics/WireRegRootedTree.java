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
import peersim.config.*;

/**
 * Takes a {@link peersim.core.Linkable} protocol and adds connections that
 * define a regular
 * rooted tree. Note that no
 * connections are removed, they are only added. So it can be used in
 * combination with other initializers.
 * @see #wire
 * @see GraphFactory#wireRegRootedTree
 */
public class WireRegRootedTree extends WireGraph {

//--------------------------------------------------------------------------
//Parameters
//--------------------------------------------------------------------------

/**
 * The parameter of the tree wiring method.
 * It is passed to {@link GraphFactory#wireRegRootedTree}.
 * @config
 */
private static final String PAR_DEGREE = "k";

//--------------------------------------------------------------------------
//Fields
//--------------------------------------------------------------------------

private final int k;

//--------------------------------------------------------------------------
//Initialization
//--------------------------------------------------------------------------

/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param prefix the configuration prefix for this class
 */
public WireRegRootedTree(String prefix)
{
	super(prefix);
	k = Configuration.getInt(prefix + "." + PAR_DEGREE);
}

//--------------------------------------------------------------------------
//Methods
//--------------------------------------------------------------------------

/** Calls {@link GraphFactory#wireRegRootedTree}. */
public void wire(Graph g) {

	GraphFactory.wireRegRootedTree(g,k);
}

}
