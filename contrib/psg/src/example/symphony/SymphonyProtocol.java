package example.symphony;

import java.lang.ref.SoftReference;
import java.util.*;
import java.util.logging.Level;
import java.util.logging.Logger;

import example.symphony.Message.MessageType;
import peersim.config.Configuration;
import peersim.core.CommonState;
import peersim.core.Node;
import peersim.edsim.EDProtocol;
import peersim.transport.Transport;

/**
 *
 * @author Andrea Esposito <and1989@gmail.com>
 */
public class SymphonyProtocol implements EDProtocol {

    private static final String PAR_SHORT_LINK = "shortlink";
    private static final String PAR_LONG_LINK = "longlink";
    private static final String PAR_TRANSP = "transport";
    private static final String PAR_ROUTING = "routing";
    private static final String PAR_LOOKAHEAD = "lookahead";
    private static Set<Double> allIdentifier = new HashSet<Double>();
    private final String prefix;
    private static int pid;
    private final int transportID;
    private final double identifier;
    public final int sequentialIdentifier;
    private static int sequentialCounter = 0;
    public final int numberShortRangeLinksPerSide;
    public final boolean bidirectionalRouting;
    public final boolean lookAhead;
    public final boolean fixedLongRangeLinks;
    public final int numberFixedLongRangeLinks;
    public LinkedHashSet<Node> longRangeLinksOutgoing;
    public LinkedHashSet<Node> longRangeLinksIncoming;
    public LinkedHashSet<Tuple<Node, BootstrapStatus>> rightShortRangeLinks;
    public LinkedHashSet<Tuple<Node, BootstrapStatus>> leftShortRangeLinks;
    /**
     * Array Contract: at position 0 -> OutgoingLongRangeLinks, 1 -> IncomingLongRangeLinks
     */
    public final LinkedHashMap<Node, Set<Double>[]> lookAheadMap;
    private HashMap<Double, Handler> mapHandler;
    /**
     * IDs Set to verify if there are cycles
     */
    private Set<Long> messageHistoryID;
    /**
     *
     * Tuple chronology that contains: <received message, the possible answer message>
     *
     * I use SoftReference as a trade off between memory usage and accurancy
     */
    private Set<SoftReference<Tuple<Message, Message>>> messageHistory;
    private static boolean firstPrintConfig = true;

    public enum BootstrapStatus {

        NEW, OFFLINE, ONLINE_AND_ALL_NEIGHBOURS_OFFLINE, ONLINE
    }
    public BootstrapStatus loggedIntoNetwork;

    public SymphonyProtocol(String prefix) {

        this.prefix = prefix;
        pid = Configuration.lookupPid(prefix.replaceAll("protocol.", ""));
        transportID = Configuration.getPid(prefix + "." + PAR_TRANSP);
        numberShortRangeLinksPerSide = Configuration.getInt(prefix + "." + PAR_SHORT_LINK, 2) / 2;
        bidirectionalRouting = !Configuration.getString(prefix + "." + PAR_ROUTING, "bidirectional").toLowerCase().equals("unidirectional");
        lookAhead = !Configuration.getString(prefix + "." + PAR_LOOKAHEAD, "on").toLowerCase().equals("off");
        numberFixedLongRangeLinks = Configuration.getInt(prefix + "." + PAR_LONG_LINK, -1);
        fixedLongRangeLinks = numberFixedLongRangeLinks >= 0;

        longRangeLinksOutgoing = new LinkedHashSet<Node>();
        longRangeLinksIncoming = new LinkedHashSet<Node>();
        rightShortRangeLinks = new LinkedHashSet<Tuple<Node, BootstrapStatus>>();
        leftShortRangeLinks = new LinkedHashSet<Tuple<Node, BootstrapStatus>>();
        lookAheadMap = new LinkedHashMap<Node, Set<Double>[]>();

        identifier = generateUniqueIdentifier();
        sequentialIdentifier = sequentialCounter++;

        mapHandler = new HashMap<Double, Handler>();

        messageHistoryID = new HashSet<Long>();
        messageHistory = new LinkedHashSet<SoftReference<Tuple<Message, Message>>>();
        loggedIntoNetwork = BootstrapStatus.NEW;

        printConfig();
    }

    private void printConfig() {

        if (firstPrintConfig) {
            firstPrintConfig = false;
            System.out.println(SymphonyProtocol.class.getSimpleName() + " Configuration:");
            System.out.println("- Number of short range links per side: " + numberShortRangeLinksPerSide);
            System.out.println("- Number of long range links: " + (fixedLongRangeLinks ? numberFixedLongRangeLinks : "log(n)"));
            System.out.println("- Routing mode: " + (bidirectionalRouting ? "Bidirectional" : "Unidirectional"));
            System.out.println("- LookAhead status: " + (lookAhead ? "ON" : "OFF"));
            System.out.println("-------------------------------\n");
        }
    }

    /**
     *
     * Method to identify the next node that has to be contacted. It's going to be used the mode
     * that is described into the configuration file
     */
    public Node getCandidateForRouting(double identifierToRoute) throws RoutingException {
        if (bidirectionalRouting) {
            return getCandidateForBidirectionalRoute(identifierToRoute);
        } else {
            return getCandidateForUnidirectionalRoute(identifierToRoute);
        }
    }

    /**
     *
     * Method to individuate the next node that as to be contacted through Unidirectional Routing
     * mode
     */
    public Node getCandidateForUnidirectionalRoute(double identifierToRoute) throws RoutingException {

        LinkedHashSet<Node> allLinks = new LinkedHashSet<Node>();
        Node manager = putShortRangeLinksIntoContainerForRouting(allLinks, identifierToRoute);

        if (manager != null) {
            return manager;
        }

        allLinks.addAll(longRangeLinksOutgoing);

        return findClosestNode(identifierToRoute, allLinks, true);
    }

    /**
     * Method to individuate the next node that as to be contacted through Bidirectional Routing
     * mode
     */
    public Node getCandidateForBidirectionalRoute(double identifierToRoute) throws RoutingException {

        LinkedHashSet<Node> allLinks = new LinkedHashSet<Node>();
        Node manager = putShortRangeLinksIntoContainerForRouting(allLinks, identifierToRoute);

        if (manager != null) {
            return manager;
        }

        allLinks.addAll(longRangeLinksOutgoing);
        allLinks.addAll(longRangeLinksIncoming);

        return findClosestNode(identifierToRoute, allLinks, false);
    }

    /**
     * @return Null if it is NOT found the manager. Node if it is found.
     */
    private Node putShortRangeLinksIntoContainerForRouting(Set<Node> container, double identifierToRoute) {
        for (Tuple<Node, BootstrapStatus> rightTuple : rightShortRangeLinks) {
            if (rightTuple.y == BootstrapStatus.ONLINE) {
                container.add(rightTuple.x);
            }
        }

        if (!container.isEmpty()) {

            // Special case: i verify if the neighbour at my right (ONLINE) is the manager
            SymphonyNodeComparator comparator = new SymphonyNodeComparator(pid, identifier);
            Node nearRightNeighbour = Collections.min(container, comparator);
            if (nearRightNeighbour != null) {
                SymphonyProtocol symphony = (SymphonyProtocol) nearRightNeighbour.getProtocol(pid);
                if (!isLeftNeighbour(identifier, identifierToRoute) && isLeftNeighbour(symphony.getIdentifier(), identifierToRoute)) {
                    return nearRightNeighbour;
                }
            }
        }

        for (Tuple<Node, BootstrapStatus> leftTuple : leftShortRangeLinks) {
            if (leftTuple.y == BootstrapStatus.ONLINE) {
                container.add(leftTuple.x);
            }
        }

        return null;
    }

    /**
     *
     * Individuates effectively the next candidate for the routing. Checks if the lookahead is
     * activated and in case of affirmative answer it's going to use that information.
     *
     * @param identifierToRoute Identifier to reach
     * @param container Candidate Nodes Container
     * @param clockwise true, does unidirectional routing. false, does bidirectional routing.
     * @return The nearest node to reach identifierToRoute
     * @throws RoutingException Throw in case no candidate is found
     */
    public Node findClosestNode(final double identifierToRoute, final Iterable<Node> container, final boolean clockwise) throws RoutingException {
        Node ret = null;
        double min = Double.MAX_VALUE;

        for (Node node : container) {
            SymphonyProtocol symphonyNodeContainer = (SymphonyProtocol) node.getProtocol(pid);
            double realCandidateIdentifier = symphonyNodeContainer.getIdentifier();

            Set<Double> candidateIdentifierSet = new LinkedHashSet<Double>();
            candidateIdentifierSet.add(realCandidateIdentifier);

            boolean lookAheadClockwise = true;

            /*
             *
             * If lookahead is activated add all the reachable identifiers. No checks are performed
             * on the node type (short/long) because at maximum the map return null.
             */
            if (lookAhead) {
                Set<Double>[] lookAheadIdentifierSetArray = lookAheadMap.get(node);

                if (lookAheadIdentifierSetArray != null) {
                    Set<Double> lookAheadIdentifierSet = lookAheadIdentifierSetArray[0];

                    if (lookAheadIdentifierSet == null) {
                        lookAheadIdentifierSet = new LinkedHashSet<Double>();
                    }

                    /*
                     *
                     * If bidirectional routing is going to be performed so i put into account also
                     * the Incoming Long Range Links of the current neighbour
                     */
                    if (bidirectionalRouting && lookAheadIdentifierSetArray[1] != null) {
                        lookAheadIdentifierSet.addAll(lookAheadIdentifierSetArray[1]);
                        lookAheadClockwise = false;
                    }

                    if (!lookAheadIdentifierSet.isEmpty()) {
                        candidateIdentifierSet.addAll(lookAheadIdentifierSet);
                    }
                }
            }

            for (Double candidateIdentifier : candidateIdentifierSet) {
                // if it is a my neighbour i use my routing mode instead if it is a looAhead one i use its routing mode
                boolean currentClockwise = candidateIdentifier.equals(realCandidateIdentifier) ? clockwise : lookAheadClockwise;

                double distance = Math.abs(candidateIdentifier - identifierToRoute);
                distance = Math.min(distance, 1.0 - distance);

                // if clockwise i have to exclude the case: candidateIdentifier - indentifierToRoute - identifier
                if (currentClockwise) {
                    if (isLeftNeighbour(candidateIdentifier, identifierToRoute)) {

                        // Special case (0.9 - 0.1) the normal order is not more meanful to decide the side
                        if (identifierToRoute >= candidateIdentifier) {
                            distance = identifierToRoute - candidateIdentifier;
                        } else {
                            distance = (1.0 - candidateIdentifier) + identifierToRoute;
                        }
                    } else {
                        distance = (1.0 - (candidateIdentifier - identifierToRoute)) % 1;
                    }
                }

                /*
                 *
                 * Priority to the node that i'm directly connected and only after i use the
                 * lookAhead information
                 */
                if (min >= Math.abs(distance)
                        && (candidateIdentifier.equals(realCandidateIdentifier)
                        || ret == null
                        || min > Math.abs(distance))) {
                    ret = node;
                    min = Math.abs(distance);
                }
            }
        }

        if (ret == null) {
            throw new RoutingException("Impossible do routing. [Hit: Neighbour links (maybe) not yet online.");
        }

        return ret;
    }

    /**
     *
     * @param neighbourNode Neighbour Node
     * @return true if the node is a left neighbour (or itself), false if it is a right one
     */
    public static boolean isLeftNeighbour(Node rootNode, Node neighbourNode) {
        SymphonyProtocol rootSymphony = (SymphonyProtocol) rootNode.getProtocol(pid);
        SymphonyProtocol neighbourSymphony = (SymphonyProtocol) neighbourNode.getProtocol(pid);

        return isLeftNeighbour(rootSymphony.getIdentifier(), neighbourSymphony.getIdentifier());
    }

    public static boolean isLeftNeighbour(double rootIdentifier, double neighbourIdentifier) {

        // I calculate putting the hypotesis that i have to translate/"normalize", after i'll check if it was useless
        double traslateRootIdentifier = (rootIdentifier + 0.5) % 1;
        double traslateNeighbourIdentifier = (neighbourIdentifier + 0.5) % 1;
        double distance = traslateNeighbourIdentifier - traslateRootIdentifier;

        // I verify if the neighbourIdentifier is over half ring, if yes i don't need to do the translation/"normalization"
        if ((neighbourIdentifier + 0.5) != traslateNeighbourIdentifier) {
            distance = neighbourIdentifier - rootIdentifier;
        }

        return distance >= 0 && distance <= 0.5;
    }

    public void route(Node src, double key, Handler handler) throws RoutingException {

        mapHandler.put(key, handler);

        Message msg = new Message(key, src, MessageType.ROUTE);

        Node targetNode = src;

        if (!isManagerOf(key)) {
            targetNode = getCandidateForRouting(key);
            Transport transport = (Transport) src.getProtocol(transportID);
            transport.send(src, targetNode, msg, pid);
        }

        // Insert the message into the chronology
        Tuple<Message, Message> historyTuple = new Tuple<Message, Message>();
        try {
            historyTuple.x = msg;
            historyTuple.y = (Message) msg.clone();
            historyTuple.y.setCurrentHop(targetNode);
        } catch (CloneNotSupportedException ex) {
            Logger.getLogger(SymphonyProtocol.class.getName()).log(Level.SEVERE, "Impossible to clonate the message!");
            historyTuple.x = null;
            historyTuple.y = msg;
            msg.setCurrentHop(targetNode);
        }
        messageHistory.add(new SoftReference<Tuple<Message, Message>>(historyTuple));
        messageHistoryID.add(msg.getID());

        /*
         *
         * If i am the manager (brutally through the reference), i don't do the loopback routing but
         * i soddisfy immediately the request
         */
        if (targetNode == src) {

            // Uppdate the chronology
            historyTuple.y = new Message(key, targetNode, MessageType.ROUTE_RESPONSE);

            Tuple<Node, Double> tuple = new Tuple<Node, Double>(src, key);
            mapHandler.remove(key);
            handler.handle(this, tuple);
        }
    }

    public void processEvent(Node node, int pid, Object event) {
        Message msg = (Message) event;
        msg.incrementHop(); // I increment the message Hop

        Tuple<Message, Message> historyTuple = new Tuple<Message, Message>();
        try {
            // I clone the message such a way to store into the chronology the hop sender's information
            historyTuple.x = (Message) msg.clone();
        } catch (CloneNotSupportedException ex) {
            Logger.getLogger(SymphonyProtocol.class.getName()).log(Level.SEVERE, "Impossible to clonate the message!");
            historyTuple.x = msg;
        }

        messageHistory.add(new SoftReference<Tuple<Message, Message>>(historyTuple));

        Double key;
        Transport transport;
        Handler handler;

        // Individuate cycles
        if (messageHistoryID.contains(msg.getID())) {
            Message responseMsg = new Message(msg, node, MessageType.ROUTE_FAIL);

            historyTuple.y = responseMsg;

            transport = (Transport) node.getProtocol(transportID);
            transport.send(node, msg.getSourceNode(), responseMsg, pid);
            return;
        }

        /*
         * If i'm arrived till here means that i'm not into a cycle --> i insert the message ID into
         * the chronology
         */
        messageHistoryID.add(msg.getID());

        switch (msg.getType()) {
            case ROUTE:
                key = (Double) msg.getBody();
                Logger.getLogger(SymphonyProtocol.class.getName()).log(Level.FINEST, key + " " + identifier);
                if (isManagerOf(key)) {
                    transport = (Transport) msg.getSourceNode().getProtocol(transportID);
                    Message responseMsg = new Message(new Tuple<Node, Double>(node, key), node, MessageType.ROUTE_RESPONSE);
                    historyTuple.y = responseMsg;
                    transport.send(node, msg.getSourceNode(), responseMsg, pid);
                } else {
                    try {
                        Node targetNode = getCandidateForRouting(key);

                        try {
                            // I clone the message such a way to store the info (into the chronology) of the hop receiver
                            historyTuple.y = (Message) msg.clone();
                            historyTuple.y.setCurrentHop(targetNode);
                        } catch (CloneNotSupportedException ex) {
                            Logger.getLogger(SymphonyProtocol.class.getName()).log(Level.SEVERE, "Impossible to clonate the message!");
                            historyTuple.y = msg;
                            msg.setCurrentHop(targetNode);
                        }

                        transport = (Transport) node.getProtocol(transportID);
                        transport.send(node, targetNode, msg, pid);
                    } catch (RoutingException ex) {
                        /*
                         *
                         * I send the same message to myself (it is going to queue into the event
                         * queue and in this way i "earn" time (postpone) and i hope that the
                         * network will be ok in the meanwhile)
                         */
                        historyTuple.y = msg;
                        msg.setCurrentHop(node);
                        transport = (Transport) node.getProtocol(transportID);
                        transport.send(node, node, msg, pid);
                    }
                }
                break;
            case ROUTE_RESPONSE:
                Tuple<Node, Double> tuple = (Tuple<Node, Double>) msg.getBody();
                key = tuple.y;
                handler = mapHandler.get(key);
                mapHandler.remove(key);
                handler.handle(this, tuple);
                break;
            case ROUTE_FAIL:
                Message requestMsg = (Message) msg.getBody();
                key = (Double) requestMsg.getBody();
                handler = mapHandler.get(key);
                mapHandler.remove(key);
                handler.handle(this, null);
                break;
        }
    }

    public boolean isManagerOf(double key) {

        if (key == identifier) {
            return true;
        }

        SymphonyNodeComparator comparator = new SymphonyNodeComparator(pid, identifier);
        AdapterSymphonyNodeComparator adapterComparator = new AdapterSymphonyNodeComparator(comparator);

        Collection<Tuple<Node, BootstrapStatus>> leftShortRangeLinksCloned = (Collection<Tuple<Node, BootstrapStatus>>) leftShortRangeLinks.clone();
        Node targetNode = null;

        while (targetNode == null && !leftShortRangeLinksCloned.isEmpty()) {
            Tuple<Node, BootstrapStatus> nearTuple = Collections.min(leftShortRangeLinksCloned, adapterComparator);
            if (nearTuple.y == BootstrapStatus.ONLINE) {
                targetNode = nearTuple.x;
            } else {
                leftShortRangeLinksCloned.remove(nearTuple);
            }
        }

        // SPECIAL CASE: NO LEFT NEIGHBOURS. I became the Manager.
        if (targetNode == null) {
            return true;
        }

        SymphonyProtocol symphony = (SymphonyProtocol) targetNode.getProtocol(pid);
        // Check if it's the situation: right neighbour - key - me. So if i'm the manager or not.
        boolean ret = isLeftNeighbour(identifier, key) && (!isLeftNeighbour(symphony.getIdentifier(), key) && symphony.getIdentifier() != key);

        return ret;
    }

    public double getIdentifier() {
        return identifier;
    }

    public Tuple<Message, Message>[] getHistoryMessage() {
        SoftReference<Tuple<Message, Message>>[] array = messageHistory.toArray(new SoftReference[0]);
        LinkedList<Tuple<Message, Message>> list = new LinkedList<Tuple<Message, Message>>();
        for (SoftReference<Tuple<Message, Message>> reference : array) {
            Tuple<Message, Message> tuple = reference.get();
            if (tuple != null) {
                list.add(tuple);
            }
        }
        return list.toArray(new Tuple[0]);
    }

    public void clearHistoryMessage() {
        messageHistory.clear();
    }

    private double generateUniqueIdentifier() {
        boolean duplicated = true;
        Double id = null;

        while (duplicated) {
            id = CommonState.r.nextDouble();
            duplicated = allIdentifier.contains(id);
        }

        allIdentifier.add(id);

        return id;
    }

    @Override
    public Object clone() {
        SymphonyProtocol dolly = new SymphonyProtocol(prefix);
        return dolly;
    }

    public boolean isBootstrapped() {
        return loggedIntoNetwork == BootstrapStatus.ONLINE;
    }
}
