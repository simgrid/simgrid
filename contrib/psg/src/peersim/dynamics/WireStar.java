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

/**
 * Takes a {@link peersim.core.Linkable} protocol and adds connection
 * which for a star
 * topology. No connections are removed, they are only added. So it can be used
 * in combination with other initializers.
 * @see GraphFactory#wireStar
 */
public class WireStar extends WireGraph {

// ===================== initialization ==============================
// ===================================================================

/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param prefix the configuration prefix for this class
 */
public WireStar(String prefix) { super(prefix); }

// ===================== public methods ==============================
// ===================================================================


/** Calls {@link GraphFactory#wireStar}.*/
public void wire(Graph g) {
	
	GraphFactory.wireStar(g);
}


}

