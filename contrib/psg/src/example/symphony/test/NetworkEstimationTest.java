package example.symphony.test;

import example.symphony.AdapterIterableNetwork;
import example.symphony.SymphonyNetworkManager;
import example.symphony.SymphonyProtocol;
import peersim.config.Configuration;
import peersim.core.Control;
import peersim.core.Node;

/**
 *
 * @author Andrea Esposito <and1989@gmail.com>
 */
public class NetworkEstimationTest implements Control {

    private static final String PAR_NETMANAGER = "symphonynetworkmanager";
    private static final String PAR_SYMPHONY = "symphony";
    private final int symphonyID;
    private final int networkManagerID;

    public NetworkEstimationTest(String prefix) {

        networkManagerID = Configuration.getPid(prefix + "." + PAR_NETMANAGER);
        symphonyID = Configuration.getPid(prefix + "." + PAR_SYMPHONY);
    }

    public boolean execute() {

        AdapterIterableNetwork it = new AdapterIterableNetwork();
        int max = Integer.MIN_VALUE;
        int min = Integer.MAX_VALUE;
        int sum = 0;
        int total = 0;
        for (Node node : it) {
            if (node.isUp() && ((SymphonyProtocol) node.getProtocol(symphonyID)).isBootstrapped()) {
                SymphonyNetworkManager networkManager = (SymphonyNetworkManager) node.getProtocol(networkManagerID);
                int n = networkManager.getN();
                min = n < min ? n : min;
                max = n > max ? n : max;
                sum += n;
                total++;
            }
        }

        System.out.println("Real Dimension: " + (Math.log(total) / Math.log(2)));
        System.out.println("Average Estimated Dimension: " + (total == 0 ? "No Node online" : (Math.log((sum / total)) / Math.log(2))));
        System.out.println("MAX: " + Math.log(max) / Math.log(2));
        System.out.println("MIN: " + Math.log(min) / Math.log(2));

        return false;
    }
}
