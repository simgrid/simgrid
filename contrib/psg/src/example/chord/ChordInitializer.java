package example.chord;

import java.math.BigInteger;
import java.util.Random;
import peersim.config.Configuration;
import peersim.core.CommonState;
import peersim.core.Network;
import peersim.core.Node;
import peersim.dynamics.NodeInitializer;

public class ChordInitializer implements NodeInitializer {

	private static final String PAR_PROT = "protocol";

	private int pid = 0;

	private ChordProtocol cp;

	public ChordInitializer(String prefix) {
		pid = Configuration.getPid(prefix + "." + PAR_PROT);
	}

	public void initialize(Node n) {
		cp = (ChordProtocol) n.getProtocol(pid);
		join(n);
	}

	public void join(Node myNode) {
		Random generator = new Random();
		//Random generator = new Random(1234567890);
		cp.predecessor = null;
		// search a node to join
		Node n;
		do {
			n = Network.get(generator.nextInt(Network.size()));
		} while (n == null || n.isUp() == false);
		cp.m = ((ChordProtocol) n.getProtocol(pid)).m;
		cp.chordId = new BigInteger(cp.m, CommonState.r);
		ChordProtocol cpRemote = (ChordProtocol) n.getProtocol(pid);

		Node successor = cpRemote.find_successor(cp.chordId);
		cp.fails = 0;
		cp.stabilizations = 0;
		cp.varSuccList = cpRemote.varSuccList;
		cp.varSuccList = 0;
		cp.succLSize = cpRemote.succLSize;
		cp.successorList = new Node[cp.succLSize];
		cp.successorList[0] = successor;
		cp.fingerTable = new Node[cp.m];
		long succId = 0;
		BigInteger lastId = ((ChordProtocol) Network.get(Network.size() - 1)
				.getProtocol(pid)).chordId;
		do {
			cp.stabilizations++;
			succId = cp.successorList[0].getID();
			cp.stabilize(myNode);
			if (((ChordProtocol) cp.successorList[0].getProtocol(pid)).chordId
					.compareTo(cp.chordId) < 0) {
				cp.successorList[0] = ((ChordProtocol) cp.successorList[0]
						.getProtocol(pid)).find_successor(cp.chordId);
			}
			// controllo di non essere l'ultimo elemento della rete
			if (cp.chordId.compareTo(lastId) > 0) {
				cp.successorList[0] = Network.get(0);
				break;
			}
		} while (cp.successorList[0].getID() != succId
				|| ((ChordProtocol) cp.successorList[0].getProtocol(pid)).chordId
						.compareTo(cp.chordId) < 0);
		cp.fixFingers();
	}
}
