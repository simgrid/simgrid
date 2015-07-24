package example.symphony;

import peersim.core.Node;

/**
 *
 * @author Andrea Esposito <and1989@gmail.com>
 */
public interface Handler {

    /**
     * Handler associable to a routing request
     *
     * @param src Symphony Protocol that has sent the routing request
     * @param evt Tuple that contains: Node that manages the identifier, Identifier that the routing
     * has done on
     */
    void handle(SymphonyProtocol src, Tuple<Node, Double> evt);
}
