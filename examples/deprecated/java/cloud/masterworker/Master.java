/* Copyright (c) 2012-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package cloud.masterworker;

import org.simgrid.msg.Host;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.Process;
import org.simgrid.msg.Task;
import org.simgrid.msg.VM;

public class Master extends Process {
	private Host[] hosts;

	public Master(Host host, String name, Host[] hosts) {
		super(host,name,null);
		this.hosts = hosts;
	}

	public void main(String[] args) throws MsgException {
		int workersCount = Main.NHOSTS;

		for (int step = 1; step <= Main.NSTEPS ; step++) {
			// Create one VM per host and bind a process inside each one.
			for (int i = 0; i < workersCount; i++) {
				Msg.verb("create VM0-s"+step+"-"+i);
				VM vm = new VM(hosts[i+1],"VM0-s"+step+"-"+i);
				vm.start();
				Worker worker= new Worker(vm,"WK:"+step+":"+ i);
				Msg.verb("Put Worker "+worker.getName()+ " on "+vm.getName());
				worker.start();
			}
			VM[] vms = VM.all();

			Msg.info("Launched " + vms.length + " VMs");

			Msg.info("Send some work to everyone");
			workBatch(workersCount,"WK:"+step+":");

			Msg.info("Suspend all VMs, wait a while, resume them, migrate them and shut them down.");
			for (VM vm : vms) {
				Msg.verb("Suspend "+vm.getName());
				vm.suspend();
			}

			Msg.verb("Wait a while, and resume all VMs.");
			waitFor(2);
			for (VM vm : vms)
				vm.resume();


			Msg.verb("Sleep long enough for everyone to be done with previous batch of work");
			waitFor(1000*step - Msg.getClock());

			Msg.verb("Migrate everyone to "+hosts[3].getName());
			for (VM vm : vms) {
				Msg.verb("Migrate "+vm.getName()+" to "+hosts[3].getName());
				vm.migrate(hosts[3]);
			}

			Msg.verb("Let's kill everyone.");

			for (VM vm : vms)
				vm.destroy();
			Msg.info("XXXXXXXXXXXXXXX Step "+step+" done.");
		}
	}

	public void workBatch(int workersCount, String nameRoot) throws MsgException {
		for (int i = 0; i < workersCount; i++) {
			Task task = new Task("Task "+nameRoot + i, Main.TASK_COMP_SIZE, Main.TASK_COMM_SIZE);
			Msg.verb("Sending to "+ nameRoot + i);
			task.send(nameRoot + i);
		}
	}
}
