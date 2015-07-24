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

import peersim.config.*;
import peersim.core.*;
import peersim.graph.*;

/**
* This class contains the implementation of the Barabasi-Albert model
* of growing scale free networks. The original model is described in
* <a href="http://arxiv.org/abs/cond-mat/0106096">http://arxiv.org/abs/cond-mat/0106096</a>. It also contains the option of building
* a directed network, in which case the model is a variation of the BA model
* described in <a href="http://arxiv.org/pdf/cond-mat/0408391">
http://arxiv.org/pdf/cond-mat/0408391</a>. In both cases, the number of the
* initial set of nodes is the same as the degree parameter, and no links are
* added. The first added node is connected to all of the initial nodes,
* and after that the BA model is used normally.
* @see GraphFactory#wireScaleFreeBA
*/
public class WireScaleFreeBA extends WireGraph {


// ================ constants ============================================
// =======================================================================

/**
 * The number of edges added to each new node (apart from those forming the 
 * initial network).
 * Passed to {@link GraphFactory#wireScaleFreeBA}.
 * @config
 */
private static final String PAR_DEGREE = "k";


// =================== fields ============================================
// =======================================================================

/** Parameter of the BA model. */
private int k;

// ===================== initialization ==================================
// =======================================================================

/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param prefix the configuration prefix for this class
*/
public WireScaleFreeBA(String prefix)
{
	super(prefix);
	k = Configuration.getInt(prefix + "." + PAR_DEGREE);
}


// ======================== methods =======================================
// ========================================================================


/** calls {@link GraphFactory#wireScaleFreeBA}.*/
public void wire(Graph g) {
	
	GraphFactory.wireScaleFreeBA(g,k,CommonState.r );
}

}

