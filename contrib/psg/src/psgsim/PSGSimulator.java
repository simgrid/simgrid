package psgsim;

import org.simgrid.msg.Host;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.Msg;
import org.simgrid.msg.NativeException;

import peersim.core.CommonState;
import peersim.core.Network;
import peersim.core.Node;

/**
 * This is the main entry point to Simgrid simulator. This class loads the
 * different parameters and initializes the simulation.
 * 
 * @author Khaled Baati 14/10/2014
 * @version version 1.1
 */
public class PSGSimulator {
	public static int size;

	static {
		Network.reset();
		size = Network.size();
	}

	// ========================== methods ==================================
	// =====================================================================

	/**
	 * Adds a new event to be scheduled, specifying the number of time units of
	 * delay (in seconds), the node and the protocol identifier to which the
	 * event will be delivered. A {@link psgsim.PSGProcessLauncher} process will
	 * be created according to this event.
	 * 
	 * @param delay
	 *            The number of time units (seconds in simgrid) before the event
	 *            is scheduled.
	 * @param event
	 *            The object associated to this event
	 * @param src
	 *            The node associated to the event.
	 * @param pid
	 *            The identifier of the protocol to which the event will be
	 *            delivered
	 */
	public static void add(long delay, Object event, Node src, int pid) {
		Host host = NodeHost.getHost(src);
		double startTime = PSGPlatform.psToSgTime(delay) + Msg.getClock(); 
		if (startTime < PSGPlatform.psToSgTime(PSGPlatform.getDuration()) ) {
			try {
				/**
				 * random instruction associated to Heap.add(...) method in
				 * peersim.edsim
				 **/
				CommonState.r.nextInt(1 << 8);
				new PSGProcessLauncher(host, host.getName(), startTime, event,
						pid).start();
			} catch (HostNotFoundException e) {
				System.err.println("Host not found");
			}
		}

	}

	// ========================== main method ==================================
	// =====================================================================
	public static void main() throws NativeException, HostNotFoundException {

		String platformfile = PSGPlatform.platformFile();
		System.err.println(platformfile + " loaded");
		String[] arguments = { platformfile, "deployment.xml" };
		Msg.init(arguments);

		/** construct the platform */
		Msg.createEnvironment(arguments[0]);

		PSGPlatform.protocols();

		/** deploy the application **/
		Msg.deployApplication(arguments[1]);

		/** construct the host-node mapping **/
		NodeHost.start();

		/** Process Controller **/
		PSGPlatform.control();
		new PSGProcessController(PSGPlatform.hostList[0],
				PSGPlatform.hostList[0].getName(), null).start();

		/** Load and execute the initializers classes in the configuration file **/
		PSGPlatform.init();

		PSGPlatform.delete("deployment.xml");
		/** execute the simulation. **/
		Msg.run();
	}
}
