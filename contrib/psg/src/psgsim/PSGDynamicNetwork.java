package psgsim;

import org.simgrid.msg.Host;
import org.simgrid.msg.HostNotFoundException;

import peersim.core.Node;

/**
 * 
 * This class can change the size of networks by adding and removing nodes, as
 * {@link peersim.dynamics.DynamicNetwork} in peersim.
 * 
 * @author Khaled Baati 09/02/2015
 * @version version 1.1
 */
public class PSGDynamicNetwork {

	/**
	 * Removes the node from the network.
	 * 
	 * @param node
	 *            the node to be removed
	 */
	public static void remove(Node node) {
		// NodeHost.getHost(node).off();
		// Host h=NodeHost.mapHostNode.get(node);
		// NodeHost.mapHostNode.remove(node);
		PSGSimulator.size = PSGSimulator.size - 1;
	}

	/**
	 * Adds a node to the network.
	 * 
	 * @param node
	 *            the node to be added
	 * @throws HostNotFoundException 
	 */
	public static void add(Node node) throws HostNotFoundException {
		Host host = PSGPlatform.hostList[(int) node.getID()];
		NodeHost.mapHostNode.put(node, host);
		if (PSGPlatform.interfED)
			new PSGProcessEvent(host, host.getName(), null).start();
		PSGSimulator.size = PSGSimulator.size + 1;
	}
}
