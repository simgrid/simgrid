package example.symphony;

import java.util.*;
import java.util.logging.Level;
import java.util.logging.Logger;

import example.symphony.Message.MessageType;
import example.symphony.SymphonyProtocol.BootstrapStatus;
import peersim.cdsim.CDProtocol;
import peersim.config.Configuration;
import peersim.core.CommonState;
import peersim.core.Fallible;
import peersim.core.Node;
import peersim.edsim.EDProtocol;
import peersim.transport.Transport;

/**
 *
 * @author Andrea Esposito <and1989@gmail.com>
 */
public class SymphonyNetworkManager implements EDProtocol, CDProtocol {

    private static final String PAR_SYMPHONY = "symphony";
    private static final String PAR_TRANSP = "transport";
    private static final String PAR_ATTEMPTS = "attempts";
    private static final String PAR_NETSIZE = "networkestimator";
    private static final String PAR_NUM_TIMEOUT = "nTimeout";
    private static final String PAR_RELINKING = "relinking";
    private static final String PAR_RELINKING_LOWER_BOUND = "relinkingLowerBound";
    private static final String PAR_RELINKING_UPPER_BOUND = "relinkingUpperBound";
    private static final int DEFAULT_K = 1;
    private static final int DEFAULT_N = 2;
    private static final double DEFAULT_RELINKING_LOWER_BOUND = 0.5;
    private static final double DEFAULT_RELINKING_UPPER_BOUND = 2.0;
    private final String prefix;
    private final int symphonyID;
    private final int transportID;
    private final int networkEstimatorID;
    private final int attempts;
    private final int pid;
    private final int nTimeout;
    private final HashMap<Node, Integer> keepAliveMap;
    private final boolean relinkingProtocolActivated;
    private final double relinkingUpperBound;
    private final double relinkingLowerBound;
    private int k = DEFAULT_K; // Number of Long Range Link
    private int n = DEFAULT_N; // Estimation Network size
    private static boolean firstPrintConfig = true;
    /*
     * Estimation Network size at which last long distance link was established, at the beginning -1
     * to indicate that we never had Long Range Links
     */
    private int nLink = -1;
    private int currentAttempts;

    public SymphonyNetworkManager(String prefix) {

        this.prefix = prefix;
        pid = Configuration.lookupPid(prefix.replaceAll("protocol.", ""));
        symphonyID = Configuration.getPid(prefix + "." + PAR_SYMPHONY);
        transportID = Configuration.getPid(prefix + "." + PAR_TRANSP);
        networkEstimatorID = Configuration.getPid(prefix + "." + PAR_NETSIZE);
        attempts = Configuration.getInt(prefix + "." + PAR_ATTEMPTS);
        nTimeout = Configuration.getInt(prefix + "." + PAR_NUM_TIMEOUT, 10);
        relinkingProtocolActivated = !Configuration.getString(prefix + "." + PAR_RELINKING, "on").toLowerCase().equals("off");
        double relinkingLowerBoundAppo = Configuration.getDouble(prefix + "." + PAR_RELINKING_LOWER_BOUND, DEFAULT_RELINKING_LOWER_BOUND);
        double relinkingUpperBoundAppo = Configuration.getDouble(prefix + "." + PAR_RELINKING_UPPER_BOUND, DEFAULT_RELINKING_UPPER_BOUND);
        if (relinkingLowerBoundAppo > relinkingUpperBoundAppo) {
            relinkingLowerBound = DEFAULT_RELINKING_LOWER_BOUND;
            relinkingUpperBound = DEFAULT_RELINKING_UPPER_BOUND;
        } else {
            relinkingLowerBound = relinkingLowerBoundAppo;
            relinkingUpperBound = relinkingUpperBoundAppo;
        }

        keepAliveMap = new HashMap<Node, Integer>();

        printConfig();
    }

    private void printConfig() {

        if (firstPrintConfig) {
            firstPrintConfig = false;
            System.out.println(SymphonyNetworkManager.class.getSimpleName() + " Configuration:");
            System.out.println("- Attempts per LongRangeLinks: " + attempts);
            System.out.println("- Number of Timeout before a node is considered OFFLINE (through Keep-alive):" + nTimeout);
            System.out.println("- Relinking: " + (relinkingProtocolActivated ? "ON" : "OFF"));
            System.out.println("- Relinking Range: [" + relinkingLowerBound + ", " + relinkingUpperBound + "]");
            System.out.println("-------------------------------\n");
        }
    }

    public void join(final Node node, final Node bootstrapNode) throws RoutingException {
        final SymphonyProtocol bootstrapSymphony = (SymphonyProtocol) bootstrapNode.getProtocol(symphonyID);
        SymphonyProtocol symphony = (SymphonyProtocol) node.getProtocol(symphonyID);

        /*
         * Search (through the bootstrap node) and contact the Manager Node of myself such a way to
         * be able to insert myself into the ring and create the short links
         *
         */
        bootstrapSymphony.route(bootstrapNode, symphony.getIdentifier(), new Handler() {

            public void handle(SymphonyProtocol src, Tuple<Node, Double> tuple) {
                if (tuple == null) {
                    Logger.getLogger(SymphonyNetworkManager.class.getName()).log(Level.SEVERE, "FAIL ROUTE JOIN");
                    node.setFailState(Fallible.DEAD);
                    return;
                }

                Node managerNode = tuple.x;

                Transport transport = (Transport) node.getProtocol(transportID);
                Message msg = new Message(node, node, MessageType.JOIN);
                transport.send(node, managerNode, msg, pid);
            }
        });

        // The Long Range Links are added after that i joined the ring (before i can't because i haven't got the nodes to do the routing)
    }

    /**
     * Conservative Re-Linking (i reuse the ones already created: not all fresh)
     *
     * @param node
     */
    public void updateLongRangeLinks(Node node) {
        SymphonyProtocol symphony = (SymphonyProtocol) node.getProtocol(symphonyID);
        Transport transport = (Transport) node.getProtocol(transportID);

        // if too much links i delete the farest ones
        while (symphony.longRangeLinksOutgoing.size() > k) {
            Node distantNode = Collections.max(symphony.longRangeLinksOutgoing, new SymphonyNodeComparator(symphonyID, node));
            symphony.longRangeLinksOutgoing.remove(distantNode);

            // Communicate to the outgoing node that it ins't anymore one of my long range links
            Message disconnectMsg = new Message(null, node, MessageType.DISCONNECT_LONG_RANGE_LINK);
            transport.send(node, distantNode, disconnectMsg, pid);
        }

        // I can search Long Range Links only if i'm into the ring and i'm able to do routing
        if (symphony.isBootstrapped()) {
            // if only few i try again, untill attempts times, to add new ones
            int difference = k - symphony.longRangeLinksOutgoing.size();
            currentAttempts = attempts;
            for (int i = 0; i < difference; i++) {
                sendLongRangeLinkRequest(symphony, node);
            }
        }
    }
    private static final int MAX_ANTILOOP_COUNT_MANAGER_MYSELF = 5;
    private int antiLoopManagerMySelf = 0;

    private void sendLongRangeLinkRequest(final SymphonyProtocol symphony, final Node node) {
        boolean routingOk;
        do {
            double distance = Math.exp((Math.log(n) / Math.log(2)) * (CommonState.r.nextDouble() - 1.0)); // Harmonic Distribution
            double targetIdentifier = (symphony.getIdentifier() + distance) % 1;
            try {

                symphony.route(node, targetIdentifier, new Handler() {

                    public void handle(SymphonyProtocol src, Tuple<Node, Double> tuple) {

                        if (tuple == null) {
                            Logger.getLogger(SymphonyNetworkManager.class.getName()).log(Level.SEVERE, "FAIL ROUTE SENDLONGRANGELINKREQUEST");
                            return;
                        }

                        Collection<Node> allShortLinks = new LinkedList<Node>();
                        for (Tuple<Node, BootstrapStatus> shortTuple : symphony.leftShortRangeLinks) {
                            allShortLinks.add(shortTuple.x);
                        }
                        for (Tuple<Node, BootstrapStatus> shortTuple : symphony.rightShortRangeLinks) {
                            allShortLinks.add(shortTuple.x);
                        }

                        /*
                         *
                         * I'm myself one of my short links, special case... i try again without
                         * reduce the attempts for a maximum of MAX_ANTILOOP_COUNT_MANAGER_MYSELF
                         * times after that i start again to reduce the attempts
                         */
                        if (tuple.x.equals(node) || allShortLinks.contains(tuple.x)) {

                            if (antiLoopManagerMySelf < MAX_ANTILOOP_COUNT_MANAGER_MYSELF) {

                                antiLoopManagerMySelf++;
                                sendLongRangeLinkRequest(symphony, node);
                            } else {
                                antiLoopManagerMySelf = 0;
                                currentAttempts--;
                            }
                        } else {

                            boolean alreadyAdded = symphony.longRangeLinksOutgoing.contains(tuple.x);
                            /*
                             *
                             * OPINABLE: DESCREASE ATTEMPTS ONLY FOR REJECT? If yes i have to manage
                             * the possible loop (nodi exhaurited so already all added)
                             */
                            if (alreadyAdded && currentAttempts > 0) {
                                currentAttempts--;
                                sendLongRangeLinkRequest(symphony, node);
                            } else if (!alreadyAdded) {
                                Message msg = new Message(null, node, MessageType.REQUEST_LONG_RANGE_LINK);
                                Transport transport = (Transport) node.getProtocol(transportID);
                                transport.send(node, tuple.x, msg, pid);
                            }
                        }
                    }
                });
                routingOk = true;
            } catch (RoutingException ex) {
                routingOk = false;
            }
        } while (!routingOk);
    }

    public void leave(Node node) {
        SymphonyProtocol symphony = (SymphonyProtocol) node.getProtocol(symphonyID);

        if (symphony.loggedIntoNetwork != BootstrapStatus.OFFLINE) {
            Transport transport = (Transport) node.getProtocol(transportID);

            symphony.loggedIntoNetwork = BootstrapStatus.OFFLINE;

            // Communicate that i'm leaving to the outgoing (that i point to) nodes
            for (Node outgoingNode : symphony.longRangeLinksOutgoing) {
                Message disconnectMsg = new Message(null, node, MessageType.DISCONNECT_LONG_RANGE_LINK);
                transport.send(node, outgoingNode, disconnectMsg, pid);
            }

            // Communicate that i'm leaving to the incoming (that they point to me) nodes
            for (Node incomingNode : symphony.longRangeLinksIncoming) {
                Message unavailableMsg = new Message(null, node, MessageType.UNAVAILABLE_LONG_RANGE_LINK);
                transport.send(node, incomingNode, unavailableMsg, pid);
            }

            // Communicate to my neighbours (short range links) that i'm leaving and i send to them the near neighbours
            for (Tuple<Node, BootstrapStatus> leftTuple : symphony.leftShortRangeLinks) {
                Message leaveMsg = new Message(symphony.rightShortRangeLinks.clone(), node, MessageType.LEAVE);
                transport.send(node, leftTuple.x, leaveMsg, pid);
            }

            for (Tuple<Node, BootstrapStatus> rightTuple : symphony.rightShortRangeLinks) {
                Message leaveMsg = new Message(symphony.leftShortRangeLinks.clone(), node, MessageType.LEAVE);
                transport.send(node, rightTuple.x, leaveMsg, pid);
            }

            node.setFailState(Fallible.DEAD);
        }
    }

    public void processEvent(Node node, int pid, Object event) {

        Message msg = (Message) event;

        SymphonyProtocol symphony = (SymphonyProtocol) node.getProtocol(symphonyID);
        Transport transport = (Transport) node.getProtocol(transportID);

        Collection<Tuple<Node, BootstrapStatus>> collection = null;
        switch (msg.getType()) {
            case JOIN:
                // I send my  current neighbours to the entering node
                collection = (Collection<Tuple<Node, BootstrapStatus>>) symphony.leftShortRangeLinks.clone();
                collection.addAll((Collection<Tuple<Node, BootstrapStatus>>) symphony.rightShortRangeLinks.clone());
                Message responseMsg = new Message(collection, node, MessageType.JOIN_RESPONSE);
                transport.send(node, msg.getSourceNode(), responseMsg, pid);

                /*
                 * Update my neighbours list, adding the new one (for sure it is entering in the
                 * left side)
                 *
                 * Put to "ONLINE_AND_ALL_NEIGHBOURS_OFFLINE" because maybe the bootstrap phase is
                 * not terminated yet (ashyncronous communication)
                 */
                symphony.leftShortRangeLinks.add(new Tuple<Node, BootstrapStatus>(msg.getSourceNode(), BootstrapStatus.ONLINE_AND_ALL_NEIGHBOURS_OFFLINE));


                fixNeighbours(node, symphony.leftShortRangeLinks);
                break;
            case JOIN_RESPONSE:

                Collection<Tuple<Node, BootstrapStatus>> tupleCollection = (Collection<Tuple<Node, BootstrapStatus>>) msg.getBody();

                /*
                 *
                 * My manager is a right neighbour. The manager is already inside the ring, boostrap
                 * obliviously ok
                 */
                symphony.rightShortRangeLinks.add(new Tuple<Node, BootstrapStatus>(msg.getSourceNode(), BootstrapStatus.ONLINE));

                // Set my neighbours in the correct position
                for (Tuple<Node, BootstrapStatus> tuple : tupleCollection) {
                    if (SymphonyProtocol.isLeftNeighbour(node, tuple.x)) {
                        symphony.leftShortRangeLinks.add(tuple);
                    } else {
                        symphony.rightShortRangeLinks.add(tuple);
                    }
                }

                fixNeighbours(node, symphony.leftShortRangeLinks);
                fixNeighbours(node, symphony.rightShortRangeLinks);

                // Update bootstrap status
                checkBootstrapStatus(node);

                // I send the refresh command such a way to exchange the views
                refreshNeighbours(node);

                // Update Long Range Links, because it's at the beginning is the same as adding k
                updateLongRangeLinks(node);
                break;
            case UPDATE_NEIGHBOURS:

                Collection<Tuple<Node, BootstrapStatus>> collectionCloned = ((Collection<Tuple<Node, BootstrapStatus>>) symphony.leftShortRangeLinks.clone());
                collectionCloned.addAll(((Collection<Tuple<Node, BootstrapStatus>>) symphony.rightShortRangeLinks.clone()));

                // Send my neighbours such a way it can also update itself
                Message responseUpdateMsg = new Message(collectionCloned, node, MessageType.UPDATE_NEIGHBOURS_RESPONSE);
                transport.send(node, msg.getSourceNode(), responseUpdateMsg, pid);

                // Update my view with the new node
                Tuple<Node, BootstrapStatus> neighbourTuple = new Tuple<Node, BootstrapStatus>(msg.getSourceNode(), (BootstrapStatus) msg.getBody());
                if (SymphonyProtocol.isLeftNeighbour(node, msg.getSourceNode())) {
                    collection = symphony.leftShortRangeLinks;
                } else {
                    collection = symphony.rightShortRangeLinks;
                }
                collection.add(neighbourTuple);

                fixNeighbours(node, collection);
                fixLookAheadMap(node);
                break;
            case UPDATE_NEIGHBOURS_RESPONSE:

                Collection<Tuple<Node, BootstrapStatus>> responseCollection = (Collection<Tuple<Node, BootstrapStatus>>) msg.getBody();

                for (Tuple<Node, BootstrapStatus> neighbourResponseTuple : responseCollection) {
                    if (SymphonyProtocol.isLeftNeighbour(node, neighbourResponseTuple.x)) {
                        collection = symphony.leftShortRangeLinks;
                    } else {
                        collection = symphony.rightShortRangeLinks;
                    }
                    collection.add(neighbourResponseTuple);
                }

                // Fix the neighbours number to the maximum allow and maybe remove myself from the list
                fixNeighbours(node, symphony.leftShortRangeLinks);
                fixNeighbours(node, symphony.rightShortRangeLinks);
                fixLookAheadMap(node);
                break;
            case UPDATE_STATUS:
            case UPDATE_STATUS_RESPONSE:

                Node updNode = msg.getSourceNode();
                BootstrapStatus updStatus = (BootstrapStatus) msg.getBody();

                // I search the neighbour and i update its status
                boolean founded = false;

                // Try to see if it is on the left
                for (Tuple<Node, BootstrapStatus> leftTuple : symphony.leftShortRangeLinks) {
                    if (leftTuple.x.equals(updNode)) {
                        symphony.leftShortRangeLinks.remove(leftTuple);
                        symphony.leftShortRangeLinks.add(new Tuple<Node, BootstrapStatus>(updNode, updStatus));

                        founded = true;
                        break;
                    }
                }

                // If it isn't on the left i try with the neighbours on the right
                if (!founded) {
                    for (Tuple<Node, BootstrapStatus> rightTuple : symphony.rightShortRangeLinks) {
                        if (rightTuple.x.equals(updNode)) {
                            symphony.rightShortRangeLinks.remove(rightTuple);
                            symphony.rightShortRangeLinks.add(new Tuple<Node, BootstrapStatus>(updNode, updStatus));

                            break;
                        }
                    }

                    fixNeighbours(node, symphony.rightShortRangeLinks);
                } else {
                    fixNeighbours(node, symphony.leftShortRangeLinks);
                }

                checkBootstrapStatusAndAlert(node);

                if (msg.getType() == MessageType.UPDATE_STATUS) {
                    Message responseUpdStatus = new Message(symphony.loggedIntoNetwork, node, MessageType.UPDATE_STATUS_RESPONSE);
                    transport.send(node, updNode, responseUpdStatus, pid);
                }

                break;
            case REQUEST_LONG_RANGE_LINK:
                MessageType responseType = MessageType.REJECT_LONG_RANGE_LINK;
                if (symphony.longRangeLinksIncoming.size() < (2 * k)) {
                    boolean added = symphony.longRangeLinksIncoming.add(msg.getSourceNode());
                    if (added) {
                        responseType = MessageType.ACCEPTED_LONG_RANGE_LINK;
                    }
                }
                Message responseLongLinkMsg = new Message(null, node, responseType);
                transport.send(node, msg.getSourceNode(), responseLongLinkMsg, pid);
                break;
            case ACCEPTED_LONG_RANGE_LINK:
                nLink = n;
                symphony.longRangeLinksOutgoing.add(msg.getSourceNode());
                break;
            case REJECT_LONG_RANGE_LINK:
                if (currentAttempts > 0) {
                    currentAttempts--;
                    sendLongRangeLinkRequest(symphony, node);
                }
                break;
            case DISCONNECT_LONG_RANGE_LINK:
                symphony.longRangeLinksIncoming.remove(msg.getSourceNode());
                symphony.lookAheadMap.put(msg.getSourceNode(), null);
                break;
            case UNAVAILABLE_LONG_RANGE_LINK:
                symphony.longRangeLinksOutgoing.remove(msg.getSourceNode());
                symphony.lookAheadMap.put(msg.getSourceNode(), null);
                break;
            case LEAVE:
                Tuple<Node, BootstrapStatus> foundedTuple = null;

                // Verify if the node that is leaving is a left neighbour
                for (Tuple<Node, BootstrapStatus> leftTuple : symphony.leftShortRangeLinks) {
                    if (leftTuple.x.equals(msg.getSourceNode())) {
                        collection = symphony.leftShortRangeLinks;
                        foundedTuple = leftTuple;
                        break;
                    }
                }

                // Verify if the node that is leaving is a right neighbour
                if (collection == null) {
                    for (Tuple<Node, BootstrapStatus> rightTuple : symphony.rightShortRangeLinks) {
                        if (rightTuple.x.equals(msg.getSourceNode())) {
                            collection = symphony.rightShortRangeLinks;
                            foundedTuple = rightTuple;
                            break;
                        }
                    }
                }

                // if i've found the neighbour i remove it and i add to myself its neighbours
                if (collection != null) {
                    collection.addAll((Collection<Tuple<Node, BootstrapStatus>>) msg.getBody());
                    collection.remove(foundedTuple);
                    fixNeighbours(node, collection);

                    // Update status and ready to send an alert in case i'm out of the ring
                    checkBootstrapStatusAndAlert(node);
                }
                break;
            case KEEP_ALIVE:
                Set<Double>[] lookAheadSetArray = new LinkedHashSet[2];

                /*
                 * Check if the contacting node is doing lookAhead and in case of affirmative answer
                 * i provide to it the long range link identifiers (according to my routing mode)
                 */
                if ((Boolean) msg.getBody()) {
                    int i = 0;
                    Iterable[] iterableArray;
                    if (symphony.bidirectionalRouting) {
                        iterableArray = new Iterable[]{symphony.longRangeLinksOutgoing, symphony.longRangeLinksIncoming};
                    } else {
                        iterableArray = new Iterable[]{symphony.longRangeLinksOutgoing};
                    }

                    for (Iterable<Node> iterable : iterableArray) {
                        lookAheadSetArray[i] = new LinkedHashSet<Double>();
                        Set<Double> lookAheadSet = lookAheadSetArray[i];
                        Iterator<Node> it = iterable.iterator();
                        while (it.hasNext()) {
                            Node longLinkNode = it.next();
                            lookAheadSet.add(((SymphonyProtocol) longLinkNode.getProtocol(symphonyID)).getIdentifier());
                        }
                        i++;
                    }
                }

                transport.send(node, msg.getSourceNode(), new Message(lookAheadSetArray, node, MessageType.KEEP_ALIVE_RESPONSE), pid);
                break;
            case KEEP_ALIVE_RESPONSE:
                // Reset the counter to 0
                keepAliveMap.put(msg.getSourceNode(), 0);

                if (symphony.lookAhead) {
                    symphony.lookAheadMap.put(msg.getSourceNode(), (Set<Double>[]) msg.getBody());
                }

                break;
        }
    }

    /**
     *
     * Update the status and communicate immediately to the neighbours if the node is gone out from
     * the ring (and before it was inside)
     *
     * @param node
     */
    private void checkBootstrapStatusAndAlert(Node node) {
        SymphonyProtocol symphony = (SymphonyProtocol) node.getProtocol(symphonyID);
        BootstrapStatus beforeStatus = symphony.loggedIntoNetwork;

        checkBootstrapStatus(node);

        // Instead of waiting that the update happens periodically i do it now because i'm out of the ring and before i wasn't
        if (symphony.loggedIntoNetwork != beforeStatus && !symphony.isBootstrapped()) {
            updateBootstrapStatusNeighbours(node, true);
        }
    }

    private void fixNeighbours(Node node, Collection<Tuple<Node, BootstrapStatus>> neighbours) {

        SymphonyProtocol symphony = (SymphonyProtocol) node.getProtocol(symphonyID);

        // Remove duplicates, remove that ones that are in an obsolete status
        Collection<Tuple<Node, BootstrapStatus>> removedNeighbours = new LinkedHashSet<Tuple<Node, BootstrapStatus>>();
        for (Tuple<Node, BootstrapStatus> tuple : neighbours) {

            // Remove myself from the neighbours list
            if (tuple.x.equals(node)) {
                removedNeighbours.add(tuple);
                continue;
            }

            EnumSet<BootstrapStatus> status = EnumSet.allOf(BootstrapStatus.class);
            status.remove(tuple.y);

            for (BootstrapStatus opposite : status) {
                Tuple<Node, BootstrapStatus> oppositeNeighbour = new Tuple<Node, BootstrapStatus>(tuple.x, opposite);
                if (neighbours.contains(oppositeNeighbour)) {
                    if (tuple.y != BootstrapStatus.ONLINE) {
                        removedNeighbours.add(new Tuple<Node, BootstrapStatus>(tuple.x, BootstrapStatus.OFFLINE));
                        if (opposite == BootstrapStatus.ONLINE) {
                            removedNeighbours.add(new Tuple<Node, BootstrapStatus>(tuple.x, BootstrapStatus.ONLINE_AND_ALL_NEIGHBOURS_OFFLINE));
                        }
                    }
                }
            }
        }
        neighbours.removeAll(removedNeighbours);

        /*
         *
         * I count the neighbours that are in the ONLINE status but before i remove the ones that
         * are gone in timeout during the keep-alive procedure because can be someone that is old
         * but not remove from the exchanging views (UPDATE_NEIGHBOURS) procedure and are not
         * effectively online. To do anyway the possibility to the node to join again i decrease its
         * timeout value. This only if the node is ONLINE and so i'm really interested that it is ok
         * for the routing.
         *
         */
        int onlineNeighbours = 0;
        for (Tuple<Node, BootstrapStatus> tuple : neighbours) {

            Integer value = keepAliveMap.get(tuple.x);
            if (value != null && value >= nTimeout && tuple.y == BootstrapStatus.ONLINE) {
                keepAliveMap.put(tuple.x, value - 1);
                removedNeighbours.add(tuple);
            } else {

                if (tuple.y == BootstrapStatus.ONLINE) {
                    onlineNeighbours++;
                }
            }
        }
        neighbours.removeAll(removedNeighbours);

        // Fix the neighbours number to the maximum allowed
        SymphonyNodeComparator comparator = new SymphonyNodeComparator(symphonyID, node);
        AdapterSymphonyNodeComparator adapterComparator = new AdapterSymphonyNodeComparator(comparator);
        while (neighbours.size() > symphony.numberShortRangeLinksPerSide) {
            Tuple<Node, BootstrapStatus> distantTuple = Collections.max(neighbours, adapterComparator);

            // Mantain the link with the ring
            if (distantTuple.y == BootstrapStatus.ONLINE) {
                if (onlineNeighbours > 1) {
                    neighbours.remove(distantTuple);
                    onlineNeighbours--;
                } else {
                    /*
                     * If will be only one neighbour that is online i save it and i'm going to
                     * eliminate another one (for sure it'll be not online)
                     *
                     */
                    Tuple<Node, BootstrapStatus> backupOnlineNeighbour = distantTuple;
                    neighbours.remove(backupOnlineNeighbour);
                    distantTuple = Collections.max(neighbours, adapterComparator);
                    neighbours.add(backupOnlineNeighbour);
                    neighbours.remove(distantTuple);
                }

            } else {
                neighbours.remove(distantTuple);
            }
        }
    }

    @Override
    public Object clone() {
        SymphonyNetworkManager dolly = new SymphonyNetworkManager(prefix);
        return dolly;
    }

    public void nextCycle(Node node, int protocolID) {

        if (node.isUp()) {

            // Update the estimated network size
            updateN(node);

            // Update the estimated K
            updateK(node);

            // Update the bootstrap status of my neighbours that were joining the ring
            updateBootstrapStatusNeighbours(node, false);

            // Refresh the neighbours views
            refreshNeighbours(node);

            // I send and check the connection status of the neighbours
            keepAlive(node);

            // Update the bootstrap status
            checkBootstrapStatus(node);

            // If it's active i check the Relinking criteria
            if (relinkingProtocolActivated) {
                reLinkingProtocol(node);
            }

            // Update the long range links (conservative)
            updateLongRangeLinks(node);
        }
    }

    /**
     *
     * @param allNeighbours true, communicate/receive the status update from all the neighbours.
     * false, communicate/receive the status update only from the neighbours that are NOT ONLINE
     *
     */
    private void updateBootstrapStatusNeighbours(Node node, boolean allNeighbours) {
        SymphonyProtocol symphony = (SymphonyProtocol) node.getProtocol(symphonyID);
        Transport transport = (Transport) node.getProtocol(transportID);

        Collection<Tuple<Node, BootstrapStatus>> collection = new LinkedHashSet<Tuple<Node, BootstrapStatus>>();
        collection.addAll(symphony.leftShortRangeLinks);
        collection.addAll(symphony.rightShortRangeLinks);

        for (Tuple<Node, BootstrapStatus> neighbourTuple : collection) {
            if (allNeighbours || neighbourTuple.y != BootstrapStatus.ONLINE) {
                Message msg = new Message(symphony.loggedIntoNetwork, node, MessageType.UPDATE_STATUS);
                transport.send(node, neighbourTuple.x, msg, pid);
            }
        }
    }

    private void updateN(Node node) {
        NetworkSizeEstimatorProtocolInterface networkEstimator = (NetworkSizeEstimatorProtocolInterface) node.getProtocol(networkEstimatorID);
        n = networkEstimator.getNetworkSize(node);
        if (n <= 0) {
            n = DEFAULT_N;
        }
    }

    /**
     * Update the K value with the current expectation of the network size
     */
    private void updateK(Node node) {

        SymphonyProtocol symphony = (SymphonyProtocol) node.getProtocol(symphonyID);
        if (!symphony.fixedLongRangeLinks) {
            k = (int) Math.ceil(Math.log(n) / Math.log(2));

            if (k <= 0) {
                k = DEFAULT_K;
            }
        } else {
            k = symphony.numberFixedLongRangeLinks;
        }
    }

    private void refreshNeighbours(Node node) {
        SymphonyProtocol symphony = (SymphonyProtocol) node.getProtocol(symphonyID);
        Transport transport = (Transport) node.getProtocol(transportID);

        for (Tuple<Node, BootstrapStatus> leftTuple : symphony.leftShortRangeLinks) {
            Node leftNode = leftTuple.x;
            Message updateNeighbourMsg = new Message(symphony.loggedIntoNetwork, node, MessageType.UPDATE_NEIGHBOURS);
            transport.send(node, leftNode, updateNeighbourMsg, pid);
        }

        for (Tuple<Node, BootstrapStatus> rightTuple : symphony.rightShortRangeLinks) {
            Node rightNode = rightTuple.x;
            Message updateNeighbourMsg = new Message(symphony.loggedIntoNetwork, node, MessageType.UPDATE_NEIGHBOURS);
            transport.send(node, rightNode, updateNeighbourMsg, pid);
        }
    }

    /**
     * Method to update the (connection) status of the node. Perform the update to the "up" so:
     * OFFLINE -> ONLINE_AND_ALL_NEIGHBOURS_OFFLINE -> ONLINE
     *
     * and to the "down" only: ONLINE -> ONLINE_AND_ALL_NEIGHBOURS_OFFLINE
     *
     * @param node
     */
    private void checkBootstrapStatus(Node node) {
        SymphonyProtocol symphony = (SymphonyProtocol) node.getProtocol(symphonyID);

        if (symphony.loggedIntoNetwork != BootstrapStatus.OFFLINE) {

            symphony.loggedIntoNetwork = BootstrapStatus.ONLINE_AND_ALL_NEIGHBOURS_OFFLINE;

            // Check if i'm inside the ring and i'm able to do routing
            if (!symphony.leftShortRangeLinks.isEmpty() && !symphony.rightShortRangeLinks.isEmpty()) {

                boolean leftOk = false;
                for (Tuple<Node, BootstrapStatus> leftTuple : symphony.leftShortRangeLinks) {
                    if (leftTuple.y == BootstrapStatus.ONLINE) {
                        leftOk = true;
                        break;
                    }
                }

                if (leftOk) {
                    for (Tuple<Node, BootstrapStatus> rightTuple : symphony.rightShortRangeLinks) {
                        if (rightTuple.y == BootstrapStatus.ONLINE) {
                            symphony.loggedIntoNetwork = BootstrapStatus.ONLINE;
                            break;
                        }
                    }
                }
            }
        }
    }

    /**
     * Remove the possible wrong entries from the lookAhead table
     */
    private void fixLookAheadMap(Node node) {
        SymphonyProtocol symphony = (SymphonyProtocol) node.getProtocol(symphonyID);
        for (Tuple<Node, BootstrapStatus> tuple : symphony.leftShortRangeLinks) {
            symphony.lookAheadMap.put(tuple.x, null);
        }
        for (Tuple<Node, BootstrapStatus> tuple : symphony.rightShortRangeLinks) {
            symphony.lookAheadMap.put(tuple.x, null);
        }
    }

    /**
     * Sent keep-alive messages to verify if the links still online
     *
     * if enable the lookAhead mode i require the neighbours list from my neighbours (1-lookAhead).
     *
     * Note: I don't reuse the UPDATE_STATUS messages because i want to mantain separate the
     * semantic and have more clear source code
     */
    private void keepAlive(Node node) {

        SymphonyProtocol symphony = (SymphonyProtocol) node.getProtocol(symphonyID);
        Transport transport = (Transport) node.getProtocol(transportID);

        // Send and check for the long range links (both incoming and outgoing)
        for (Iterable<Node> iterable : new Iterable[]{symphony.longRangeLinksOutgoing, symphony.longRangeLinksIncoming}) {
            Iterator<Node> longLinksIterator = iterable.iterator();
            while (longLinksIterator.hasNext()) {
                Node longLinkNode = longLinksIterator.next();
                Integer value = keepAliveMap.get(longLinkNode);
                if (value == null) {
                    value = 0;
                }

                /*
                 * Verify if i reached the sufficient time of sending and not receiving an answer
                 * and so i can consider that node as disconnected
                 */
                if (value >= nTimeout) {
                    symphony.lookAheadMap.put(longLinkNode, null); // Do it anyway if it's enabled the lookAhead or not
                    longLinksIterator.remove();
                } else {
                    keepAliveMap.put(longLinkNode, value + 1);

                    Message keepAliveMsg = new Message(symphony.lookAhead, node, MessageType.KEEP_ALIVE);
                    transport.send(node, longLinkNode, keepAliveMsg, pid);
                }
            }
        }

        // Send and check for the short links
        for (Iterable<Tuple<Node, BootstrapStatus>> iterable : new Iterable[]{symphony.rightShortRangeLinks, symphony.leftShortRangeLinks}) {
            Iterator<Tuple<Node, BootstrapStatus>> shortLinksIterator = iterable.iterator();
            while (shortLinksIterator.hasNext()) {
                Node shortLinkNode = shortLinksIterator.next().x;
                Integer value = keepAliveMap.get(shortLinkNode);
                if (value == null) {
                    value = 0;
                }

                // the same as above
                if (value >= nTimeout) {
                    shortLinksIterator.remove();
                } else {
                    keepAliveMap.put(shortLinkNode, value + 1);

                    // LookAhead is not to be performed to the short links!
                    Message keepAliveMsg = new Message(false, node, MessageType.KEEP_ALIVE);
                    transport.send(node, shortLinkNode, keepAliveMsg, pid);
                }
            }
        }
    }

    /**
     * Implement the Re-Linking criteria of the Long Range Links. It does the complete refresh. The
     * repopulation is done through the 'updateLongRangeLinks' method.
     */
    private void reLinkingProtocol(Node node) {
        // I do the check only if i succeed at least one time to create a long range link
        if (nLink > 0) {
            double criterionValue = n / nLink;

            if (!(criterionValue >= relinkingLowerBound && criterionValue <= relinkingUpperBound)) {

                /*
                 * Not explicitly precised in the paper: if i haven't created a new link i update
                 * anyway nLink because can happen a special case that i will not be able to create
                 * links because the reLinkingProtocol procedure is "faster".
                 */
                nLink = n;

                SymphonyProtocol symphony = (SymphonyProtocol) node.getProtocol(symphonyID);
                Transport transport = (Transport) node.getProtocol(transportID);

                // Communicate to the all Outgoing Long Range Links that they aren't anymore
                for (Node longRangeLinkOutgoingNode : symphony.longRangeLinksOutgoing) {
                    Message disconnectMsg = new Message(null, node, MessageType.DISCONNECT_LONG_RANGE_LINK);
                    transport.send(node, longRangeLinkOutgoingNode, disconnectMsg, pid);
                }

                symphony.longRangeLinksOutgoing.clear();
            }
        }
    }

    public int getK() {
        return k;
    }

    public int getN() {
        return n;
    }
}
