/* Copyright (c) 2016-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package app.tokenring;
import org.simgrid.msg.Host;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Process;

class Main {
	private Main() {
		/* This is just to prevent anyone from creating an instance of this singleton */
		throw new IllegalAccessError("Utility class");
	}

	public static void main(String[] args) {
		Msg.init(args);

		String platform = "../platforms/small_platform.xml";
		if(args.length >= 1)
			platform = args[0];
		Msg.createEnvironment(platform);

		Host[] hosts = Host.all();
		for (int rank = 0; rank < hosts.length; rank++) {
			Process proc = new RelayRunner(hosts[rank], Integer.toString(rank),  null);
			proc.start();
		}
		Msg.info("Number of hosts '"+hosts.length+"'");
		Msg.run();

		Msg.info("Simulation time " + Msg.getClock());
	}
}
