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

package peersim.edsim;

import java.util.Arrays;

import peersim.Simulator;
import peersim.config.*;
import peersim.core.*;
import psgsim.PSGSimulator;

/**
 * Event-driven simulator engine. It is a fully static singleton class. For an
 * event driven simulation the configuration has to describe a set of
 * {@link Protocol}s, a set of {@link Control}s and their ordering and a set of
 * initializers and their ordering. See parameters {@value #PAR_INIT},
 * {@value #PAR_CTRL}.
 * <p>
 * One experiment run by {@link #nextExperiment} works as follows. First the
 * initializers are run in the specified order. Then the first execution of all
 * specified controls is scheduled in the event queue. This scheduling is
 * defined by the {@link Scheduler} parameters of each control component. After
 * this, the first event is taken from the event queue. If the event wraps a
 * control, the control is executed, otherwise the event is delivered to the
 * destination protocol, that must implement {@link EDProtocol}. This is
 * iterated while the current time is less than {@value #PAR_ENDTIME} or the
 * queue becomes empty. If more control events fall at the same time point, then
 * the order given in the configuration is respected. If more non-control events
 * fall at the same time point, they are processed in a random order.
 * <p>
 * The engine also provides the interface to add events to the queue. Note that
 * this engine does not explicitly run the protocols. In all cases at least one
 * control or initializer has to be defined that sends event(s) to protocols.
 * <p>
 * Controls can be scheduled (using the {@link Scheduler} parameters in the
 * configuration) to run after the experiment has finished. That is, each
 * experiment is finished by running the controls that are scheduled to be run
 * after the experiment.
 * <p>
 * Any control can interrupt an experiment at any time it is executed by
 * returning true in method {@link Control#execute}. However, the controls
 * scheduled to run after the experiment are still executed completely,
 * irrespective of their return value and even if the experiment was
 * interrupted.
 * <p>
 * {@link CDScheduler} has to be mentioned that is a control that can bridge the
 * gap between {@link peersim.cdsim} and the event driven engine. It can wrap
 * {@link peersim.cdsim.CDProtocol} appropriately so that the execution of the
 * cycles are scheduled in configurable ways for each node individually. In some
 * cases this can add a more fine-grained control and more realism to
 * {@link peersim.cdsim.CDProtocol} simulations, at the cost of some loss in
 * performance.
 * <p>
 * When protocols at different nodes send messages to each other, they might
 * want to use a model of the transport layer so that in the simulation message
 * delay and message omissions can be modeled in a modular way. This
 * functionality is implemented in package {@link peersim.transport}.
 * 
 * @see Configuration
 */
public class EDSimulator {

	// ---------------------------------------------------------------------
	// Parameters
	// ---------------------------------------------------------------------

	/**
	 * The ending time for simulation. Only events that have a strictly smaller
	 * value are executed. It must be positive. Although in principle negative
	 * timestamps could be allowed, we assume time will be positive.
	 * 
	 * @config
	 */
	public static final String PAR_ENDTIME = "simulation.endtime";

	/**
	 * This parameter specifies how often the simulator should log the current
	 * time on the standard error. The time is logged only if there were events
	 * in the respective interval, and only the time of some actual event is
	 * printed. That is, the actual log is not guaranteed to happen in identical
	 * intervals of time. It is merely a way of seeing whether the simulation
	 * progresses and how fast...
	 * 
	 * @config
	 */
	private static final String PAR_LOGTIME = "simulation.logtime";

	/**
	 * This parameter specifies the event queue to be used. It must be an
	 * implementation of interface {@link PriorityQ}. If it is not defined, the
	 * internal implementation is used.
	 * 
	 * @config
	 */
	private static final String PAR_PQ = "simulation.eventqueue";

	/**
	 * This is the prefix for initializers. These have to be of type
	 * {@link Control}. They are run at the beginning of each experiment, in the
	 * order specified by the configuration.
	 * 
	 * @see Configuration
	 * @config
	 * @config
	 */
	private static final String PAR_INIT = "init";

	/**
	 * This is the prefix for {@link Control} components. They are run at the
	 * time points defined by the {@link Scheduler} associated to them. If some
	 * controls have to be executed at the same time point, they are executed in
	 * the order specified in the configuration.
	 * 
	 * @see Configuration
	 * @config
	 */
	private static final String PAR_CTRL = "control";

	// ---------------------------------------------------------------------
	// Fields
	// ---------------------------------------------------------------------

	/** Maximum time for simulation */
	private static long endtime;

	/** Log time */
	private static long logtime;

	/** holds the modifiers of this simulation */
	private static Control[] controls = null;

	/** Holds the control schedulers of this simulation */
	private static Scheduler[] ctrlSchedules = null;

	/** Ordered list of events (heap) */
	private static PriorityQ heap = null;

	private static long nextlog = 0;

	// =============== initialization ======================================
	// =====================================================================

	/** to prevent construction */
	private EDSimulator() {
	}

	// ---------------------------------------------------------------------
	// Private methods
	// ---------------------------------------------------------------------

	/**
	 * Load and run initializers.
	 */
	private static void runInitializers() {

		Object[] inits = Configuration.getInstanceArray(PAR_INIT);
		String names[] = Configuration.getNames(PAR_INIT);

		for (int i = 0; i < inits.length; ++i) {

			System.err.println("- Running initializer " + names[i] + ": "
					+ inits[i].getClass());
			((Control) inits[i]).execute();
		}
	}

	// --------------------------------------------------------------------

	private static void scheduleControls() {
		// load controls
		String[] names = Configuration.getNames(PAR_CTRL);
		controls = new Control[names.length];
		ctrlSchedules = new Scheduler[names.length];
		for (int i = 0; i < names.length; ++i) {
			controls[i] = (Control) Configuration.getInstance(names[i]);
			ctrlSchedules[i] = new Scheduler(names[i], false);
		}
		System.err.println("EDSimulator: loaded controls "
				+ Arrays.asList(names));

		// Schedule controls execution
		if (controls.length > heap.maxPriority() + 1)
			throw new IllegalArgumentException("Too many control objects");
		for (int i = 0; i < controls.length; i++) {
			new ControlEvent(controls[i], ctrlSchedules[i], i);
		}
	}

	// ---------------------------------------------------------------------

	/**
	 * Adds a new event to be scheduled, specifying the number of time units of
	 * delay, and the execution order parameter.
	 * 
	 * @param time
	 *            The actual time at which the next event should be scheduled.
	 * @param order
	 *            The index used to specify the order in which control events
	 *            should be executed, if they happen to be at the same time,
	 *            which is typically the case.
	 * @param event
	 *            The control event
	 */
	static void addControlEvent(long time, int order, ControlEvent event) {
		// we don't check whether time is negative or in the past: we trust
		// the caller, which must be from this package
		if (time >= endtime)
			return;
		heap.add(time, event, null, (byte) 0, order);
	}

	// ---------------------------------------------------------------------

	/**
	 * This method is used to check whether the current configuration can be
	 * used for event driven simulations. It checks for the existence of config
	 * parameter {@value #PAR_ENDTIME}.
	 */
	public static final boolean isConfigurationEventDriven() {
		return Configuration.contains(PAR_ENDTIME);
	}

	// ---------------------------------------------------------------------

	/**
	 * Execute and remove the next event from the ordered event list.
	 * 
	 * @return true if the execution should be stopped.
	 */
	private static boolean executeNext() {
		PriorityQ.Event ev = heap.removeFirst();
		if (ev == null) {
			System.err.println("EDSimulator: queue is empty, quitting"
					+ " at time " + CommonState.getTime());
			return true;
		}

		long time = ev.time;

		if (time >= nextlog) {
			// System.err.println("Current time: " + time);
			// seemingly complicated: to prevent overflow
			while (time - nextlog >= logtime)
				nextlog += logtime;
			if (endtime - nextlog >= logtime)
				nextlog += logtime;
			else
				nextlog = endtime;
		}
		if (time >= endtime) {
			System.err.println("EDSimulator: reached end time, quitting,"
					+ " leaving " + heap.size()
					+ " unprocessed events in the queue");
			return true;
		}

		CommonState.setTime(time);
		int pid = ev.pid;
			if (ev.node == null) {
			// might be control event; handled through a special method
			ControlEvent ctrl = null;
			try {
				ctrl = (ControlEvent) ev.event;
			} catch (ClassCastException e) {
				throw new RuntimeException(
						"No destination specified (null) for event " + ev);
			}
			return ctrl.execute();
		} else if (ev.node != Network.prototype && ev.node.isUp()) {
			CommonState.setPid(pid);
			CommonState.setNode(ev.node);
			if (ev.event instanceof NextCycleEvent) {
				NextCycleEvent nce = (NextCycleEvent) ev.event;
				nce.execute();
			} else {
				EDProtocol prot = null;
				try {
					prot = (EDProtocol) ev.node.getProtocol(pid);
					// System.out.println("prot "+prot.getClass().getName());
				} catch (ClassCastException e) {
					e.printStackTrace();
					throw new IllegalArgumentException("Protocol "
							+ Configuration.lookupPid(pid)
							+ " does not implement EDProtocol; "
							+ ev.event.getClass());
				}
				prot.processEvent(ev.node, pid, ev.event);
			}
		}

		return false;
	}

	// ---------------------------------------------------------------------
	// Public methods
	// ---------------------------------------------------------------------

	/**
	 * Runs an experiment, resetting everything except the random seed.
	 */
	public static void nextExperiment() {
		// Reading parameter
		if (Configuration.contains(PAR_PQ))
			heap = (PriorityQ) Configuration.getInstance(PAR_PQ);
		else
			heap = new Heap();
		endtime = Configuration.getLong(PAR_ENDTIME);
		if (CommonState.getEndTime() < 0) // not initialized yet
			CommonState.setEndTime(endtime);
		if (heap.maxTime() < endtime)
			throw new IllegalParameterException(PAR_ENDTIME,
					"End time is too large: configured event queue only"
							+ " supports " + heap.maxTime());
		logtime = Configuration.getLong(PAR_LOGTIME, Long.MAX_VALUE);

		// initialization
		System.err.println("EDSimulator: resetting");
		CommonState.setPhase(CommonState.PHASE_UNKNOWN);
		CommonState.setTime(0); // needed here
		controls = null;
		ctrlSchedules = null;
		nextlog = 0;
		Network.reset();
		System.err.println("EDSimulator: running initializers");
		runInitializers();
		scheduleControls();
			// Perform the actual simulation; executeNext() will tell when to
		// stop.
		boolean exit = false;
		while (!exit) {
			exit = executeNext();
		}

		// analysis after the simulation
		CommonState.setPhase(CommonState.POST_SIMULATION);
		for (int j = 0; j < controls.length; ++j) {
			if (ctrlSchedules[j].fin)
				controls[j].execute();
		}

	}

	// ---------------------------------------------------------------------

	/**
	 * Adds a new event to be scheduled, specifying the number of time units of
	 * delay, and the node and the protocol identifier to which the event will
	 * be delivered.
	 * 
	 * @param delay
	 *            The number of time units before the event is scheduled. Has to
	 *            be non-negative.
	 * @param event
	 *            The object associated to this event
	 * @param node
	 *            The node associated to the event.
	 * @param pid
	 *            The identifier of the protocol to which the event will be
	 *            delivered
	 */
	public static void add(long delay, Object event, Node node, int pid) {
		//if (event instanceof NextCycleEvent)
			//System.err.println("************* edsim delay="+delay +" pid="+pid+" event="+event+" time="+CommonState.getTime());
		if (Simulator.getSimID() == 2){
			PSGSimulator.add(delay, event, node, pid);
		}
		
		else {
			if (delay < 0)
				throw new IllegalArgumentException("Protocol "
						+ node.getProtocol(pid) + " is trying to add event "
						+ event + " with a negative delay: " + delay);
			if (pid > Byte.MAX_VALUE)
				throw new IllegalArgumentException(
						"This version does not support more than "
								+ Byte.MAX_VALUE + " protocols");

			long time = CommonState.getTime();
			if (endtime - time > delay) // check like this to deal with overflow
				heap.add(time + delay, event, node, (byte) pid);
		}
	}

}
