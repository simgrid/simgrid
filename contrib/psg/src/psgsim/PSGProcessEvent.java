package psgsim;

import org.simgrid.msg.Host;
import org.simgrid.msg.MsgException;

import peersim.core.Node;
import peersim.edsim.EDProtocol;

/**
 * This class extends {@link org.simgrid.msg.Process} which creates a process
 * for each host (corresponding to node in peersim) in the system.
 * <p>
 * The main method of this class is to handle events received, by calling the
 * processEvent method on the corresponding node and pid.
 * <p>
 * See {@link peersim.edsim.EDProtocol#processEvent}
 * 
 * @author Khaled Baati 28/10/2014
 * @version version 1.1
 */
public class PSGProcessEvent extends org.simgrid.msg.Process {
	/** The delivered event **/
	private PSGTask task;
	/** The current protocol **/
	private EDProtocol prot;
	/** The identifier of the current protocol **/
	private int pid;

	/**
	 * Constructs a new process from the name of a host.
	 * 
	 * @param host
	 *            the local host to create according to the active node in
	 *            peersim
	 * @param name
	 *            the host's name
	 * @param args
	 *            The arguments of main method of the process.
	 */
	public PSGProcessEvent(Host host, String name, String[] args) {
		super(host, name, args);
	}

	@Override
	public void main(String[] args) throws MsgException {
		Node node = NodeHost.getNode(getHost());
		Host.setAsyncMailbox(getHost().getName());
		while (PSGPlatform.getTime() < PSGPlatform.getDuration()) {
			task = null;
			task = (PSGTask) PSGTask.receive(Host.currentHost().getName(),
					PSGPlatform.psToSgTime(PSGPlatform.getDuration() - PSGPlatform.getTime()-1));
			if (task != null && PSGPlatform.getTime() < PSGPlatform.getDuration()) {
				pid = task.getPid();
				prot = (EDProtocol) node.getProtocol(pid);
				prot.processEvent(node, pid, task.getEvent());
				PSGTransport.flush();
			} else
				break;
		}
	}

}
