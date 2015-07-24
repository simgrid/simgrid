package example.symphony;

import java.util.Comparator;
import java.util.logging.Level;
import java.util.logging.Logger;
import peersim.core.Node;

/**
 * Comparator that measure the relative distance from a target node
 *
 * @author Andrea Esposito <and1989@gmail.com>
 */
public class SymphonyNodeComparator implements Comparator<Node> {

    private final int symphonyID;
    private double target;

    public SymphonyNodeComparator(int symphonyID) {
        this.symphonyID = symphonyID;
    }

    public SymphonyNodeComparator(int symphonyID, double target) {
        this(symphonyID);
        this.target = target;
    }

    public SymphonyNodeComparator(int symphonyID, Node targetNode) {
        this(symphonyID);
        SymphonyProtocol symphony = (SymphonyProtocol) targetNode.getProtocol(symphonyID);
        this.target = symphony.getIdentifier();
    }

    public int compare(Node o1, Node o2) {

        SymphonyProtocol symphony1 = (SymphonyProtocol) o1.getProtocol(symphonyID);
        SymphonyProtocol symphony2 = (SymphonyProtocol) o2.getProtocol(symphonyID);

        Double identifier1 = symphony1.getIdentifier();
        Double identifier2 = symphony2.getIdentifier();

        Double distance1 = Math.abs(target - identifier1) % 1;
        Double distance2 = Math.abs(target - identifier2) % 1;

        identifier1 = Math.min(1.0 - distance1, distance1);
        identifier2 = Math.min(1.0 - distance2, distance2);

        Logger.getLogger(SymphonyNodeComparator.class.getName()).log(Level.FINEST, "id1= " + symphony1.getIdentifier() + " target= " + target + " id2= " + symphony2.getIdentifier() + " res= " + identifier1.compareTo(identifier2));

        return identifier1.compareTo(identifier2);
    }
}
