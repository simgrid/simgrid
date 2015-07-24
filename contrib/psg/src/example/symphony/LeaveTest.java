package example.symphony;

import peersim.config.Configuration;
import peersim.core.CommonState;
import peersim.core.Control;
import peersim.core.Network;
import peersim.core.Node;

/**
 *
 * @author Andrea Esposito <and1989@gmail.com>
 */
public class LeaveTest implements Control {

    private static final String PAR_NETMANAGER = "symphonynetworkmanager";
    private static final String PAR_NUMBER_LEAVES = "n";
    private static final String PAR_MIN_SIZE = "minsizeOnline";
    private static final String PAR_WAIT_TARGET_SIZE = "waitTargetSizeToStart";
    private final int networkManagerID;
    private final double n;
    private final int minSizeNetwork;
    private int targetSize;

    public LeaveTest(String prefix) {
        networkManagerID = Configuration.getPid(prefix + "." + PAR_NETMANAGER);
        double nAppo = Configuration.getDouble(prefix + "." + PAR_NUMBER_LEAVES);
        if (!(nAppo > 0.0 && nAppo < 1.0)) {
            n = (int) Math.round(nAppo);
        } else {
            n = nAppo;
        }

        minSizeNetwork = Configuration.getInt(prefix + "." + PAR_MIN_SIZE, -1);
        targetSize = Configuration.getInt(prefix + "." + PAR_WAIT_TARGET_SIZE, -1);
    }

    public boolean execute() {

        if (minSizeNetwork > 0) {

            int onlineNode = 0;
            AdapterIterableNetwork it = new AdapterIterableNetwork();
            for (Node node : it) {
                if (node.isUp()) {
                    onlineNode++;
                }
            }

            if (targetSize <= 0 || targetSize <= onlineNode) {
                targetSize = -1;

                // verify if i have to remove an exact number of nodes or a percentage of them
                int actualN = (int) (n < 1.0 ? Math.ceil(Network.size() * n) : n);

                for (int i = 0; i < actualN && Network.size() > 0; i++) {
                    if (onlineNode > minSizeNetwork) {
                        Node leaveNode = Network.get(Math.abs(CommonState.r.nextInt()) % Network.size());

                        while (!leaveNode.isUp()) {
                            leaveNode = Network.get(Math.abs(CommonState.r.nextInt()) % Network.size());
                        }

                        SymphonyNetworkManager networkManager = (SymphonyNetworkManager) leaveNode.getProtocol(networkManagerID);

                        networkManager.leave(leaveNode);

                        onlineNode--;
                    } else {
                        break;
                    }
                }
            }
        }

        return false;
    }
}
