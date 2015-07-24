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

import peersim.core.CommonState;


/**
 * This is the common state of a cycle driven simulation that all objects see.
 * It contains additional information, specific to the cycle driven model,
 * in addition to the info in {@link peersim.core.CommonState}.
 */
public class CDState extends CommonState {


// ======================= fields ==================================
// =================================================================

/**
 * Current time within the current cycle.
 * Note that {@link #cycle} gives the cycle id to which this value is relative.
 */
private static int ctime = -1;

/**
 * Current cycle in the simulation. It makes sense only in the case of a
 * cycle based simulator, that is, cycle based simulators will maintain this
 * value, others will not. It still makes sense to keep it separate from
 * {@link #time} because it is an int, while time is a long.
 */
private static int cycle = -1;


// ======================== initialization =========================
// =================================================================


static {}

/** to avoid construction */
private CDState() {}

// ======================= methods =================================
// =================================================================


/**
* Returns true if and only if there is a cycle driven simulation going on.
*/
public static boolean isCD() { return cycle >= 0; }

//-----------------------------------------------------------------

/**
 * Returns the current cycle.
 * Note that {@link #getTime()} returns the same value.
 * @throws UnsupportedOperationException if no cycle-driven state is available
 */
public static int getCycle()
{
	if( cycle >= 0 ) return cycle;
	else throw new UnsupportedOperationException(
		"Cycle driven state accessed when "+
		"no cycle state information is available.");
}

//-----------------------------------------------------------------

/**
 * Sets current cycle. Resets also cycle time to 0. It also calls
 * {@link #setTime(long)} with the given parameter, to make sure 
 * {@link #getTime()} is indeed independent of the simulation model.
 */
public static void setCycle(int t)
{
	cycle = t;
	ctime = 0;
	setTime(t);
}

//-----------------------------------------------------------------

/**
 * Returns current cycle as an Integer object.
 * @throws UnsupportedOperationException if no cycle-driven state is available
 */
public static Integer getCycleObj()
{
	if( cycle >= 0 ) return Integer.valueOf(cycle);
	else throw new UnsupportedOperationException(
		"Cycle driven state accessed when "+
		"no cycle state information is available.");
}

//-----------------------------------------------------------------

/**
 * Returns the current time within the current cycle.
 * Note that the time returned by {@link #getCycle} is the cycle id
 * in this case. In other words, it returns the number of nodes that have
 * already been visited in a given cycle.
 * @throws UnsupportedOperationException if no cycle-driven state is available
 */
public static int getCycleT()
{
	if( ctime >= 0 ) return ctime;
	else throw new UnsupportedOperationException(
		"Cycle driven state accessed when "+
		"no cycle state information is available.");
}

// -----------------------------------------------------------------

public static void setCycleT(int t)
{
	ctime = t;
}
}


