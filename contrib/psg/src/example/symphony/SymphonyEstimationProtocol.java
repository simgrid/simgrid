package example.symphony;

import java.util.*;
import peersim.config.Configuration;
import peersim.core.Network;
import peersim.core.Node;

/**
 *
 * @author Andrea Esposito <and1989@gmail.com>
 */
public class SymphonyEstimationProtocol implements NetworkSizeEstimatorProtocolInterface {

    private static final String PAR_SYMPHONY = "symphony";
    private static final String PAR_S = "s";
    private final int symphonyID;
    private final int s;

    public SymphonyEstimationProtocol(String prefix) {
        symphonyID = Configuration.getPid(prefix + "." + PAR_SYMPHONY);
        s = Configuration.getInt(prefix + "." + PAR_S, -1);
    }

    /**
     * Implementation of the estimated network size as a variant of the paper one. It use anyway the
     * idea to calculate the size from the length segments but without exchanging the information
     * with the neighbours instead using only the local information.
     */
    public int getNetworkSize(Node node) {
        SymphonyProtocol symphony = (SymphonyProtocol) node.getProtocol(symphonyID);

        // If the node is not yet inside the ring i return the minimum size (2 nodes)
        if (!symphony.isBootstrapped()) {
            return 2;
        }

        /*
         * I clone the short range links views (wrapped into an ArrayList because the returned list
         * 'Arrays.asList doesn't support the "removeAll" method or better its size is fixed)
         */
        ArrayList<Tuple<Node, SymphonyProtocol.BootstrapStatus>> leftList = new ArrayList<Tuple<Node, SymphonyProtocol.BootstrapStatus>>(Arrays.asList((Tuple<Node, SymphonyProtocol.BootstrapStatus>[]) symphony.leftShortRangeLinks.toArray(new Tuple[0])));
        ArrayList<Tuple<Node, SymphonyProtocol.BootstrapStatus>> rightList = new ArrayList<Tuple<Node, SymphonyProtocol.BootstrapStatus>>(Arrays.asList((Tuple<Node, SymphonyProtocol.BootstrapStatus>[]) symphony.rightShortRangeLinks.toArray(new Tuple[0])));

        // Remove the neighbours that are offline
        LinkedList<Tuple<Node, SymphonyProtocol.BootstrapStatus>> offlineNeighbors = new LinkedList<Tuple<Node, SymphonyProtocol.BootstrapStatus>>();
        for (Tuple<Node, SymphonyProtocol.BootstrapStatus> tuple : leftList) {
            if (tuple.y == SymphonyProtocol.BootstrapStatus.OFFLINE) {
                offlineNeighbors.add(tuple);
            }
        }
        leftList.removeAll(offlineNeighbors);
        offlineNeighbors.clear();
        for (Tuple<Node, SymphonyProtocol.BootstrapStatus> tuple : rightList) {
            if (tuple.y == SymphonyProtocol.BootstrapStatus.OFFLINE) {
                offlineNeighbors.add(tuple);
            }
        }
        rightList.removeAll(offlineNeighbors);

        // Sort the neighbours based on the distance from me
        Comparator<Tuple<Node, SymphonyProtocol.BootstrapStatus>> comparator = new AdapterSymphonyNodeComparator(new SymphonyNodeComparator(symphonyID, symphony.getIdentifier()));
        Collections.sort(leftList, comparator);
        Collections.sort(rightList, comparator);

        // Calculate the variables to estimated the network size
        double Xs = 0;
        int countS = 0;

        List<Tuple<Node, SymphonyProtocol.BootstrapStatus>> appoList[] = new List[2];
        appoList[0] = leftList;
        appoList[1] = rightList;

        double[] appoPrecIdentifier = new double[]{symphony.getIdentifier(), symphony.getIdentifier()};
        int[] appoCurrentIndex = new int[]{0, 0};

        int realS = (int) (s <= 0 ? Math.log(Network.size() / Math.log(2)) : s);

        for (int i = 0; i < realS; i++) {
            double precIdentifier = appoPrecIdentifier[i % 2];
            int currentIndex = appoCurrentIndex[i % 2];
            List<Tuple<Node, SymphonyProtocol.BootstrapStatus>> currentList = appoList[i % 2];

            try {
                double currentIdentifier = ((SymphonyProtocol) currentList.get(currentIndex).x.getProtocol(symphonyID)).getIdentifier();

                appoPrecIdentifier[i % 2] = currentIdentifier;
                appoCurrentIndex[i % 2] = appoCurrentIndex[i % 2] + 1;

                double distance = Math.abs(currentIdentifier - precIdentifier);
                Xs += Math.min(distance, 1.0 - distance);
                countS++;
            } catch (IndexOutOfBoundsException ex) {
                // Simply i skip the counting
            }
        }

        int ret = Xs == 0 ? 0 : (int) Math.round(countS / Xs);

        return ret;
    }

    @Override
    public Object clone() {
        return this;
    }
}
