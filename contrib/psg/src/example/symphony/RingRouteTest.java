package example.symphony;

import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.LinkedList;
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
public class RingRouteTest implements Control, Handler {

    private static final String PAR_SYMPHONY = "symphony";
    private static final String PAR_STARTNODE = "startnode";
    private final int symphonyID;
    private final int indexStartNode;
    private Node start;
    private boolean finished;
    private boolean flagTimeout;
    private HashSet<Node> antiLoopSet;

    public RingRouteTest(String prefix) {
        symphonyID = Configuration.getPid(prefix + "." + PAR_SYMPHONY);
        indexStartNode = Configuration.getInt(prefix + "." + PAR_STARTNODE, 0);

        finished = true;
        flagTimeout = false;
        antiLoopSet = new HashSet<Node>();
    }

    public boolean execute() {

        if (!finished && flagTimeout) {

            Logger.getLogger(RingRouteTest.class.getName()).log(Level.WARNING, "Sent msg but no aswer. Timeout. Ring Route Test terminated for Timeout.");

            finished = true;
            flagTimeout = false;
        }

        if (finished) {

            flagTimeout = true;
            antiLoopSet.clear();

            int indexRealStartNode = indexStartNode;
            Node realStartNode = Network.get(indexStartNode);
            SymphonyProtocol symphony = (SymphonyProtocol) realStartNode.getProtocol(symphonyID);

            while (!symphony.isBootstrapped() || !realStartNode.isUp()) {
                indexRealStartNode = (indexRealStartNode + 1) % Network.size();
                realStartNode = Network.get(indexRealStartNode);
                symphony = (SymphonyProtocol) realStartNode.getProtocol(symphonyID);

                if (indexRealStartNode == indexStartNode) {
                    Logger.getLogger(RingRouteTest.class.getName()).log(Level.WARNING, "No ONLINE nodes. The ring is vanished. Ring Route Terminated.");
                    finished = true;
                    flagTimeout = false;
                    return false;
                }
            }

            start = realStartNode;
            finished = false;

            Logger.getLogger(RingRouteTest.class.getName()).log(Level.INFO, "RingRoute started.");

            doRoute(start, true);
        }

        return false;
    }

    public void handle(SymphonyProtocol symphony, Tuple<Node, Double> tuple) {

        if (tuple == null) {
            Logger.getLogger(RingRouteTest.class.getName()).log(Level.SEVERE, "FAIL RING ROUTING");
            finished = true;
            return;
        }

        Logger.getLogger(RingRouteTest.class.getName()).log(Level.FINER, symphony.getIdentifier() + " source find the manager of " + tuple.y + " and it is " + ((SymphonyProtocol) tuple.x.getProtocol(symphonyID)).getIdentifier());

        doRoute(tuple.x, false);
    }

    private void doRoute(Node node, boolean firstTime) {

        SymphonyProtocol symphonyStartNode = (SymphonyProtocol) start.getProtocol(symphonyID);

        if (!symphonyStartNode.isBootstrapped()) {
            Logger.getLogger(RingRouteTest.class.getName()).log(Level.INFO, "The node i started from left. Ring Route Terminated.");
            finished = true;
            return;
        }

        if (!firstTime && node.equals(start)) {
            Logger.getLogger(RingRouteTest.class.getName()).log(Level.INFO, "RingRoute Terminated");
            finished = true;
            return;
        }

        if (antiLoopSet.contains(node)) {
            Logger.getLogger(RingRouteTest.class.getName()).log(Level.INFO, "Not able to reach the node that i started from. Ring Route Terminated.");
            finished = true;
            return;
        } else {
            antiLoopSet.add(node);
        }

        SymphonyProtocol symphony = (SymphonyProtocol) node.getProtocol(symphonyID);
        AdapterSymphonyNodeComparator adapterSymphonyNodeComparator = new AdapterSymphonyNodeComparator(new SymphonyNodeComparator(symphonyID, node));

        Collection<Tuple<Node, BootstrapStatus>> collection = (Collection<Tuple<Node, BootstrapStatus>>) symphony.leftShortRangeLinks.clone();
        LinkedList<Tuple<Node, BootstrapStatus>> list = new LinkedList<Tuple<Node, BootstrapStatus>>(collection);
        Collections.sort(list, adapterSymphonyNodeComparator);

        Node targetNode = null;
        for (Tuple<Node, BootstrapStatus> tuple : list) {
            if (tuple.y == BootstrapStatus.ONLINE) {
                targetNode = tuple.x;
                break;
            }
        }

        if (targetNode == null || !targetNode.isUp()) {
            Logger.getLogger(RingRouteTest.class.getName()).log(Level.WARNING, "Terminated Ring Route but not done completely");
            finished = true;
            return;
        }

        SymphonyProtocol symphonyTarget = (SymphonyProtocol) targetNode.getProtocol(symphonyID);
        try {
            symphony.route(node, symphonyTarget.getIdentifier(), this);
            Logger.getLogger(RingRouteTest.class.getName()).log(Level.FINEST, "Ring from: " + symphony.getIdentifier() + " to " + symphonyTarget.getIdentifier());
        } catch (RoutingException ex) {
            Logger.getLogger(RingRouteTest.class.getName()).log(Level.WARNING, "Finito AnelloRoute MA NON FATTO TUTTO");
            finished = true;
        }
    }
}
