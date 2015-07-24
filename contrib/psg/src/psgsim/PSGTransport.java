package psgsim;

import java.util.LinkedHashMap;
import java.util.Map;

import peersim.config.Configuration;
import peersim.config.IllegalParameterException;
import peersim.core.CommonState;
import peersim.core.Node;
import peersim.transport.Transport;

/**
 * PSGTransport is the transport layer. it is responsible for sending messages.
 * 
 * @author Khaled Baati 28/10/2014
 * @version 1.1
 */
public class PSGTransport implements Transport {

	private static double computeDuration = 0;
	private PSGTask task;
	private static Map<PSGTask, String> taskToSend = new LinkedHashMap<PSGTask, String>();

	/**
	 * String name of the parameter used to configure the minimum latency. * @config
	 */
	private static final String PAR_MINDELAY = "mindelay";

	/**
	 * String name of the parameter used to configure the maximum latency.
	 * Defaults to {@value #PAR_MINDELAY}, which results in a constant delay.
	 * 
	 * @config
	 */
	private static final String PAR_MAXDELAY = "maxdelay";

	/** Minimum delay for message sending */
	private long min;
	/** Maximum delay for message sending */
	private long max;

	/**
	 * Difference between the max and min delay plus one. That is, max delay is
	 * min+range-1.
	 */
	private long range;

	public PSGTransport() {

	}

	public PSGTransport(String prefix) {
		min = Configuration.getLong(prefix + "." + PAR_MINDELAY);
		max = Configuration.getLong(prefix + "." + PAR_MAXDELAY, min);
		if (max < min)
			throw new IllegalParameterException(prefix + "." + PAR_MAXDELAY,
					"The maximum latency cannot be smaller than the minimum latency");
		range = max - min + 1;
	}

	/**
	 * Returns <code>this</code>. This way only one instance exists in the
	 * system that is linked from all the nodes. This is because this protocol
	 * has no node specific state.
	 */
	public Object clone() {
		return this;
	}

	@Override
	public void send(Node src, Node dest, Object msg, int pid) {
		double commSizeLat = 0;
		/**
		 * random instruction associated to UniformRandomTransport.send(...)
		 * method in peersim.transport
		 **/
		long delay = (range == 1 ? min : min + CommonState.r.nextLong(range));
		CommonState.r.nextInt(1 << 8);
		if (msg instanceof Sizable) {
			commSizeLat = ((Sizable) msg).getSize();
		}

		task = new PSGTask("task sender_" + src.getID(), computeDuration,
				commSizeLat, msg, pid);
		taskToSend.put(this.task, NodeHost.getHost(dest).getName());

	}

	/**
	 * Process for sending all messages in the queue.
	 */
	public static void flush() {
		Map<PSGTask, String> taskToSendCopy = new LinkedHashMap<PSGTask, String>();
		for (Map.Entry<PSGTask, String> entry : taskToSend.entrySet()) {
			taskToSendCopy.put(entry.getKey(), entry.getValue());
		}
		taskToSend.clear();
		for (Map.Entry<PSGTask, String> entry : taskToSendCopy.entrySet()) {
			PSGTask task = entry.getKey();
			String dest = entry.getValue();
			task.dsend(dest);
		}
		taskToSendCopy.clear();

	}

	@Override
	public long getLatency(Node src, Node dest) {
		/**
		 * random instruction associated to
		 * UniformRandomTransport.getLatency(...) method in peersim.transport
		 **/
		return (range == 1 ? min : min + CommonState.r.nextLong(range));
	}
}
