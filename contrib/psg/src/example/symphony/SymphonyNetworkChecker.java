package example.symphony;

import java.util.Collections;
import java.util.HashSet;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;

import example.symphony.SymphonyProtocol.BootstrapStatus;
import peersim.config.Configuration;
import peersim.core.Control;
import peersim.core.Network;
import peersim.core.Node;

/**
 *
 * @author Andrea Esposito <and1989@gmail.com>
 */
public class SymphonyNetworkChecker implements Control {

    private static final String PAR_SYMHONY = "symphony";
    private static final String PAR_NETSIZE = "networkestimator";
    private final int symphonyID;
    private final int networkEstimatorID;

    public SymphonyNetworkChecker(String prefix) {
        symphonyID = Configuration.getPid(prefix + "." + PAR_SYMHONY);
        networkEstimatorID = Configuration.getPid(prefix + "." + PAR_NETSIZE);
    }

    public boolean execute() {

        boolean isNotOK = false;

        Set<Double> idSet = new HashSet<Double>();
        Iterable<Node> coll = new AdapterIterableNetwork();

        int countOnline = 0;
        int count = 0;
        int notBootstrapped = 0;
        int countKO = 0;
        int disconnected = 0;

        for (Node node : coll) {
            SymphonyProtocol symphony = (SymphonyProtocol) node.getProtocol(symphonyID);

            if (!node.isUp()) {
                disconnected++;
            } else {
                count++;
            }

            if (symphony.loggedIntoNetwork == SymphonyProtocol.BootstrapStatus.ONLINE) {

                countOnline++;

                NetworkSizeEstimatorProtocolInterface networkEstimator = (NetworkSizeEstimatorProtocolInterface) node.getProtocol(networkEstimatorID);
                int k = (int) Math.ceil(Math.log(networkEstimator.getNetworkSize(node)) / Math.log(2));

                boolean checkLeftShortRangeLinks = symphony.leftShortRangeLinks.size() > 0 && symphony.leftShortRangeLinks.size() <= symphony.numberShortRangeLinksPerSide;
                boolean checkRightShortRangeLinks = symphony.rightShortRangeLinks.size() > 0 && symphony.rightShortRangeLinks.size() <= symphony.numberShortRangeLinksPerSide;

                boolean oneNeighborOnline = false;
                for (Tuple<Node, BootstrapStatus> leftTuple : symphony.leftShortRangeLinks) {
                    if (leftTuple.y != BootstrapStatus.ONLINE && leftTuple.y != BootstrapStatus.OFFLINE) {
                        notBootstrapped++;
                    } else {
                        oneNeighborOnline = true;
                        checkLeftShortRangeLinks = checkLeftShortRangeLinks && SymphonyProtocol.isLeftNeighbour(node, leftTuple.x);
                    }
                }
                checkLeftShortRangeLinks = checkLeftShortRangeLinks && oneNeighborOnline;

                oneNeighborOnline = false;
                for (Tuple<Node, BootstrapStatus> rightTuple : symphony.rightShortRangeLinks) {
                    if (rightTuple.y != BootstrapStatus.ONLINE && rightTuple.y != BootstrapStatus.OFFLINE) {
                        notBootstrapped++;
                    } else {
                        oneNeighborOnline = true;
                        checkRightShortRangeLinks = checkRightShortRangeLinks && !SymphonyProtocol.isLeftNeighbour(node, rightTuple.x);
                    }
                }
                checkRightShortRangeLinks = checkRightShortRangeLinks && oneNeighborOnline;

                // Check if the node is in its neighbours
                if (checkLeftShortRangeLinks) {
                    AdapterSymphonyNodeComparator comparator = new AdapterSymphonyNodeComparator(new SymphonyNodeComparator(symphonyID, node));
                    checkLeftShortRangeLinks = checkLeftShortRangeLinks && !Collections.min(symphony.leftShortRangeLinks, comparator).x.equals(node);
                }

                if (checkRightShortRangeLinks) {
                    AdapterSymphonyNodeComparator comparator = new AdapterSymphonyNodeComparator(new SymphonyNodeComparator(symphonyID, node));
                    checkRightShortRangeLinks = checkRightShortRangeLinks && !Collections.min(symphony.rightShortRangeLinks, comparator).x.equals(node);
                }

                boolean checkLongRangeLinksOutgoing = !symphony.longRangeLinksOutgoing.contains(node);
                boolean checkLongRangeLinksIncoming = /*
                         * symphony.longRangeLinksIncoming.size() <= (2 * k) &&
                         */ !symphony.longRangeLinksIncoming.contains(node);

                boolean checkUniqueID = !idSet.contains(symphony.getIdentifier());
                idSet.add(symphony.getIdentifier());

                boolean nextIsNotOK = !(checkUniqueID && checkLeftShortRangeLinks && checkRightShortRangeLinks && checkLongRangeLinksOutgoing && checkLongRangeLinksIncoming);

                if (nextIsNotOK) {
                    countKO++;
                    Logger.getLogger(SymphonyNetworkChecker.class.getName()).log(Level.SEVERE, "OPS");
                }

                isNotOK = isNotOK || nextIsNotOK;
            }
        }

        System.out.println("Error: " + countKO);
        System.out.println("Online: " + countOnline + "/" + count);
        System.out.println("Not Bootstrapped: " + notBootstrapped);
        System.out.println("Disconnected: " + disconnected);
        System.out.println("Network Size: " + Network.size());

        return isNotOK;
    }
}
