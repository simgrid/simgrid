/* Copyright (c) 2016-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package energy.pstate;

import org.simgrid.msg.Host;
import org.simgrid.msg.HostFailureException;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Process;
import org.simgrid.msg.Task;
import org.simgrid.msg.TaskCancelledException;

/* This class is a process in charge of running the test. It creates and starts the VMs, and fork processes within VMs */
public class PstateRunner extends Process {

	public class DVFS extends Process {
		public  DVFS (Host host, String name) {
			super(host, name);
		}

		@Override
		public void main(String[] strings) throws HostNotFoundException, HostFailureException, TaskCancelledException {
			double workload = 100E6;
			int newPstate = 2;
			Host host = getHost();

			int nb = host.getPstatesCount();
			Msg.info("Count of Processor states="+ nb);

			double currentPeak = host.getCurrentPowerPeak();
			Msg.info("Current power peak=" + currentPeak);

			// Run a task
			Task task1 = new Task("t1", workload, 0);
			task1.execute();

			double taskTime = Msg.getClock();
			Msg.info("Task1 simulation time: "+ taskTime);

			// Change power peak
			if ((newPstate >= nb) || (newPstate < 0)){
				Msg.info("Cannot set pstate "+newPstate+"%d, host supports only "+nb+" pstates.");
				return;
			}

			double peakAt = host.getPowerPeakAt(newPstate);
			Msg.info("Changing power peak value to "+peakAt+" (at index "+newPstate+")");

			host.setPstate(newPstate);

			currentPeak = host.getCurrentPowerPeak();
			Msg.info("Current power peak="+ currentPeak);

			// Run a second task
			new Task("t1", workload, 0).execute();

			taskTime = Msg.getClock() - taskTime;
			Msg.info("Task2 simulation time: "+ taskTime);

			// Verify the default pstate is set to 0
			host = Host.getByName("MyHost2");
			int nb2 = host.getPstatesCount();
			Msg.info("Count of Processor states="+ nb2);

			double currentPeak2 = host.getCurrentPowerPeak();
			Msg.info("Current power peak=" + currentPeak2);
		}
	}

	PstateRunner(Host host, String name, String[] args) {
		super(host, name, args);
	}

	@Override
	public void main(String[] strings) throws HostNotFoundException, HostFailureException {

	    new DVFS (Host.getByName("MyHost1"), "dvfs_test").start();
	    new DVFS (Host.getByName("MyHost2"), "dvfs_test").start();

	}
}
