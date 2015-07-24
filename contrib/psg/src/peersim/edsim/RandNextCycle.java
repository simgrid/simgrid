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

package peersim.edsim;

import peersim.core.*;


/**
* Implements random delay between calling the nextCycle method of the protocol.
* @see #nextDelay
*/
public class RandNextCycle extends NextCycleEvent {


// =============================== initialization ======================
// =====================================================================


/**
* Calls super constructor.
*/
public RandNextCycle(String n) { super(n); }

// --------------------------------------------------------------------

/**
* Calls super.clone().
*/
public Object clone() throws CloneNotSupportedException {
	
	return super.clone();
}


// ========================== methods ==================================
// =====================================================================


/**
* Returns a random delay with uniform distribution between 1 (inclusive) and
* 2*<code>step</code> (exclusive)
* (expected value is therefore <code>step</code>).
*/
protected long nextDelay(long step) {
	
	return 1+CommonState.r.nextLong((step<<1)-1);
}


}


