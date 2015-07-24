package example.symphony;

import java.util.Comparator;

import example.symphony.SymphonyProtocol.BootstrapStatus;
import peersim.core.Node;

/**
 * Object-Adapter
 *
 * @author Andrea Esposito <and1989@gmail.com>
 */
public class AdapterSymphonyNodeComparator implements Comparator<Tuple<Node, BootstrapStatus>> {

    private SymphonyNodeComparator comparator;

    public AdapterSymphonyNodeComparator(SymphonyNodeComparator comparator) {
        this.comparator = comparator;
    }

    public int compare(Tuple<Node, BootstrapStatus> o1, Tuple<Node, BootstrapStatus> o2) {

        Node node1 = o1.x;
        Node node2 = o2.x;

        return comparator.compare(node1, node2);
    }
}
