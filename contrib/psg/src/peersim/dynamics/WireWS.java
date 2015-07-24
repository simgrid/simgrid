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
import peersim.core.*;
import peersim.config.Configuration;

/**
* Takes a {@link Linkable} protocol and adds connections following the
* small-world model of Watts and Strogatz. Note that no
* connections are removed, they are only added. So it can be used in
* combination with other initializers.
* @see GraphFactory#wireWS
*/
public class WireWS extends WireGraph {


// ========================= fields =================================
// ==================================================================

/**
 * The beta parameter of a Watts-Strogatz graph represents the probability for a
 * node to be re-wired.
 * Passed to {@link GraphFactory#wireWS}.
 * @config
 */
private static final String PAR_BETA = "beta";

/**
 * The degree of the graph. See {@link GraphFactory#wireRingLattice}.
 * Passed to {@link GraphFactory#wireWS}.
 * @config
 */
private static final String PAR_DEGREE = "k";

/**
 * The degree of the regular graph
 */
private final int k;

/**
 * The degree of the regular graph
 */
private final double beta;


// ==================== initialization ==============================
//===================================================================


/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param prefix the configuration prefix for this class
 */
public WireWS(String prefix) {

	super(prefix);
	k = Configuration.getInt(prefix+"."+PAR_DEGREE);
	beta = Configuration.getDouble(prefix+"."+PAR_BETA);
}


// ===================== public methods ==============================
// ===================================================================


/** calls {@link GraphFactory#wireWS}.*/
public void wire(Graph g) {

	GraphFactory.wireWS(g,k,beta,CommonState.r);
}

}

