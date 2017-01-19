/* Copyright (c) 2016. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package energy.pstate;

import org.simgrid.msg.Host;
import org.simgrid.msg.HostFailureException;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.Msg;
import org.simgrid.msg.NativeException;
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
			int new_peak_index=2;
			Host host = getHost();

			int nb = host.getPstatesCount();
			Msg.info("Count of Processor states="+ nb);

			double current_peak = host.getCurrentPowerPeak();
			Msg.info("Current power peak=" + current_peak);

			// Run a task
			Task task1 = new Task("t1", workload, 0);
			task1.execute();

			double task_time = Msg.getClock();
			Msg.info("Task1 simulation time: "+ task_time);

			// Change power peak
			if ((new_peak_index >= nb) || (new_peak_index < 0)){
				Msg.info("Cannot set pstate "+new_peak_index+"%d, host supports only "+nb+" pstates.");
				return;
			}

			double peak_at = host.getPowerPeakAt(new_peak_index);
			Msg.info("Changing power peak value to "+peak_at+" (at index "+new_peak_index+")");

			host.setPstate(new_peak_index);

			current_peak = host.getCurrentPowerPeak();
			Msg.info("Current power peak="+ current_peak);

			// Run a second task
			task1 = new Task("t1", workload, 0);

			task_time = Msg.getClock() - task_time;
			Msg.info("Task2 simulation time: "+ task_time);

			// Verify the default pstate is set to 0
			host = Host.getByName("MyHost2");
			int nb2 = host.getPstatesCount();
			Msg.info("Count of Processor states="+ nb2);

			double current_peak2 = host.getCurrentPowerPeak();
			Msg.info("Current power peak=" + current_peak2);
			return ;
		}
	}

	PstateRunner(Host host, String name, String[] args) throws HostNotFoundException, NativeException  {
		super(host, name, args);
	}

	@Override
	public void main(String[] strings) throws HostNotFoundException, HostFailureException {

	    new DVFS (Host.getByName("MyHost1"), "dvfs_test").start(); 
	    new DVFS (Host.getByName("MyHost2"), "dvfs_test").start(); 

	}
}
