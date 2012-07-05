/*
 * 2012. The SimGrid Team. All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */
package mutualExclusion.centralized;
import org.simgrid.msg.Task;

public class RequestTask extends Task {
	String from;
	public RequestTask(String name) {
		super();
		from=name;
	}
}
