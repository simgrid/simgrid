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

import peersim.Simulator;
import peersim.config.Configuration;
import psgsim.PSGDynamicNetwork;

import java.util.Comparator;
import java.util.Arrays;

import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.MsgException;

/**
 * This class forms the basic framework of all simulations. This is a static
 * singleton which is based on the assumption that we will simulate only one
 * overlay network at a time. This allows us to reduce memory usage in many
 * cases by allowing all the components to directly reach the fields of this
 * class without having to store a reference.
 * <p>
 * The network is a set of nodes implemented via an array list for the sake of
 * efficiency. Each node has an array of protocols. The protocols within a node
 * can interact directly as defined by their implementation, and can be imagined
 * as processes running in a common local environment (i.e. the node). This
 * class is called a "network" because, although it is only a set of nodes, in
 * most simulations there is at least one {@link Linkable} protocol that defines
 * connections between nodes. In fact, such a {@link Linkable} protocol layer
 * can be accessed through a {@link peersim.graph.Graph} view using
 * {@link OverlayGraph}.
 */
public class Network {

	// ========================= fields =================================
	// ==================================================================

	/**
	 * This config property defines the node class to be used. If not set, then
	 * {@link GeneralNode} will be used.
	 * 
	 * @config
	 */
	private static final String PAR_NODE = "network.node";

	/**
	 * This config property defines the initial capacity of the overlay network.
	 * If not set then the value of {@value #PAR_SIZE} will be used. The main
	 * purpose of this parameter is that it allows for optimization. In the case
	 * of scenarios when the network needs to grow, setting this to the maximal
	 * expected size of the network avoids reallocation of memory during the
	 * growth of the network.
	 * 
	 * @see #getCapacity
	 * @config
	 */
	private static final String PAR_MAXSIZE = "network.initialCapacity";

	/**
	 * This config property defines the initial size of the overlay network.
	 * This property is required.
	 * 
	 * @config
	 */
	private static final String PAR_SIZE = "network.size";

	/**
	 * The node array. This is not a private array which is not nice but
	 * efficiency has the highest priority here. The main purpose is to allow
	 * the package quick reading of the contents in a maximally flexible way.
	 * Nevertheless, methods of this class should be used instead of the array
	 * when modifying the contents. Because this array is not private, it is
	 * necessary to know that the actual node set is only the first
	 * {@link #size()} items of the array.
	 */
	static Node[] node = null;

	/**
	 * Actual size of the network.
	 */
	private static int len;

	/**
	 * The prototype node which is used to populate the simulation via cloning.
	 * After all the nodes have been cloned, {@link Control} components can be
	 * applied to perform any further initialization.
	 */
	public static Node prototype = null;

	// ====================== initialization ===========================
	// =================================================================

	/**
	 * Reads configuration parameters, constructs the prototype node, and
	 * populates the network by cloning the prototype.
	 */
	public static void reset() {

		if (prototype != null) {
			// not first experiment
			while (len > 0)
				remove(); // this is to call onKill on all nodes
			prototype = null;
			node = null;
		}

		len = Configuration.getInt(PAR_SIZE);
		int maxlen = Configuration.getInt(PAR_MAXSIZE, len);
		if (maxlen < len)
			throw new IllegalArgumentException(PAR_MAXSIZE + " is less than "
					+ PAR_SIZE);

		node = new Node[maxlen];

		// creating prototype node
		Node tmp = null;
		if (!Configuration.contains(PAR_NODE)) {
			System.err.println("Network: no node defined, using GeneralNode");
			tmp = new GeneralNode("");
		} else {
			tmp = (Node) Configuration.getInstance(PAR_NODE);
		}
		prototype = tmp;
		prototype.setIndex(-1);

		// cloning the nodes
		if (len > 0) {
			for (int i = 0; i < len; ++i) {
				node[i] = (Node) prototype.clone();
				node[i].setIndex(i);
			}
		}
	}

	/** Disable instance construction */
	private Network() {
	}

	// =============== public methods ===================================
	// ==================================================================

	/** Number of nodes currently in the network */
	public static int size() {
		return len;
	}

	// ------------------------------------------------------------------

	/**
	 * Sets the capacity of the internal array storing the nodes. The nodes will
	 * remain the same in the same order. If the new capacity is less than the
	 * old size of the node list, than the end of the list is cut. The nodes
	 * that get removed via this cutting are removed through {@link #remove()}.
	 */
	public static void setCapacity(int newSize) {

		if (node == null || newSize != node.length) {
			for (int i = newSize; i < len; ++i)
				remove();
			Node[] newnodes = new Node[newSize];
			final int l = Math.min(node.length, newSize);
			System.arraycopy(node, 0, newnodes, 0, l);
			node = newnodes;
			if (len > newSize)
				len = newSize;
		}
	}

	// ------------------------------------------------------------------

	/**
	 * Returns the maximal number of nodes that can be stored without
	 * reallocating the underlying array to increase capacity.
	 */
	public static int getCapacity() {
		return node.length;
	}

	// ------------------------------------------------------------------

	/**
	 * The node will be appended to the end of the list. If necessary, the
	 * capacity of the internal array is increased.
	 */
	public static void add(Node n) {
		if (len == node.length)
			setCapacity(3 * node.length / 2 + 1);
		node[len] = n;
		n.setIndex(len);
		len++;
		if (Simulator.getSimID() == 2)
			try {
				PSGDynamicNetwork.add(n);
			} catch (HostNotFoundException e) {
				System.err.println("Host not found in platform file");
			}
		System.err.println("node " + n.getID() + " with index " + n.getIndex()
				+ " was added at time " + CommonState.getTime());
	}

	// ------------------------------------------------------------------

	/**
	 * Returns node with the given index. Note that the same node will normally
	 * have a different index in different times. This can be used as a random
	 * access iterator. This method does not perform range checks to increase
	 * efficiency. The maximal valid index is {@link #size()}.
	 */
	public static Node get(int index) {
		return node[index];
	}

	// ------------------------------------------------------------------

	/**
	 * The node at the end of the list is removed. Returns the removed node. It
	 * also sets the fail state of the node to {@link Fallible#DEAD}.
	 */
	public static Node remove() {
		Node n = node[len - 1]; // if len was zero this throws and exception
		node[len - 1] = null;
		len--;
		System.err.println("node " + n.getID() + " with index " + n.getIndex()
				+ " was removed at time " + CommonState.getTime());
		n.setFailState(Fallible.DEAD);
		if (Simulator.getSimID() == 2)
			PSGDynamicNetwork.remove(n);
		return n;
	}

	// ------------------------------------------------------------------

	/**
	 * The node with the given index is removed. Returns the removed node. It
	 * also sets the fail state of the node to {@link Fallible#DEAD}.
	 * <p>
	 * Look out: the index of the other nodes will not change (the right hand
	 * side of the list is not shifted to the left) except that of the last
	 * node. Only the last node is moved to the given position and will get
	 * index i.
	 */
	public static Node remove(int i) {

		if (i < 0 || i >= len)
			throw new IndexOutOfBoundsException("" + i);
		swap(i, len - 1);
		return remove();
	}

	// ------------------------------------------------------------------

	/**
	 * Swaps the two nodes at the given indexes.
	 */
	public static void swap(int i, int j) {

		Node n = node[i];
		node[i] = node[j];
		node[j] = n;
		node[j].setIndex(j);
		node[i].setIndex(i);
	}

	// ------------------------------------------------------------------

	/**
	 * Shuffles the node array. The index of each node is updated accordingly.
	 */
	public static void shuffle() {

		for (int i = len; i > 1; i--)
			swap(i - 1, CommonState.r.nextInt(i));

	}

	// ------------------------------------------------------------------

	/**
	 * Sorts the node array. The index of each node is updated accordingly.
	 * 
	 * @param c
	 *            The comparator to be used for sorting the nodes. If null, the
	 *            natural order of the nodes is used.
	 */
	public static void sort(Comparator<? super Node> c) {

		Arrays.sort(node, 0, len, c);
		for (int i = 0; i < len; i++)
			node[i].setIndex(i);
	}

	// ------------------------------------------------------------------

	public static void test() {

		System.err.println("number of nodes = " + len);
		System.err.println("capacity (max number of nodes) = " + node.length);
		for (int i = 0; i < len; ++i) {
			System.err.println("node[" + i + "]");
			System.err.println(node[i].toString());
		}

		if (prototype == null)
			return;
		for (int i = 0; i < prototype.protocolSize(); ++i) {
			if (prototype.getProtocol(i) instanceof Linkable)
				peersim.graph.GraphIO.writeUCINET_DL(new OverlayGraph(i),
						System.err);
		}
	}

}
