/**
 * 
 */
package example.chord;

import java.util.ArrayList;
import peersim.core.Control;
import peersim.core.Network;
import peersim.config.Configuration;

/**
 * @author Andrea
 * 
 */
public class MessageCounterObserver implements Control {

	private static final String PAR_PROT = "protocol";

	private final String prefix;

	private final int pid;

	/**
	 * 
	 */
	public MessageCounterObserver(String prefix) {
		this.prefix = prefix;
		this.pid = Configuration.getPid(prefix + "." + PAR_PROT);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see peersim.core.Control#execute()
	 */
	public boolean execute() {
		int size = Network.size();
		int totalStab = 0;
		int totFails = 0;
		ArrayList hopCounters = new ArrayList(); // struttura dati che
													// memorizza gli hop di
													// tutti i mess mandati
		hopCounters.clear();
		int max = 0;
		int min = Integer.MAX_VALUE;
		for (int i = 0; i < size; i++) {
			ChordProtocol cp = (ChordProtocol) Network.get(i).getProtocol(pid);
			// trovare tutti gli hopCOunter dei messaggi lookup mandati
			int[] counters = new int[cp.getLookupMessage().length];
			System.arraycopy(cp.getLookupMessage(), 0, counters, 0, cp
					.getLookupMessage().length);
			totalStab = totalStab + cp.stabilizations;
			totFails = totFails + cp.fails;
			cp.stabilizations = 0;
			cp.fails = 0;
			int maxNew = maxArray(counters, cp.index);
			if (maxNew > max)
				max = maxNew;
			if (cp.index != 0) {
				for (int j = 0; j < cp.index; j++)
					hopCounters.add(counters[j]);
				int minNew = minArray(counters, cp.index);
				if (minNew < min)
					min = minNew;
			}
			cp.emptyLookupMessage();
		}
		double media = meanCalculator(hopCounters);
		if (media > 0)
			System.out.println("Mean:  " + media + " Max Value: " + max
					+ " Min Value: " + min + " # Observations: "
					+ hopCounters.size());
		System.out.println("	 # Stabilizations: " + totalStab + " # Failures: "
				+ totFails);
		System.out.println("");
		return false;
	}

	private double meanCalculator(ArrayList list) {
		int lenght = list.size();
		if (lenght == 0)
			return 0;
		int sum = 0;
		for (int i = 0; i < lenght; i++) {
			sum = sum + ((Integer) list.get(i)).intValue();
		}
		double mean = sum / lenght;
		return mean;
	}

	private int maxArray(int[] array, int dim) {
		int max = 0;
		for (int j = 0; j < dim; j++) {
			if (array[j] > max)
				max = array[j];
		}
		return max;
	}

	private int minArray(int[] array, int dim) {
		int min = 0;
		for (int j = 0; j < dim; j++) {
			if (array[j] < min)
				min = array[j];
		}
		return min;
	}
}
