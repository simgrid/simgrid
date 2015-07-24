package example.symphony;

import java.util.Collection;
import java.util.Comparator;
import java.util.LinkedList;
import java.util.logging.Level;
import java.util.logging.Logger;

import example.symphony.SymphonyProtocol.BootstrapStatus;
import peersim.config.Configuration;
import peersim.core.CommonState;
import peersim.core.Control;
import peersim.core.Network;
import peersim.core.Node;

/**
 * Inizializer that create the initial ring
 *
 * @author Andrea Esposito <and1989@gmail.com>
 */
public class SymphonyNetworkBuilder implements Control {

    private static final String PAR_SYMHONY = "symphony";
    private static final String PAR_LONG_LINK = "createLongLinks";
    private static final String PAR_MAX_ATTEMPTS = "attempts";
    private final int symphonyID;
    private final boolean createLongRangeLinks;
    private final int MAX_ATTEMPTS;

    public SymphonyNetworkBuilder(String prefix) {

        symphonyID = Configuration.getPid(prefix + "." + PAR_SYMHONY);
        createLongRangeLinks = Configuration.getBoolean(prefix + "." + PAR_LONG_LINK, true);
        MAX_ATTEMPTS = Configuration.getInt(prefix + "." + PAR_MAX_ATTEMPTS, 5);
    }

    public boolean execute() {

        // Sort the network for convenience (from 0.0 to 1.0)
        Network.sort(new Comparator<Node>() {

            public int compare(Node o1, Node o2) {

                SymphonyProtocol symphony1 = (SymphonyProtocol) o1.getProtocol(symphonyID);
                SymphonyProtocol symphony2 = (SymphonyProtocol) o2.getProtocol(symphonyID);

                Double identifier1 = symphony1.getIdentifier();
                Double identifier2 = symphony2.getIdentifier();

                return identifier1.compareTo(identifier2);
            }
        });

        int numShortLinksPerSide = ((SymphonyProtocol) Network.get(0).getProtocol(symphonyID)).numberShortRangeLinksPerSide;

        for (int i = 0; i < Network.size(); i++) {

            Node node = Network.get(i);
            SymphonyProtocol symphonyNode = (SymphonyProtocol) node.getProtocol(symphonyID);

            // Create the short links
            for (int j = 1; j <= numShortLinksPerSide; j++) {

                int pos = i - j;
                pos = pos < 0 ? Network.size() + pos : pos;
                symphonyNode.rightShortRangeLinks.add(new Tuple<Node, BootstrapStatus>(Network.get(pos), BootstrapStatus.ONLINE));

                pos = (i + j) % Network.size();
                symphonyNode.leftShortRangeLinks.add(new Tuple<Node, BootstrapStatus>(Network.get(pos), BootstrapStatus.ONLINE));
            }

            symphonyNode.loggedIntoNetwork = SymphonyProtocol.BootstrapStatus.ONLINE;
        }

        /*
         * UPDATE: Putted a flag to decide if perform this part of code or not at configuration
         * time. At default i create the long range links.
         *
         * The Long Range Links could be left to the networkmanager but the tests that we'll do have
         * to put into account that in an initial phase will be some message exchanging to create
         * the long range links and so the latency is faked... for that reason the long range links
         * are created manually here such a way to have a complete symphony network from the
         * beginning.
         */
        if (createLongRangeLinks) {
            for (Node node : new AdapterIterableNetwork()) {

                SymphonyProtocol symphonyNode = (SymphonyProtocol) node.getProtocol(symphonyID);

                // Create the long links
                int k = (int) Math.ceil(Math.log(Network.size()) / Math.log(2));

                if (symphonyNode.fixedLongRangeLinks) {
                    k = symphonyNode.numberFixedLongRangeLinks;
                }

                Collection<Node> allShortLinks = new LinkedList<Node>();
                for (Tuple<Node, BootstrapStatus> shortTuple : symphonyNode.leftShortRangeLinks) {
                    allShortLinks.add(shortTuple.x);
                }
                for (Tuple<Node, BootstrapStatus> shortTuple : symphonyNode.rightShortRangeLinks) {
                    allShortLinks.add(shortTuple.x);
                }

                int j = 0;
                int attempts = MAX_ATTEMPTS;
                while (j <= k) {

                    double distance = Math.exp(k * (CommonState.r.nextDouble() - 1.0));
                    double targetIdentifier = (symphonyNode.getIdentifier() + distance) % 1;

                    Node targetNode;
                    try {

                        // use the unidirectional routing because i want to catch the manager
                        targetNode = symphonyNode.findClosestNode(targetIdentifier, new AdapterIterableNetwork(), true);
                        SymphonyProtocol symphonyTargetNode = (SymphonyProtocol) targetNode.getProtocol(symphonyID);
                        if (!targetNode.equals(node)
                                && !symphonyNode.longRangeLinksOutgoing.contains(targetNode)
                                && symphonyTargetNode.longRangeLinksIncoming.size() < (2 * k)
                                && !allShortLinks.contains(targetNode)) {

                            boolean fresh = symphonyTargetNode.longRangeLinksIncoming.add(node);

                            if (fresh) {
                                j++;
                                attempts = MAX_ATTEMPTS;
                                symphonyNode.longRangeLinksOutgoing.add(targetNode);
                            } else {
                                attempts--;
                                if (attempts <= 0) { // Because i don't want to loop i try a finite number of times
                                    attempts = MAX_ATTEMPTS;
                                    j++;
                                }

                            }
                        } else {
                            attempts--;
                            if (attempts <= 0) { // Because i don't want to loop i try a finite number of times
                                attempts = MAX_ATTEMPTS;
                                j++;
                            }

                        }
                    } catch (RoutingException ex) {
                        Logger.getLogger(SymphonyNetworkBuilder.class.getName()).log(Level.SEVERE, null, ex);
                    }
                }
            }
        }

        // Shuffle
        Network.shuffle();

        return false;
    }
}
