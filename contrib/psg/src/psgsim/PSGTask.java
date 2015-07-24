package psgsim;

import org.simgrid.msg.HostFailureException;
import org.simgrid.msg.Task;
import org.simgrid.msg.TimeoutException;
import org.simgrid.msg.TransferFailureException;

/**
 * The PSGTask includes all the parameters of sending a message as the size, the
 * compute duration and the protocol identifier.
 * 
 * @author Khaled Baati 28/10/2014
 * @version version 1.1
 */
public class PSGTask extends org.simgrid.msg.Task {
	/** The Message to be sent **/
	private Object event;
	/** The protocol identifier **/
	private int pid;

	/**
	 * Construct a new task to be sent.
	 * 
	 * @param name
	 *            The name of task
	 * @param computeDuration
	 *            The compute duration
	 * 
	 * @param messageSize
	 *            The size of the message
	 * @param event
	 *            The message to be sent
	 * @param pid
	 *            The protocol identifier
	 */
	public PSGTask(String name, double computeDuration, double messageSize,
			Object event, int pid) {
		super(name, computeDuration, messageSize);
		this.event = event;
		this.pid = pid;
	}

	/**
	 * 
	 * @return the protocol identifier
	 */
	public int getPid() {
		return pid;
	}

	/**
	 * 
	 * @return the message
	 */
	public Object getEvent() {
		return event;
	}

	/**
	 * Retrieves next task on the mailbox identified by the specified name (wait
	 * at most timeout seconds)
	 * 
	 * @param mailbox
	 *            the mailbox on where to receive the task
	 * @param timeout
	 *            the timeout to wait for receiving the task
	 * @return the task
	 */
	public static Task receive(String mailbox, double timeout) {
		double time = PSGPlatform.getClock();
		if (time + timeout > PSGPlatform.getClock()) {
			try {
				return receive(mailbox, timeout, null);
			} catch (TimeoutException e) {
			} catch (TransferFailureException e) {
				e.printStackTrace();
			} catch (HostFailureException e) {
				e.printStackTrace();
			}
		}
		return null;

	}

}
