package psgsim;

import org.simgrid.msg.Host;
import peersim.core.Network;
import peersim.core.Node;

import java.util.Comparator;
import java.util.Map;
import java.util.TreeMap;

/**
 * 
 * NodeHost class used to make the mapping Node-Host.
 * 
 * @author Khaled Baati 26/10/2014
 * @version version 1.1
 */
public class NodeHost {

	/**
	 * A collection of map contained the couple (host,node)
	 */
	public static TreeMap<Node, Host> mapHostNode = new TreeMap<Node, Host>(
			new Comparator<Node>() {
				public int compare(Node n1, Node n2) {
					return String.valueOf(n1.getID()).compareTo(
							String.valueOf(n2.getID()));
				}
			});

	/**
	 * The main method to make the mapping Node to Host in the
	 * {@link #mapHostNode} field
	 */
	public static void start() {
		Host host = null;
		for (Integer i = 0; i < PSGSimulator.size; i++) {
			host = PSGPlatform.hostList[i];
			mapHostNode.put(Network.get(i), host);
		}
	}

	/**
	 * This static method gets a Node instance associated with a host of your
	 * platform.
	 * 
	 * @param host
	 *            The host associated in your platform.
	 * @return The node associated.
	 */
	public static Node getNode(Host host) {
		for (Map.Entry<Node, Host> element : mapHostNode.entrySet()) {
			if (element.getValue() == host)
				return element.getKey();
		}
		return null;
	}

	/**
	 * This static method gets a host instance associated with the node of your
	 * platform.
	 * 
	 * @param node
	 *            The node associated in your platform.
	 * @return The host associated, else return null (host doesn't exist).
	 */
	public static Host getHost(Node node) {
		for (Map.Entry<Node, Host> element : mapHostNode.entrySet()) {
			if (element.getKey() == node)
				return element.getValue();
		}
		return null;
	}
}
