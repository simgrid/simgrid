package example.symphony;

import java.util.logging.Level;
import java.util.logging.Logger;
import peersim.config.Configuration;
import peersim.core.Network;
import peersim.core.Node;
import peersim.dynamics.NodeInitializer;

/**
 *
 * @author Andrea Esposito <and1989@gmail.com>
 */
public class SymphonyNodeInizializer implements NodeInitializer {

    private static final String PAR_NETMANAGER = "symphonynetworkmanager";
    private static final String PAR_SYMPHONY = "symphony";
    private static final String PAR_BOOTNODE = "bootstrapnode";
    private final int networkManagerID;
    private final int symphonyID;
    private final int indexBootstrapNode;

    public SymphonyNodeInizializer(String prefix) {

        networkManagerID = Configuration.getPid(prefix + "." + PAR_NETMANAGER);
        indexBootstrapNode = Configuration.getInt(prefix + "." + PAR_BOOTNODE, 0);
        symphonyID = Configuration.getPid(prefix + "." + PAR_SYMPHONY);
    }

    @Override
    public void initialize(Node node) {
        int indexRealBootstrapNode = indexBootstrapNode;
        Node realBootstrapNode = Network.get(indexBootstrapNode);
        SymphonyNetworkManager symphonyNetworkManager = (SymphonyNetworkManager) node.getProtocol(networkManagerID);
        SymphonyProtocol symphony = (SymphonyProtocol) realBootstrapNode.getProtocol(symphonyID);

        boolean joinSent;
        do {
            try {
                while (!symphony.isBootstrapped() || !realBootstrapNode.isUp()) {
                    indexRealBootstrapNode = (indexRealBootstrapNode + 1) % Network.size();
                    realBootstrapNode = Network.get(indexRealBootstrapNode);
                    symphony = (SymphonyProtocol) realBootstrapNode.getProtocol(symphonyID);

                    if (indexRealBootstrapNode == indexBootstrapNode) {
                        Logger.getLogger(SymphonyNodeInizializer.class.getName()).log(Level.WARNING, "No node ONLINE. Impossible to do the network bootstrap.");
                        return;
                    }
                }

                symphonyNetworkManager.join(node, realBootstrapNode);
                joinSent = true;
            } catch (RoutingException ex) {
                Logger.getLogger(SymphonyNodeInizializer.class.getName()).log(Level.SEVERE, "Join Issue");
                joinSent = false;
            }
        } while (!joinSent);
    }
}
