package psgsim;

import java.util.Map.Entry;

import org.simgrid.msg.Host;
import peersim.cdsim.CDProtocol;
import peersim.core.Node;
import peersim.core.Protocol;

/**
 * This class handle an event of type NextCycleEvent, received on the protocol
 * identified by a pid among all CDProtocols defined in the
 * {@link PSGPlatform#cdProtocolsStepMap} collection.
 * <p>
 * It executes the nextCyle method associated to this protocol.
 * 
 * @author Khaled Baati 27/10/2014
 * @version version 1.1
 */
public class PSGProcessCycle {

	/**
	 * Executes the nextCycle method of the CDprotocol with the appropriate
	 * parameters, and schedules the next call using {@link PSGSimulator#add}.
	 * 
	 * @param host
	 *            the host on which this component is run
	 * @param name
	 *            the host's name
	 * @param delay
	 *            the start time
	 * @param event
	 *            the actual event
	 * @param pid
	 *            the protocol identifier
	 */
	public static void nextCycle(Host host, String name, double delay,
			Object event, int pid) {
		CDProtocol cdp = null;
		Node node = NodeHost.getNode(host);
		cdp = (CDProtocol) node.getProtocol(pid);
		cdp.nextCycle(node, pid);
		PSGTransport.flush();
		for (Entry<Protocol, Double> entry : PSGPlatform.cdProtocolsStepMap
				.entrySet()) {
			Double step = entry.getValue();
			for (Entry<Protocol, Integer> p : PSGPlatform.protocolsPidsMap
					.entrySet()) {
				if (p.getValue() == pid) {
					break;
				}
			}
			if (PSGPlatform.getTime() <= PSGPlatform.getDuration() && host.isOn()) {
				PSGSimulator.add(PSGPlatform.sgToPsTime(step), event, node, pid);
			}
		}
	}

}
