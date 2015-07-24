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

import peersim.core.*;

/**
* Shuffles the network. After shuffling, the order in which the nodes
* are iterated over during a cycle of a cycle driven simulation
* will be random. It has an effect only in cycle driven simulations.
*/
public class Shuffle implements Control {


// ========================= fields =================================
// ==================================================================

// ==================== initialization ==============================
// ==================================================================

/** Does nothing. */
public Shuffle(String prefix) {}


// ===================== public methods ==============================
// ===================================================================


/**
* Calls {@link Network#shuffle()}. 
* As a result, the order in which the nodes
* are iterated over during a cycle of a cycle driven simulation
* will be random. It has an effect only in cycle driven simulations.
*/
public final boolean execute() {
	Network.shuffle();
	return false;
}


}


