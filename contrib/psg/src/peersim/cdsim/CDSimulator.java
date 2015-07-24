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

package peersim.cdsim;

import java.util.*;
import peersim.config.*;
import peersim.core.*;

/**
 * This is the cycle driven simulation engine. It is a fully static
 * singleton class. For a cycle driven simulation the configuration can
 * describe a set of {@link Protocol}s, and their ordering, a set of
 * {@link Control}s and their ordering and a set of initializers and their
 * ordering. See parameters {@value #PAR_INIT}, {@value #PAR_CTRL}. Out
 * of the set of protocols, this engine only executes the ones that
 * implement the {@link CDProtocol} interface.
 * <p>
 * One experiment run by {@link #nextExperiment} works as follows. First
 * the initializers are run in the specified order, then the following is
 * iterated {@value #PAR_CYCLES} times: If {@value #PAR_NOMAIN} is
 * specified, then simply the controls specified in the configuration are
 * run in the specified order. If {@value #PAR_NOMAIN} is not specified,
 * then the controls in the configuration are run in the specified order,
 * followed by the execution of {@link FullNextCycle}.
 * <p>
 * All components (controls and protocols) can have configuration
 * parameters that control their scheduling (see {@link Scheduler}). This
 * way they can skip cycles, start from a specified cycle, etc. As a
 * special case, components can be scheduled to run after the last cycle.
 * That is, each experiment is finished by running the controls that are
 * scheduled after the last cycle.
 * <p>
 * Finally, any control can interrupt an experiment at any time it is
 * executed by returning true in method {@link Control#execute}. However,
 * the controls scheduled to run after the last cycle are still executed
 * completely, irrespective of their return value and even if the
 * experiment was interrupted.
 * @see Configuration
 */
public class CDSimulator
{

// ============== fields ===============================================
// =====================================================================

/**
 * Parameter representing the maximum number of cycles to be performed
 * @config
 */
public static final String PAR_CYCLES = "simulation.cycles";

/**
 * This option is only for experts. It switches off the main cycle that
 * calls the cycle driven protocols. When you switch this off, you need to
 * control the execution of the protocols by configuring controls that do
 * the job (e.g., {@link FullNextCycle}, {@link NextCycle}). It's there for
 * people who want maximal flexibility for their hacks.
 * @config
 */
private static final String PAR_NOMAIN = "simulation.nodefaultcycle";

/**
 * This is the prefix for initializers. These have to be of type
 * {@link Control}. They are run at the beginning of each experiment, in
 * the order specified by the configuration.
 * @see Configuration
 * @config
 */
private static final String PAR_INIT = "init";

/**
 * This is the prefix for controls. These have to be of type
 * {@link Control}. They are run before each cycle, in the order specified
 * by the configuration.
 * @see Configuration
 * @config
 */
private static final String PAR_CTRL = "control";

// --------------------------------------------------------------------

/** The maximum number of cycles to be performed */
private static int cycles;

/** holds the modifiers of this simulation */
private static Control[] controls = null;

/** Holds the control schedulers of this simulation */
private static Scheduler[] ctrlSchedules = null;

// =============== initialization ======================================
// =====================================================================

/** to prevent construction */
private CDSimulator()
{
}

// =============== private methods =====================================
// =====================================================================

/**
 * Load and run initializers.
 */
private static void runInitializers()
{

	Object[] inits = Configuration.getInstanceArray(PAR_INIT);
	String names[] = Configuration.getNames(PAR_INIT);

	for (int i = 0; i < inits.length; ++i) {
		System.err.println("- Running initializer " + names[i] + ": "
				+ inits[i].getClass());
		((Control) inits[i]).execute();
	}
}

// --------------------------------------------------------------------

private static String[] loadControls()
{

	boolean nomaincycle = Configuration.contains(PAR_NOMAIN);
	String[] names = Configuration.getNames(PAR_CTRL);
	if (nomaincycle) {
		controls = new Control[names.length];
		ctrlSchedules = new Scheduler[names.length];
	} else {
		// provide for an extra control that handles the main cycle
		controls = new Control[names.length + 1];
		ctrlSchedules = new Scheduler[names.length + 1];
		// calling with a prefix that cannot exist
		controls[names.length] = new FullNextCycle(" ");
		ctrlSchedules[names.length] = new Scheduler(" ");
	}
	for (int i = 0; i < names.length; ++i) {
		controls[i] = (Control) Configuration.getInstance(names[i]);
		ctrlSchedules[i] = new Scheduler(names[i]);
	}
	System.err.println("CDSimulator: loaded controls " + Arrays.asList(names));
	return names;
}

// ---------------------------------------------------------------------

/**
 * This method is used to check whether the current configuration can be
 * used for cycle-driven simulations. It checks for the existence of
 * configuration parameter {@value #PAR_CYCLES}.
 */
public static final boolean isConfigurationCycleDriven()
{
	return Configuration.contains(PAR_CYCLES);
}

// ---------------------------------------------------------------------

/**
 * Runs an experiment, resetting everything except the random seed. 
 */
public static final void nextExperiment()
{

	// Reading parameter
	cycles = Configuration.getInt(PAR_CYCLES);
	if (CommonState.getEndTime() < 0) // not initialized yet
		CDState.setEndTime(cycles);

	// initialization
	CDState.setCycle(0);
	CDState.setPhase(CDState.PHASE_UNKNOWN);
	System.err.println("CDSimulator: resetting");
	controls = null;
	ctrlSchedules = null;
	Network.reset();
	System.err.println("CDSimulator: running initializers");
	runInitializers();

	// main cycle
	loadControls();

	System.err.println("CDSimulator: starting simulation");
	for (int i = 0; i < cycles; ++i) {
		CDState.setCycle(i);

		boolean stop = false;
		for (int j = 0; j < controls.length; ++j) {
			if (ctrlSchedules[j].active(i))
				stop = stop || controls[j].execute();
		}
		if (stop)
			break;
		//System.err.println("CDSimulator: cycle " + i + " DONE");
	}

	CDState.setPhase(CDState.POST_SIMULATION);

	// analysis after the simulation
	for (int j = 0; j < controls.length; ++j) {
		if (ctrlSchedules[j].fin)
			controls[j].execute();
	}
}

}
