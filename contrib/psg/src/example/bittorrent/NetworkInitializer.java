/*
 * Copyright (c) 2007-2008 Fabrizio Frioli, Michele Pedrolli
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
 * --
 *
 * Please send your questions/suggestions to:
 * {fabrizio.frioli, michele.pedrolli} at studenti dot unitn dot it
 *
 */

package example.bittorrent;

import peersim.core.*;
import peersim.config.Configuration;
import peersim.edsim.EDSimulator;
import peersim.transport.Transport;
import java.util.Random;

/**
 * This {@link Control} ...
 */
public class NetworkInitializer implements Control {
	/**
	* The protocol to operate on.
	*
	* @config
	*/
	private static final String PAR_PROT="protocol";
		
	private static final String PAR_TRANSPORT="transport";
	
	private static final int TRACKER = 11;
	
	private static final int CHOKE_TIME = 13;
	
	private static final int OPTUNCHK_TIME = 14;
	
	private static final int ANTISNUB_TIME = 15;
	
	private static final int CHECKALIVE_TIME = 16;
	
	private static final int TRACKERALIVE_TIME = 17;
	
	/** Protocol identifier, obtained from config property */
	private final int pid;
	private final int tid;
	private NodeInitializer init;
	
	private Random rnd;
	
	public NetworkInitializer(String prefix) {
		pid = Configuration.getPid(prefix+"."+PAR_PROT);
		tid = Configuration.getPid(prefix+"."+PAR_TRANSPORT);
		init = new NodeInitializer(prefix);
	}
	
	public boolean execute() {
		int completed;
		Node tracker = Network.get(0);
		
		// manca l'inizializzazione del tracker;
		
		((BitTorrent)Network.get(0).getProtocol(pid)).initializeTracker();
		
		for(int i=1; i<Network.size(); i++){
			System.err.println("chiamate ad addNeighbor " + i);
			((BitTorrent)Network.get(0).getProtocol(pid)).addNeighbor(Network.get(i));
			init.initialize(Network.get(i));
		}
		for(int i=1; i< Network.size(); i++){
			Node n = Network.get(i);
			
			Object ev = new SimpleMsg(TRACKER, n);
			long latency = ((Transport)n.getProtocol(tid)).getLatency(n,tracker);
			EDSimulator.add(latency,ev,tracker,pid);			
//			((Transport) n.getProtocol(tid)).send(n,tracker, ev, pid);

			ev = new SimpleEvent(CHOKE_TIME);
			EDSimulator.add(10000,ev,n,pid);
			ev = new SimpleEvent(OPTUNCHK_TIME);
			EDSimulator.add(30000,ev,n,pid);
			ev = new SimpleEvent(ANTISNUB_TIME);
			EDSimulator.add(60000,ev,n,pid);
			ev = new SimpleEvent(CHECKALIVE_TIME);
			EDSimulator.add(120000,ev,n,pid);
			ev = new SimpleEvent(TRACKERALIVE_TIME);
			EDSimulator.add(1800000,ev,n,pid);
		}
		return true;
	}
	
	}
