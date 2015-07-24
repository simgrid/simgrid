package psgsim;

import java.util.LinkedHashMap;
import java.util.Map;
import org.simgrid.msg.Host;
import org.simgrid.msg.MsgException;

import peersim.core.Control;

/**
 * This class executes all controls object scheduled in the
 * {@link PSGPlatform#controlStepMap} collection.
 * 
 * @author Khaled Baati 10/12/2014
 * @version version 1.1
 */
public class PSGProcessController extends org.simgrid.msg.Process {
	
	private Map<Control, Double> controlStepMapTmp = new LinkedHashMap<Control, Double>();

	public PSGProcessController(Host host, String name, String[] args) {
		super(host, name, null);
	}

	@Override
	public void main(String[] args) throws MsgException {
		Double nextControlEvent;
		for (Map.Entry<Control, Double> entry : PSGPlatform.controlStepMap
				.entrySet()) {
			controlStepMapTmp.put(entry.getKey(), entry.getValue());
		}
		while (PSGPlatform.getTime() <= PSGPlatform.getDuration()) {
			for (Map.Entry<Control, Double> entrytmp : controlStepMapTmp
					.entrySet()) {
				Control cle = entrytmp.getKey();
				Double valeur = entrytmp.getValue();
				if (PSGPlatform.getTime() % valeur == 0) {
					cle.execute();
					if (PSGPlatform.getTime() != 0)
						for (Map.Entry<Control, Double> entry : PSGPlatform.controlStepMap
								.entrySet()) {
							if (cle == entry.getKey())
								controlStepMapTmp.replace(cle, valeur, valeur
										+ entry.getValue());

						}
				}
			}
			nextControlEvent = next();
			if (nextControlEvent + PSGPlatform.getTime() >= PSGPlatform.getDuration()) {
				break;
			} else {
				waitFor(nextControlEvent);
			}
		}
	}

	private Double next() {
		Double min = controlStepMapTmp.values().iterator().next();
		for (Map.Entry<Control, Double> entry : controlStepMapTmp.entrySet()) {
			Double valeur = (entry.getValue() - PSGPlatform.getClock());
			if (min > valeur)
				min = valeur;
		}

		return min;
	}
}