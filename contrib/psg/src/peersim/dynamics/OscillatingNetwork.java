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

import peersim.config.Configuration;
import peersim.core.*;

/**
 * Makes the network size oscillate.
 * The network size will be the function of time, parameterized by this
 * parameter. The size function is
 * <code>avg+sin(time*pi/{@value #PAR_PERIOD})*ampl</code> where
 * <code>avg=({@value #PAR_MAX}+{@value #PAR_MIN})/2</code> and 
 * <code>ampl=({@value #PAR_MAX}-{@value #PAR_MIN})/2</code>.
 * This function is independent of how many times this class is executed, that
 * is, whenever it is executed, it takes the current time and sets the network
 * size accordingly.
 */
public class OscillatingNetwork implements Control
{

//--------------------------------------------------------------------------
//Parameters
//--------------------------------------------------------------------------

/**
 * Config parameter which gives the prefix of node initializers. An arbitrary
 * number of node initializers can be specified (Along with their parameters).
 * These will be applied
 * on the newly created nodes. The initializers are ordered according to
 * alphabetical order if their ID.
 * Example:
 * <pre>
control.0 DynamicNetwork
control.0.init.0 RandNI
control.0.init.0.k 5
control.0.init.0.protocol somelinkable
...
 * </pre>
 * @config
 */
private static final String PAR_INIT = "init";

/**
 * Nodes are added until the size specified by this parameter is reached. The
 * network will never exceed this size as a result of this class.
 * If not set, there will be no limit on the size of the network.
 * @config
 */
private static final String PAR_MAX = "maxsize";

/**
 * Nodes are removed until the size specified by this parameter is reached. The
 * network will never go below this size as a result of this class.
 * Defaults to 0.
 * @config
 */
private static final String PAR_MIN = "minsize";

/**
 * Config parameter used to define the length of one period of the oscillation.
 * The network size will be the function of time, parameterized by this
 * parameter. The size function is
 * <code>avg+sin(time*pi/{@value #PAR_PERIOD})*ampl</code> where
 * <code>avg=({@value #PAR_MAX}+{@value #PAR_MIN})/2</code> and 
 * <code>ampl=({@value #PAR_MAX}-{@value #PAR_MIN})/2</code>.
 * @config
 */
private static final String PAR_PERIOD = "period";


//--------------------------------------------------------------------------
//Fields
//--------------------------------------------------------------------------

/** Period */
private final int period;

/** Maximum size */
private final int minsize;

/** Minimum size */
private final int maxsize;

/** New nodes initializers */
private final NodeInitializer[] inits;


//--------------------------------------------------------------------------
// Initialization
//--------------------------------------------------------------------------

/**
 * Standard constructor that reads the configuration parameters. Invoked by the
 * simulation engine.
 * @param prefix
 *          the configuration prefix for this class
 */
public OscillatingNetwork(String prefix)
{

	period = Configuration.getInt(prefix + "." + PAR_PERIOD);
	maxsize =
		Configuration.getInt(
			prefix + "." + PAR_MAX,
			Integer.MAX_VALUE);
	minsize = Configuration.getInt(prefix + "." + PAR_MIN, 0);

	Object[] tmp = Configuration.getInstanceArray(prefix + "." + PAR_INIT);
	inits = new NodeInitializer[tmp.length];
	for (int i = 0; i < tmp.length; ++i)
	{
		inits[i] = (NodeInitializer) tmp[i];
	}
}

//--------------------------------------------------------------------------
// Methods
//--------------------------------------------------------------------------

/**
 * Adds n nodes to the network. Extending classes can implement any algorithm to
 * do that. The default algorithm adds the given number of nodes after calling
 * all the configured initializers on them.
 * 
 * @param n
 *          the number of nodes to add, must be non-negative.
 */
protected void add(int n)
{
	for (int i = 0; i < n; ++i) {
		Node newnode = (Node) Network.prototype.clone();
		for (int j = 0; j < inits.length; ++j) {
			inits[j].initialize(newnode);
		}
		Network.add(newnode);
	}
}

// ------------------------------------------------------------------

/**
 * Removes n nodes from the network. Extending classes can implement any
 * algorithm to do that. The default algorithm removes <em>random</em>
 * nodes <em>permanently</em> simply by calling {@link Network#remove(int)}.
 * @param n the number of nodes to remove
 */
protected void remove(int n)
{
	for (int i = 0; i < n; ++i) {
		Network.remove(CommonState.r.nextInt(Network.size()));
	}
}

// ------------------------------------------------------------------

/**
 * Takes the current time and sets the network size according to a periodic
 * function of time.
 * The size function is
 * <code>avg+sin(time*pi/{@value #PAR_PERIOD})*ampl</code> where
 * <code>avg=({@value #PAR_MAX}+{@value #PAR_MIN})/2</code> and 
 * <code>ampl=({@value #PAR_MAX}-{@value #PAR_MIN})/2</code>.
 * Calls {@link #add(int)} or {@link #remove} depending on whether the size
 * needs to be increased or decreased to get the desired size.
 * @return always false 
 */
public boolean execute()
{
	long time = CommonState.getTime();
	int amplitude = (maxsize - minsize) / 2;
	int newsize = (maxsize + minsize) / 2 + 
	  (int) (Math.sin(((double) time) / period * Math.PI) *
	  amplitude);
	int diff = newsize - Network.size();
	if (diff < 0)
		remove(-diff);
	else
		add(diff);
	
	return false;
}

}
