package example.symphony;

import peersim.core.Node;
import peersim.core.Protocol;

/**
 *
 * @author Andrea Esposito <and1989@gmail.com>
 */
public interface NetworkSizeEstimatorProtocolInterface extends Protocol {

    public int getNetworkSize(Node node);
}
