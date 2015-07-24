package example.symphony;

import java.util.Iterator;
import peersim.core.Network;
import peersim.core.Node;

/**
 * Adapter Class absolutely UNSAFE, just to be able to iterate peersim.core.Network
 *
 * @author Andrea Esposito <and1989@gmail.com>
 */
public class AdapterIterableNetwork implements Iterable<Node>, Iterator<Node> {

    private int i = 0;

    public Iterator<Node> iterator() {
        return this;
    }

    public boolean hasNext() {
        return i < Network.size();
    }

    public Node next() {
        return Network.get(i++);
    }

    public void remove() {
        throw new UnsupportedOperationException("Not supported yet.");
    }
}
