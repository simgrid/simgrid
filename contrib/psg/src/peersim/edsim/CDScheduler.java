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

import peersim.core.*;
import peersim.cdsim.CDProtocol;
import peersim.config.*;
import peersim.dynamics.NodeInitializer;

/**
 * Schedules the first execution of the cycle based protocol instances in the
 * event driven engine. It implements {@link Control} but it will most often be
 * invoked only once for each protocol as an initializer, since the scheduled
 * events schedule themselves for the consecutive executions (see
 * {@link NextCycleEvent}).
 *
 * <p>
 * All {@link CDProtocol} specifications in the configuration need to contain a
 * {@link Scheduler} specification at least for the step size (see config
 * parameter {@value peersim.core.Scheduler#PAR_STEP} of {@link Scheduler}).
 * This value is used as the cycle length for the corresponding protocol.
 *
 * @see NextCycleEvent
 */
public class CDScheduler implements Control, NodeInitializer {

	// ============================== fields ==============================
	// ====================================================================

	/**
	 * Parameter that is used to define the class that is used to schedule the
	 * next cycle. Its type is (or extends) {@link NextCycleEvent}. Defaults to
	 * {@link NextCycleEvent}.
	 * 
	 * @config
	 */
	private static final String PAR_NEXTC = "nextcycle";

	/**
	 * The protocols that this scheduler schedules for the first execution. It
	 * might contain several protocol names, separated by whitespace. All
	 * protocols will be scheduled based on the common parameter set for this
	 * scheduler and the parameters of the protocol (cycle length). Protocols
	 * are scheduled independently of each other.
	 * 
	 * @config
	 */
	private static final String PAR_PROTOCOL = "protocol";

	/**
	 * If set, it means that the initial execution of the given protocol is
	 * scheduled for a different random time for all nodes. The random time is a
	 * sample between the current time (inclusive) and the cycle length
	 * (exclusive), the latter being specified by the step parameter (see
	 * {@link Scheduler}) of the assigned protocol.
	 * 
	 * @see #execute
	 * @config
	 */
	private static final String PAR_RNDSTART = "randstart";

	/**
	 * Contains the scheduler objects for all {@link CDProtocol}s defined in the
	 * configuration. The length of the array is the number of protocols
	 * defined, but those entries that belong to protocols that are not
	 * {@link CDProtocol}s are null.
	 */
	public static final Scheduler[] sch;

	private final NextCycleEvent[] nce;

	private final int[] pid;

	private final boolean randstart;

	// =============================== initialization ======================
	// =====================================================================

	/**
	 * Loads protocol schedulers for all protocols.
	 */
	static {

		String[] names = Configuration.getNames(Node.PAR_PROT);
		sch = new Scheduler[names.length];
		for (int i = 0; i < names.length; ++i) {
			if (Network.prototype.getProtocol(i) instanceof CDProtocol)
				// with no default values for step to avoid
				// "overscheduling" due to lack of step option.
				sch[i] = new Scheduler(names[i], false);
		}
	}

	// --------------------------------------------------------------------

	/**
	 * Initialization based on configuration parameters.
	 */
	public CDScheduler(String n) {
		String[] prots = Configuration.getString(n + "." + PAR_PROTOCOL).split("\\s");
		pid = new int[prots.length];
		nce = new NextCycleEvent[prots.length];
		for (int i = 0; i < prots.length; ++i) {
			pid[i] = Configuration.lookupPid(prots[i]);
			if (!(Network.prototype.getProtocol(pid[i]) instanceof CDProtocol)) {
				throw new IllegalParameterException(n + "." + PAR_PROTOCOL,
						"Only CDProtocols are accepted here");
			}
			nce[i] = (NextCycleEvent) Configuration.getInstance(n + "."
					+ PAR_NEXTC, new NextCycleEvent(null));
		}
		randstart = Configuration.contains(n + "." + PAR_RNDSTART);
	}

	// ========================== methods ==================================
	// =====================================================================

	/**
	 * Schedules the protocol at all nodes for the first execution adding it to
	 * the priority queue of the event driven simulation. The time of the first
	 * execution is determined by {@link #firstDelay}. The implementation calls
	 * {@link #initialize} for all nodes.
	 * 
	 * @see #initialize
	 */
	public boolean execute() {

		for (int i = 0; i < Network.size(); ++i) {
			initialize(Network.get(i));
		}

		return false;
	}

	// --------------------------------------------------------------------

	/**
	 * Schedules the protocol at given node for the first execution adding it to
	 * the priority queue of the event driven simulation. The time of the first
	 * execution is determined by a reference point in time and
	 * {@link #firstDelay}, which defines the delay from the reference point.
	 * The reference point is the maximum of the current time, and the value of
	 * parameter {@value peersim.core.Scheduler#PAR_FROM} of the protocol being
	 * scheduled. If the calculated time of the first execution is not valid
	 * according to the schedule of the protocol then no execution is scheduled
	 * for that protocol.
	 * <p>
	 * A final note: for performance reasons, the recommended practice is not to
	 * use parameter {@value peersim.core.Scheduler#PAR_FROM} in protocols, but
	 * to schedule {@link CDScheduler} itself for the desired time, whenever
	 * possible (e.g., it is not possible if {@link CDScheduler} is used as a
	 * {@link NodeInitializer}).
	 */
	public void initialize(Node n) {
		/*
		 * XXX If "from" is not the current time and this is used as a control
		 * (not node initializer) then we dump _lots_ of events in the queue
		 * that are just stored there until "from" comes. This reduces
		 * performance, and should be fixed. When fixed, the final comment can
		 * be removed from the docs.
		 */

		final long time = CommonState.getTime();
		for (int i = 0; i < pid.length; ++i) {
			Object nceclone = null;
			try {
				nceclone = nce[i].clone();
			} catch (CloneNotSupportedException e) {
			} // cannot possibly happen

			final long delay = firstDelay(sch[pid[i]].step);
			final long nexttime = Math.max(time, sch[pid[i]].from) + delay;
			if (nexttime < sch[pid[i]].until)
				EDSimulator.add(nexttime - time, nceclone, n, pid[i]);
		}
	}

	// --------------------------------------------------------------------

	/**
	 * Returns the time (through giving the delay from the current time) when
	 * this even is first executed. If {@value #PAR_RNDSTART} is not set, it
	 * returns zero, otherwise a random value between 0, inclusive, and
	 * cyclelength, exclusive.
	 * 
	 * @param cyclelength
	 *            The cycle length of the cycle based protocol for which this
	 *            method is called
	 */
	protected long firstDelay(long cyclelength) {

		if (randstart)
			return CommonState.r.nextLong(cyclelength);
		else
			return 0;
	}
}
