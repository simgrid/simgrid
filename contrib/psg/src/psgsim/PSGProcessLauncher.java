package psgsim;

import org.simgrid.msg.Host;
import org.simgrid.msg.MsgException;

import peersim.core.Node;
import peersim.edsim.EDProtocol;
import peersim.edsim.NextCycleEvent;

/**
 * This class extends {@link org.simgrid.msg.Process}, it creates a process for
 * each event added in {@link PSGSimulator#add}.
 * <p>
 * This class performs to launch the appropriate call according to the type of
 * the event;
 * <p>
 * - A NextCycleEvent: the event will be delivered to the
 * {@link PSGProcessCycle} for treatment.
 * <p>
 * - Otherwise the event is delivered to the destination protocol, that must
 * implement {@link EDProtocol}, and the processEvent method is executed.
 * 
 * @author Khaled Baati 12/11/2014
 * @version version 1.1
 */
public class PSGProcessLauncher extends org.simgrid.msg.Process {
	private EDProtocol prot = null;
	private int pid;
	private double delay;
	private Object event;
	private Host host;

	/**
	 * Constructs a new process from the name of a host and with the associated
	 * parameters
	 * 
	 * @param host
	 *            the local host
	 * @param name
	 *            the host's name
	 * @param delay
	 *            the start time of the process
	 * @param event
	 *            the event added to the simulator
	 * @param pid
	 *            the protocol identifier
	 */
	public PSGProcessLauncher(Host host, String name, double delay, Object event,
			int pid) {
		super(host, name, null, delay, -1);
		this.host = host;
		this.pid = pid;
		this.event = event;
		this.delay = delay;
	}

	public PSGProcessLauncher(Host host, String name, String[] args) {

	}

	@Override
	public void main(String[] args) throws MsgException {
		Node node = NodeHost.getNode(host);
		if (event instanceof NextCycleEvent) {
			PSGProcessCycle.nextCycle(Host.currentHost(), host.getName(),
					delay, event, pid);
		} else {
			prot = (EDProtocol) node.getProtocol(pid);
			prot.processEvent(node, pid, event);
			PSGTransport.flush();
		}
		waitFor(500);
	}
}