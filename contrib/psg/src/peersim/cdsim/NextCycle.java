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

import peersim.config.*;
import peersim.core.*;

/**
* It generalizes its superclass so that the list of protocols to run can
* be specified. The superclass ({@link FullNextCycle}) always runs all the
* {@link CDProtocol}s.
*/
public class NextCycle extends FullNextCycle {


// ============== fields ===============================================
// =====================================================================


/**
* Gives the list of protocols (whitespace separated) that need to be
* iterated over.
* @config
*/
private static final String PAR_PROTS = "protocol";

private final int[] pids;


// =============== initialization ======================================
// =====================================================================

/**
* reads configuration parameters and the {@link Scheduler}s.
*/
public NextCycle(String prefix) {
	
	super(prefix);
	
	String prots = Configuration.getString(prefix+"."+PAR_PROTS);
	String[] protnames = prots.split("\\s");
	pids = new int[protnames.length];
	for(int i=0; i<protnames.length; ++i)
	{
		pids[i] = Configuration.lookupPid(protnames[i]);
	}
}

// =============== methods =============================================
// =====================================================================

/** 
 * Execute the configured protocols on all nodes.
 * It works exactly as {@link FullNextCycle#execute}, only just the configured
 * protocols are iterated over.
 */
public boolean execute() {

	final int cycle=CDState.getCycle();
	if( shuffle ) rperm.reset( Network.size() );
	for(int j=0; j<Network.size(); ++j)
	{
		Node node = null;
		if( getpair_rand )
			node = Network.get(CDState.r.nextInt(Network.size()));
		else if( shuffle )
			node = Network.get(rperm.next());
		else
			node = Network.get(j);
		if( !node.isUp() ) continue; 
		CDState.setNode(node);
		CDState.setCycleT(j);
		for(int pid: pids)
		{
			// Check if the protocol should be executed, given the
			// associated scheduler.
			if (!protSchedules[pid].active(cycle))
				continue;
				
			CDState.setPid(pid);
			Protocol protocol = node.getProtocol(pid);
			if( protocol instanceof CDProtocol )
			{
				((CDProtocol)protocol).nextCycle(node, pid);
				if( !node.isUp() ) break;
			}
		}
	}

	return false;
}

}


