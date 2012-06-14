/*
 * Copyright 2012. The SimGrid Team. All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */
package cloud;

import java.util.ArrayList;

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
		int slavesCount = 10;
		
		ArrayList<VM> vms = new ArrayList<VM>();
		
		for (int i = 0; i < slavesCount; i++) {
			Slave slave = new Slave(hosts[i],i);
			slave.start();
			VM vm = new VM(hosts[i],1);
			vm.bind(slave);
			vms.add(vm);
		}
		Msg.info("Launched " + vms.size() + " VMs");
		
		Msg.info("Send a first batch of work to everyone");
		workBatch(slavesCount);
		
		Msg.info("Now suspend all VMs, just for fun");
		for (int i = 0; i < vms.size(); i++) {
			vms.get(i).suspend();
		}
		
		Msg.info("Wait a while");
		waitFor(2);
		
		Msg.info("Enough. Let's resume everybody.");
		for (int i = 0; i < vms.size(); i++) {
			vms.get(i).resume();
		}
		
		Msg.info("Sleep long enough for everyone to be done with previous batch of work");
		waitFor(1000 - Msg.getClock());
		
		Msg.info("Add one more process per VM, and dispatch a batch of work to everyone");
		for (int i = 0; i < vms.size(); i++) {
			VM vm = vms.get(i);
			Slave slave = new Slave(hosts[i],i + slavesCount);
			slave.start();
			vm.bind(slave);
		}
		
		Msg.info("Migrate everyone to the second host.");
		for (int i = 0; i < vms.size(); i++) {
			vms.get(i).migrate(hosts[1]);
		}
		
		Msg.info("Suspend everyone, move them to the third host, and resume them.");
		for (int i = 0; i < vms.size(); i++) {
			VM vm = vms.get(i);
			vm.suspend();
			vm.migrate(hosts[2]);
		}
		
		workBatch(slavesCount * 2);
		
		Msg.info("Let's shut down the simulation. 10 first processes will be shut down cleanly while the second half will forcefully get killed");
		
		for (int i = 0; i < 10; i++) {
			FinalizeTask task = new FinalizeTask(0,0);
			task.send("slave_" + i);
		}
		
		for (int i = 0; i < vms.size(); i++) {
			vms.get(i).shutdown();
		}				
	}
	
	public void workBatch(int slavesCount) throws MsgException {
		for (int i = 0; i < slavesCount; i++) {
			Task task = new Task("Task_" + i, Cloud.task_comp_size, Cloud.task_comm_size);
			task.send("slave_" + i);
		}
	}
}