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
		
package peersim.reports;

import peersim.core.*;
import peersim.config.Configuration;
import peersim.graph.*;
import peersim.cdsim.CDState;

/**
* Class that provides functionality for observing graphs.
* It can efficiently create an undirected version of the graph, making sure
* it is updated only when the simulation has advanced already, and provides
* some common parameters.
*/
public abstract class GraphObserver implements Control {


// ===================== fields =======================================
// ====================================================================

/**
 * The protocol to operate on.
 * @config
 */
private static final String PAR_PROT = "protocol";

/**
 * If defined, the undirected version of the graph will be analyzed. Not defined
 * by default.
 * @config
 */
protected static final String PAR_UNDIR = "undir";

/**
* Alias for {@value #PAR_UNDIR}.
* @config
*/
private static final String PAR_UNDIR_ALT = "undirected";

/**
 * If defined, the undirected version of the graph will be stored using much
 * more memory but observers will be in general a few times faster. As a
 * consequence, it will not work with large graphs. Not defined by default. It
 * is a static property, that is, it affects all graph observers that are used
 * in a simulation. That is, it is not a parameter of any observer, the name
 * should be specified as a standalone property.
 * @config
 */
private static final String PAR_FAST = "graphobserver.fast";

/** The name of this observer in the configuration */
protected final String name;

protected final int pid;

protected final boolean undir;

protected final GraphAlgorithms ga = new GraphAlgorithms();

protected Graph g;

// ---------------------------------------------------------------------

private static int lastpid = -1234;

private static long time = -1234;

private static int phase = -1234;

private static int ctime = -1234;

private static Graph dirg;

private static Graph undirg;

private static boolean fast;

/** If any instance of some extending class defines undir we need to
maintain an undir graph. Note that the graph is stored in a static
field so it is common to all instances. */
private static boolean needUndir=false;

// ===================== initialization ================================
// =====================================================================


/**
 * Standard constructor that reads the configuration parameters.
 * Invoked by the simulation engine.
 * @param name the configuration prefix for this class
 */
protected GraphObserver(String name) {

	this.name = name;
	pid = Configuration.getPid(name+"."+PAR_PROT);
	undir = (Configuration.contains(name + "." + PAR_UNDIR) |
		Configuration.contains(name + "." + PAR_UNDIR_ALT));
	GraphObserver.fast = Configuration.contains(PAR_FAST);
	GraphObserver.needUndir = (GraphObserver.needUndir || undir);
}


// ====================== methods ======================================
// =====================================================================

/**
* Sets {@link #g}.
* It MUST be called by any implementation of {@link #execute()} before
* doing anything else.
* Attempts to initialize {@link #g} from a
* pre-calculated graph stored in a static field, but first it
* checks whether it needs to be updated.
* If the simulation time has progressed or it was calculated for a different
* protocol, then updates this static graph as well.
* The purpose of this mechanism is to save the time of constructing the
* graph if many observers are run on the same graph. Time savings can be very
* significant if the undirected version of the same graph is observed by many
* observers.
*/
protected void updateGraph() {
	
	if( CommonState.getTime() != GraphObserver.time ||
	    (CDState.isCD() && (CDState.getCycleT() != GraphObserver.ctime)) ||
	    CommonState.getPhase() != GraphObserver.phase ||
	    pid != GraphObserver.lastpid )
	{
		// we need to update the graphs
		
		GraphObserver.lastpid = pid;
		GraphObserver.time = CommonState.getTime();
		if( CDState.isCD() ) GraphObserver.ctime = CDState.getCycleT();
		GraphObserver.phase = CommonState.getPhase();

		GraphObserver.dirg = new OverlayGraph(pid);
		if( GraphObserver.needUndir )
		{
			if( fast )
				GraphObserver.undirg =
				new FastUndirGraph(GraphObserver.dirg);
			else
				GraphObserver.undirg =
				new ConstUndirGraph(GraphObserver.dirg);
		}
	}
	
	if( undir ) g = GraphObserver.undirg;
	else g = GraphObserver.dirg;
}

}



