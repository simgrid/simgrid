/**
 * 
 */
package example.chord;

import org.simgrid.msg.Host;

import peersim.core.*;
import peersim.config.Configuration;
import peersim.edsim.EDSimulator;
import psgsim.PSGSimulator;

/**
 * @author Andrea
 * 
 */
public class TrafficGenerator implements Control {

	private static final String PAR_PROT = "protocol";

	private final int pid;

	/**
	 * 
	 */
	public TrafficGenerator(String prefix) {
		pid = Configuration.getPid(prefix + "." + PAR_PROT);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see peersim.core.Control#execute()
	 */
	public boolean execute() {
		int size = Network.size();
		Node sender, target;
		int i = 0;
		do {
			i++;
			sender = Network.get(CommonState.r.nextInt(size));
			target = Network.get(CommonState.r.nextInt(size));
		} while (sender == null || sender.isUp() == false || target == null
				|| target.isUp() == false);
		LookUpMessage message = new LookUpMessage(sender,
				((ChordProtocol) target.getProtocol(pid)).chordId);
		System.out.println("TrafficGenerator at time "+CommonState.getTime()+" Node:"
				+ message.getSender().getID() +" target "+target.getID() + " pid:"
				+ pid);
		EDSimulator.add(10, message, sender, pid);
		return false;
	}

}
