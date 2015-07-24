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
		
package peersim.core;

/**
* A protocol that does nothing but knows everything.
* It provides an interface which models a protocol that knows all nodes
* in the network, i.e. the neighborhood set of this protocol is always the
* whole node set. this protocol is also extremely cheap, in fact it
* has no data fields.
*/
public final class OracleIdleProtocol implements Protocol, Linkable {

// =================== initialization, creation ======================
// ===================================================================


/** Does nothing */
public OracleIdleProtocol(String prefix) {}

// --------------------------------------------------------------------

/** Returns <tt>this</tt> to maximize memory saving. It contains no fields.*/
public Object clone() { return this; }


// ===================== public methods ===============================
// ====================================================================


/** This is an expensive operation, should not be used at all.
* It returns false only if the given node is not in the current network.
*/
public boolean contains(Node n) {

	final int len = Network.size();
	for (int i=0; i < len; i++)
	{
		if (Network.node[i] == n)
		return true;
	}
	return false;
}

// --------------------------------------------------------------------

/** Unsupported operation */
public boolean addNeighbor(Node n) {

	throw new UnsupportedOperationException();
}

// --------------------------------------------------------------------
  
/**
* The neighborhood contains the node itself, ie it contains the loop
* edge.
*/
public Node getNeighbor(int i) {
	
	return Network.node[i];
}

// --------------------------------------------------------------------

public int degree() {
	
	return Network.size();
}

// --------------------------------------------------------------------

public void pack() {}  

// --------------------------------------------------------------------

public void onKill() {}

// --------------------------------------------------------------------

public String toString() {

	return degree()+" [all the nodes of the overlay network]";
}

}

