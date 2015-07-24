/*
 * Copyright (c) 2003 The BISON Project
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

package example.edaggregation;

import peersim.vector.SingleValueHolder;
import peersim.config.*;
import peersim.core.*;
import peersim.transport.Transport;
import peersim.cdsim.CDProtocol;
import peersim.edsim.EDProtocol;

/**
 * Event driven version of epidemic averaging.
 */
public class AverageED extends SingleValueHolder implements CDProtocol,
		EDProtocol {

	// --------------------------------------------------------------------------
	// Initialization
	// --------------------------------------------------------------------------

	/**
	 * @param prefix
	 *            string prefix for config properties
	 */
	public AverageED(String prefix) {
		super(prefix);
	}

	// --------------------------------------------------------------------------
	// methods
	// --------------------------------------------------------------------------

	/**
	 * This is the standard method the define periodic activity. The frequency
	 * of execution of this method is defined by a
	 * {@link peersim.edsim.CDScheduler} component in the configuration.
	 */
	public void nextCycle(Node node, int pid) {
		Linkable linkable = (Linkable) node.getProtocol(FastConfig
				.getLinkable(pid));
		if (linkable.degree() > 0) {
			int degree=linkable.degree();
			int i=CommonState.r.nextInt(degree);		
			Node peern = linkable.getNeighbor(i);
			System.out.println("Pid of the protocol: "+pid);
			System.out.println("Time="+CommonState.getTime()+" degree="+degree+" i="+i+" peernID="+peern.getID()+" peernIndex="+peern.getIndex());
			if (!peern.isUp())
				return;
			AverageMessage ob=new AverageMessage(value, node);
			System.out.println("NextCycle\t"+"\t Time: " + CommonState.getTime()+ "\t src: " + node.getID() + "\t dst: " + peern.getID()+"\t msg:"+ob.value);
			((Transport) node.getProtocol(FastConfig.getTransport(pid))).send(
					node, peern, ob, pid);
		}
	}

	// --------------------------------------------------------------------------

	/**
	 * This is the standard method to define to process incoming messages.
	 */
	public void processEvent(Node node, int pid, Object event) {

		AverageMessage aem = (AverageMessage) event;

		AverageMessage ob=null;
		if (aem.sender != null){
			System.out.println("ProcessEventR\t"+"\t Time: " + CommonState.getTime() + "\t src: " + aem.sender.getID() + "\t dst: " + node.getID()+"\t msg:"+aem.value);
			ob=new AverageMessage(value, null);
			System.out.println("ProcessEventS\t"+"\t Time: " + CommonState.getTime()+ "\t src: " + node.getID() + "\t dst: " + aem.sender.getID()+"\t msg:"+ob.value);
			((Transport) node.getProtocol(FastConfig.getTransport(pid))).send(
					node, aem.sender, ob, pid);
	} else {
		System.out.println("ProcessEventR\t"+"\t Time: " +CommonState.getTime() + "\t src: " + "NULL" + "\t dst: " + node.getID()+"\t msg:"+aem.value);
	}
		value = (value + aem.value) / 2;
	}

}

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------

/**
 * The type of a message. It contains a value of type double and the sender node
 * of type {@link peersim.core.Node}.
 */
class AverageMessage {

	final double value;
	/**
	 * If not null, this has to be answered, otherwise this is the answer.
	 */
	final Node sender;

	public AverageMessage(double value, Node sender) {
		this.value = value;
		this.sender = sender;
	}
}